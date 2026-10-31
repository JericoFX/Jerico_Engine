/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Model/ModelAsset.h>
#include <JE/Render/RPI/Reflect/Model/ModelAssetHelpers.h>
#include <JE/Render/RPI/Reflect/Model/ModelKdTree.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Sha1.h>
#include <Atom/RHI.Reflect/Format.h>
#include <cstring>
#include <cmath>
#include <AzCore/std/limits.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Translation/TranslationDef.h>

#include <AzFramework/Asset/AssetSystemBus.h>

namespace JE::Render
{
    namespace RPI
    {
        namespace
        {
            class ModelContentEncoder
            {
            public:
                explicit ModelContentEncoder(size_t budget)
                    : m_budget(AZStd::min(budget, ModelAsset::ContentRevisionMaxEncodedBytes)) {}
                bool Bytes(const void* value, size_t size)
                {
                    if (size > m_budget - m_written) { return false; }
                    if (size != 0) { m_hash.ProcessBytes(reinterpret_cast<const AZStd::byte*>(value), size); }
                    m_written += size; return true;
                }
                bool Word(AZ::u32 value)
                {
                    const AZ::u8 bytes[]{static_cast<AZ::u8>(value), static_cast<AZ::u8>(value >> 8),
                        static_cast<AZ::u8>(value >> 16), static_cast<AZ::u8>(value >> 24)};
                    return Bytes(bytes, sizeof(bytes));
                }
                bool Wide(AZ::u64 value) { return Word(static_cast<AZ::u32>(value)) && Word(static_cast<AZ::u32>(value >> 32)); }
                bool Scalar(float value)
                {
                    static_assert(sizeof(float) == sizeof(AZ::u32) && AZStd::numeric_limits<float>::is_iec559);
                    if (!std::isfinite(value)) { return false; }
                    AZ::u32 bits; std::memcpy(&bits, &value, sizeof(bits));
                    if ((bits & 0x7fffffffu) == 0) { bits = 0; }
                    return Word(bits);
                }
                bool Vector(const AZ::Vector3& value)
                { return Scalar(value.GetX()) && Scalar(value.GetY()) && Scalar(value.GetZ()); }
                bool Bounds(const AZ::Aabb& value)
                { return value.IsValid() && Vector(value.GetMin()) && Vector(value.GetMax()); }
                bool String(AZStd::string_view value) { return Wide(value.size()) && Bytes(value.data(), value.size()); }
                bool View(const BufferAssetView& view, bool index, uint32_t vertexCount, bool position = false)
                {
                    // A temporary Asset handle pins existing CPU bytes only. It does not QueueLoad/AddRefBufferAssets.
                    const auto asset = view.GetBufferAsset();
                    if (!asset.IsReady() || !asset.Get()) { return false; }
                    const auto& descriptor = view.GetBufferViewDescriptor();
                    const auto format = descriptor.m_elementFormat;
                    const uint32_t stride = descriptor.m_elementSize;
                    if (descriptor.m_elementCount == 0 || stride == 0 ||
                        static_cast<AZ::u32>(format) >= static_cast<AZ::u32>(AZ::RHI::Format::Count)) { return false; }
                    if (format != AZ::RHI::Format::Unknown && AZ::RHI::GetFormatSize(format) != stride) { return false; }
                    uint32_t floatWidth = 0;
                    uint32_t contentWidth = stride;
                    if (index)
                    {
                        if (descriptor.m_elementCount % 3 != 0 || (stride != 2 && stride != 4) ||
                            (format != AZ::RHI::Format::Unknown &&
                                !((stride == 2 && format == AZ::RHI::Format::R16_UINT) ||
                                  (stride == 4 && format == AZ::RHI::Format::R32_UINT)))) { return false; }
                    }
                    else
                    {
                        if (descriptor.m_elementCount != vertexCount) { return false; }
                        switch (format)
                        {
                        case AZ::RHI::Format::Unknown:
                            // Only the audited float3 POSITION layout is known; the fourth structured lane is padding.
                            if (!position || (stride != 12 && stride != 16)) { return false; }
                            floatWidth = 4; contentWidth = 12; break;
                        case AZ::RHI::Format::R32_FLOAT:
                        case AZ::RHI::Format::R32G32_FLOAT:
                        case AZ::RHI::Format::R32G32B32_FLOAT:
                        case AZ::RHI::Format::R32G32B32A32_FLOAT:
                            floatWidth = 4; break;
                        case AZ::RHI::Format::R16_FLOAT:
                        case AZ::RHI::Format::R16G16_FLOAT:
                        case AZ::RHI::Format::R16G16B16A16_FLOAT:
                            floatWidth = 2; break;
                        case AZ::RHI::Format::R16_UINT:
                        case AZ::RHI::Format::R32_UINT:
                        case AZ::RHI::Format::R8G8B8A8_UNORM:
                        case AZ::RHI::Format::R8G8B8A8_UINT:
                        case AZ::RHI::Format::R8G8B8A8_SNORM:
                        case AZ::RHI::Format::R8G8B8A8_SINT:
                            break;
                        default: return false; // No inferred packed-float, morph or ambiguous structured layout.
                        }
                        if (position && (contentWidth != 12 || floatWidth != 4)) { return false; }
                    }
                    const AZ::u64 begin = static_cast<AZ::u64>(descriptor.m_elementOffset) * stride;
                    const AZ::u64 length = static_cast<AZ::u64>(descriptor.m_elementCount) * stride;
                    // Deny this view before decoding/allocating if earlier views have exhausted the encoding budget.
                    const AZ::u64 encodedBytes = 24 + static_cast<AZ::u64>(descriptor.m_elementCount) * contentWidth;
                    if (encodedBytes > m_budget - m_written) { return false; }
                    BufferAsset::BufferData scratch;
                    const auto decoded = asset->GetDecodedCpuBuffer(scratch, m_budget);
                    if (!decoded) { return false; }
                    const auto bytes = *decoded;
                    const auto declaredBytes = asset->GetBufferDescriptor().m_byteCount;
                    // Products of u32 fields fit u64; subtraction checks avoid overflowing begin+length.
                    if (begin > bytes.size() || length > bytes.size() - begin ||
                        begin > declaredBytes || length > declaredBytes - begin) { return false; }
                    if (!Word(descriptor.m_elementOffset) || !Word(descriptor.m_elementCount) || !Word(stride) ||
                        !Word(static_cast<AZ::u32>(format)) || !Wide(static_cast<AZ::u64>(descriptor.m_elementCount) * contentWidth))
                    { return false; }
                    for (uint32_t element = 0; element != descriptor.m_elementCount; ++element)
                    {
                        const uint8_t* record = bytes.data() + static_cast<size_t>(begin) + static_cast<size_t>(element) * stride;
                        if (index)
                        {
                            uint32_t value = 0;
                            std::memcpy(&value, record, stride);
                            if (value >= vertexCount || !Bytes(record, stride)) { return false; }
                        }
                        else if (floatWidth == 4)
                        {
                            for (uint32_t component = 0; component != contentWidth / 4; ++component)
                            {
                                float value; std::memcpy(&value, record + component * 4, sizeof(value));
                                if (!Scalar(value)) { return false; }
                            }
                        }
                        else if (floatWidth == 2)
                        {
                            for (uint32_t component = 0; component != contentWidth / 2; ++component)
                            {
                                uint16_t bits; std::memcpy(&bits, record + component * 2, sizeof(bits));
                                if ((bits & 0x7c00u) == 0x7c00u) { return false; } // Half infinity/NaN.
                                if ((bits & 0x7fffu) == 0) { bits = 0; }
                                const uint8_t canonical[]{static_cast<uint8_t>(bits), static_cast<uint8_t>(bits >> 8)};
                                if (!Bytes(canonical, sizeof(canonical))) { return false; }
                            }
                        }
                        else if (!Bytes(record, contentWidth)) { return false; }
                    }
                    return true;
                }
                bool Identities(const ModelLodAsset::Mesh& mesh)
                {
                    const auto& table = mesh.GetSurfaceIdentityTable();
                    if (table.m_schema == 0)
                    { return table.m_entries.empty() && Word(0) && Wide(0); }
                    if (table.m_schema != ModelSurfaceIdentityTable::Schema || mesh.GetIndexCount() % 3 != 0 ||
                        table.m_entries.empty() || table.m_entries.size() != mesh.GetIndexCount() / 3) { return false; }
                    // Check bounded canonical payload before validation allocations or traversal.
                    const size_t available = m_budget - m_written;
                    if (available < 12 || table.m_entries.size() > (available - 12) / 36) { return false; }
                    size_t required = 12;
                    for (const auto& entry : table.m_entries)
                    {
                        const size_t width = entry.m_regionId ? 52 : 36;
                        if (width > available - required) { return false; }
                        required += width;
                    }
                    if (!Word(table.m_schema) || !Wide(table.m_entries.size())) { return false; }
                    for (const auto& entry : table.m_entries)
                    {
                        if (entry.m_surfaceId.IsNull() || entry.m_faceId.IsNull() ||
                            (entry.m_regionId && entry.m_regionId->IsNull())) { return false; }
                        const auto inserted = m_faceOwners.emplace(entry.m_faceId, entry.m_surfaceId);
                        if (!inserted.second && inserted.first->second != entry.m_surfaceId) { return false; }
                        if (!Bytes(entry.m_surfaceId.begin(), entry.m_surfaceId.size()) ||
                            !Bytes(entry.m_faceId.begin(), entry.m_faceId.size()) || !Word(entry.m_regionId ? 1 : 0) ||
                            (entry.m_regionId && !Bytes(entry.m_regionId->begin(), entry.m_regionId->size()))) { return false; }
                    }
                    return true;
                }
                ModelContentRevision Finish(AZ::u32 schema)
                {
                    ModelContentRevision result; result.m_schema = schema;
                    AZ::u32 digest[5]; m_hash.GetDigest(digest);
                    for (size_t i = 0; i != result.m_digest.size(); ++i) { result.m_digest[i] = digest[i]; }
                    return result;
                }
            private:
                AZ::Sha1 m_hash;
                size_t m_budget;
                size_t m_written = 0;
                AZStd::unordered_map<AZ::Uuid, AZ::Uuid> m_faceOwners;
            };
        }

        const ModelContentRevision& ModelAsset::GetContentRevision() const { return m_contentRevision; }

        AZStd::optional<ModelContentRevision> ModelAsset::ComputeContentRevision(size_t maxEncodedBytes) const
        {
            return ComputeContentRevisionForSchema(ModelContentRevision::Schema, maxEncodedBytes);
        }

        AZStd::optional<ModelContentRevision> ModelAsset::ComputeContentRevisionForSchema(AZ::u32 schema, size_t maxEncodedBytes) const
        {
            // Metadata is explicitly LE. Published buffer element bytes have LE product encoding on the active targets.
            const AZ::u32 endian = 1;
            if (*reinterpret_cast<const AZ::u8*>(&endian) != 1 || m_bufferAssetsRef == 0 || m_lodAssets.empty()) { return AZStd::nullopt; }
            constexpr char domain[] = "JE.Model.PublishedContent";
            ModelContentEncoder encoder(maxEncodedBytes);
            if (!encoder.Bytes(domain, sizeof(domain) - 1) || !encoder.Word(schema) ||
                !encoder.Wide(m_lodAssets.size()) || !encoder.Bounds(m_aabb)) { return AZStd::nullopt; }
            for (const auto& reference : m_lodAssets)
            {
                const auto lod = reference;
                if (!lod.IsReady() || !lod.Get() || lod->GetMeshes().empty() || !encoder.Bounds(lod->GetAabb()) ||
                    !encoder.Wide(lod->GetMeshes().size())) { return AZStd::nullopt; }
                for (const auto& mesh : lod->GetMeshes())
                {
                    const auto streams = mesh.GetStreamBufferInfoList();
                    if (streams.empty() || !encoder.Bounds(mesh.GetAabb()) || !encoder.Word(mesh.GetMaterialSlotId()) ||
                        !encoder.View(mesh.GetIndexBufferAssetView(), true, mesh.GetVertexCount()) || !encoder.Wide(streams.size())) { return AZStd::nullopt; }
                    bool position = false;
                    for (const auto& stream : streams)
                    {
                        const bool isPosition = stream.m_semantic.m_name == m_positionName && stream.m_semantic.m_index == 0;
                        if (isPosition && position) { return AZStd::nullopt; }
                        position |= isPosition;
                        if (!encoder.String(stream.m_semantic.m_name.GetStringView()) || !encoder.Word(stream.m_semantic.m_index) ||
                            !encoder.String(stream.m_customName.GetStringView()) || !encoder.View(stream.m_bufferAssetView, false, mesh.GetVertexCount(), isPosition)) { return AZStd::nullopt; }
                    }
                    if (!position) { return AZStd::nullopt; }
                    if (schema == ModelContentRevision::GeometrySchema)
                    {
                        const auto& table = mesh.GetSurfaceIdentityTable();
                        // A geometry-only claim cannot authenticate newly introduced identity data.
                        if (table.m_schema != 0 || !table.m_entries.empty()) { return AZStd::nullopt; }
                    }
                    else if (!encoder.Identities(mesh)) { return AZStd::nullopt; }
                }
            }
            return encoder.Finish(schema);
        }

        AZStd::optional<ModelContentRevision> ModelAsset::ValidateContentRevision(size_t maxEncodedBytes) const
        {
            if (m_contentRevision.m_schema != ModelContentRevision::Schema &&
                m_contentRevision.m_schema != ModelContentRevision::GeometrySchema) { return AZStd::nullopt; }
            const auto actual = ComputeContentRevisionForSchema(m_contentRevision.m_schema, maxEncodedBytes);
            return actual && *actual == m_contentRevision ? actual : AZStd::nullopt;
        }

        void ModelAsset::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelContentRevision>()
                    ->Version(1)
                    ->Field("Schema", &ModelContentRevision::m_schema)
                    ->Field("Digest", &ModelContentRevision::m_digest);
                serializeContext->Class<ModelAsset, AZ::Data::AssetData>()
                    ->Version(2)
                    ->Field("Name", &ModelAsset::m_name)
                    ->Field("Aabb", &ModelAsset::m_aabb)
                    ->Field("MaterialSlots", &ModelAsset::m_materialSlots)
                    ->Field("LodAssets", &ModelAsset::m_lodAssets)
                    ->Field("Tags", &ModelAsset::m_tags)
                    ->Field("ContentRevision", &ModelAsset::m_contentRevision)
                    ;

                // Note: This class needs to have edit context reflection so PropertyAssetCtrl::OnEditButtonClicked
                //       can open the asset with the preferred asset editor (Scene Settings).
                if (auto* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ModelAsset>(QT_TRANSLATE_NOOP("Atom::RPI", "Model Asset"), "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }

        ModelAsset::ModelAsset()
        {
            // c-tor and d-tor have to be defined in .cpp in order to have AZStd::unique_ptr<ModelKdTree> without having to include the header of KDTree
        }

        ModelAsset::~ModelAsset()
        {
            // c-tor and d-tor have to be defined in .cpp in order to have AZStd::unique_ptr<ModelKdTree> without having to include the header of KDTree
        }

        const AZ::Name& ModelAsset::GetName() const
        {
            return m_name;
        }

        const AZ::Aabb& ModelAsset::GetAabb() const
        {
            return m_aabb;
        }
        
        const ModelMaterialSlotMap& ModelAsset::GetMaterialSlots() const
        {
            return m_materialSlots;
        }
            
        const ModelMaterialSlot& ModelAsset::FindMaterialSlot(uint32_t stableId) const
        {
            auto iter = m_materialSlots.find(stableId);

            if (iter == m_materialSlots.end())
            {
                return m_fallbackSlot;
            }
            else
            {
                return iter->second;
            }
        }

        size_t ModelAsset::GetLodCount() const
        {
            return m_lodAssets.size();
        }

        AZStd::span<const AZ::Data::Asset<ModelLodAsset>> ModelAsset::GetLodAssets() const
        {
            return AZStd::span<const AZ::Data::Asset<ModelLodAsset>>(m_lodAssets);
        }

        void ModelAsset::LoadBufferAssets()
        {
            for (auto& lodAsset : m_lodAssets)
            {
                lodAsset->LoadBufferAssets();
            }
        }

        void ModelAsset::ReleaseBufferAssets()
        {
            for (auto& lodAsset : m_lodAssets)
            {
                lodAsset->ReleaseBufferAssets();
            }
        }

        void ModelAsset::AddRefBufferAssets()
        {
            if (m_bufferAssetsRef == 0)
            {
                LoadBufferAssets();
            }
            m_bufferAssetsRef++;
        }

        void ModelAsset::ReleaseRefBufferAssets()
        {
            if (m_bufferAssetsRef > 0)
            {
                m_bufferAssetsRef--;
                if (m_bufferAssetsRef == 0)
                {
                    ReleaseBufferAssets();
                }
            }
        }

        bool ModelAsset::SupportLocalRayIntersection() const
        {
            return m_bufferAssetsRef > 0;
        }

        void ModelAsset::SetReady()
        {
            m_status = AZ::Data::AssetData::AssetStatus::Ready;
        }

        bool ModelAsset::LocalRayIntersectionAgainstModel(
            const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, bool allowBruteForce,
            float& distanceNormalized, AZ::Vector3& normal) const
        {
            AZStd::optional<ModelMaterialSlot::StableId> materialSlotId;
            return LocalRayIntersectionAgainstModel(rayStart, rayDir, allowBruteForce, distanceNormalized, normal, materialSlotId);
        }

        bool ModelAsset::LocalRayIntersectionAgainstModel(
            const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, bool allowBruteForce,
            float& distanceNormalized, AZ::Vector3& normal,
            AZStd::optional<ModelMaterialSlot::StableId>& materialSlotId) const
        {
            materialSlotId.reset();
            if (!m_modelTriangleCount)
            {
                // [GFX TODO][ATOM-4343 Bake mesh spatial information during AP processing]
                m_modelTriangleCount = CalculateTriangleCount();
            }

            // check the total vertex count for this model and skip kd-tree if the model is simple enough
            if (*m_modelTriangleCount > s_minimumModelTriangleCountToOptimize)
            {
                if (!m_kdTree)
                {
                    BuildKdTree();

                    AZ_WarningOnce("Model", false, "ray intersection against a model that is still creating spatial information");
                    return allowBruteForce ? BruteForceRayIntersect(rayStart, rayDir, distanceNormalized, normal, materialSlotId) : false;
                }
                else
                {
                    return m_kdTree->RayIntersection(rayStart, rayDir, distanceNormalized, normal, materialSlotId);
                }
            }

            return BruteForceRayIntersect(rayStart, rayDir, distanceNormalized, normal, materialSlotId);
        }

        ModelRayQueryResult ModelAsset::LocalRayIntersectionsAgainstModel(
            const AZ::Vector3& start, const AZ::Vector3& direction, bool allowBruteForce,
            ModelRaySidedness sidedness, AZStd::span<ModelRayHit> output) const
        {
            if (!ModelKdTree::ValidateRayRequest(start, direction, sidedness, output))
            {
                return {ModelRayQueryStatus::InvalidInput};
            }
            AZStd::vector<ModelKdTree::MeshData> meshes;
            size_t primitiveCount = 0;
            const auto status = ModelKdTree::ReadRayMeshes(this, meshes, primitiveCount);
            if (status != ModelRayQueryStatus::Complete)
            {
                return {status};
            }
            if (primitiveCount > s_minimumModelTriangleCountToOptimize)
            {
                {
                    // The asynchronous builder publishes the tree under this same lock.
                    AZStd::lock_guard<AZStd::mutex> lock(m_kdTreeLock);
                    if (m_kdTree)
                    {
                        return m_kdTree->RayIntersections(start, direction, sidedness, output);
                    }
                }
                // BuildKdTree acquires m_kdTreeLock itself. Never call it while holding that lock.
                BuildKdTree();
                if (!allowBruteForce)
                {
                    return {ModelRayQueryStatus::Unavailable};
                }
            }
            return ModelKdTree::BruteForceRayIntersections(
                {meshes.data(), meshes.size()}, start, direction, sidedness, output);
        }

        ModelRayQueryResult ModelAsset::WorldVolumeIntersectionsAgainstModel(const AZ::Aabb& bounds,
            const AZ::Transform& pose, const AZ::Vector3& scale, bool allowBruteForce, AZStd::span<ModelVolumeHit> output) const
        {
            if (!ModelKdTree::ValidateVolumeRequest(bounds, pose, scale, output)) { return {ModelRayQueryStatus::InvalidInput}; }
            AZStd::vector<ModelKdTree::MeshData> meshes;
            size_t primitiveCount = 0;
            const auto status = ModelKdTree::ReadRayMeshes(this, meshes, primitiveCount);
            if (status != ModelRayQueryStatus::Complete) { return {status}; }
            if (primitiveCount > s_minimumModelTriangleCountToOptimize)
            {
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_kdTreeLock);
                    if (m_kdTree) { return m_kdTree->VolumeIntersections(bounds, pose, scale, output); }
                }
                BuildKdTree(); // Same cache/publication lock as rays; never a second spatial index.
                if (!allowBruteForce) { return {ModelRayQueryStatus::Unavailable}; }
            }
            return ModelKdTree::CollectVolumeIntersections({meshes.data(), meshes.size()}, nullptr,
                primitiveCount, bounds, pose, scale, output);
        }

        const AZStd::vector<AZ::Name>& ModelAsset::GetTags() const
        {
            return m_tags;
        }

        void ModelAsset::BuildKdTree() const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_kdTreeLock);
            if ((m_isKdTreeCalculationRunning == false) && !m_kdTree)
            {
                m_isKdTreeCalculationRunning = true;

                // ModelAsset can go away while the job is queued up or is in progress, keep it alive until the job is done
                const_cast<ModelAsset*>(this)->Acquire();

                // [GFX TODO][ATOM-4343 Bake mesh spatial information during AP processing]
                // This is a temporary workaround to enable interactive Editor experience.
                // For runtime approach is to do this during asset processing and serialized spatial information alongside with mesh model assets
                const auto jobLambda = [&]() -> void
                {
                    AZ_PROFILE_FUNCTION(RPI);

                    AZStd::unique_ptr<ModelKdTree> tree = AZStd::make_unique<ModelKdTree>();
                    tree->Build(this);

                    AZStd::lock_guard<AZStd::mutex> jobLock(m_kdTreeLock);
                    m_isKdTreeCalculationRunning = false;
                    m_kdTree = AZStd::move(tree);

                    const_cast<ModelAsset*>(this)->Release();
                };

                AZ::Job* executeGroupJob = aznew AZ::JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
                executeGroupJob->Start();
            }
        }

        bool ModelAsset::BruteForceRayIntersect(
            const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal) const
        {
            AZStd::optional<ModelMaterialSlot::StableId> materialSlotId;
            return BruteForceRayIntersect(rayStart, rayDir, distanceNormalized, normal, materialSlotId);
        }

        bool ModelAsset::BruteForceRayIntersect(
            const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal,
            AZStd::optional<ModelMaterialSlot::StableId>& materialSlotId) const
        {
            materialSlotId.reset();
            // brute force - check every triangle
            if (GetLodAssets().empty() == false)
            {
                // intersect against the highest level of detail
                if (ModelLodAsset* loadAssetPtr = GetLodAssets()[0].Get())
                {
                    bool anyHit = false;
                    AZ::Vector3 intersectionNormal;
                    float shortestDistanceNormalized = AZStd::numeric_limits<float>::max();
                    for (const ModelLodAsset::Mesh& mesh : loadAssetPtr->GetMeshes())
                    {
                        float currentDistanceNormalized;
                        if (LocalRayIntersectionAgainstMesh(mesh, rayStart, rayDir, currentDistanceNormalized, intersectionNormal))
                        {
                            anyHit = true;

                            if (currentDistanceNormalized < shortestDistanceNormalized)
                            {
                                normal = intersectionNormal;
                                shortestDistanceNormalized = currentDistanceNormalized;
                                materialSlotId.reset();
                                if (mesh.GetMaterialSlotId() != ModelMaterialSlot::InvalidStableId)
                                {
                                    materialSlotId = mesh.GetMaterialSlotId();
                                }
                            }
                        }
                    }

                    if (anyHit)
                    {
                        distanceNormalized = shortestDistanceNormalized;
                    }

                    return anyHit;
                }
            }

            return false;
        }

        bool ModelAsset::LocalRayIntersectionAgainstMesh(
            const ModelLodAsset::Mesh& mesh,
            const AZ::Vector3& rayStart,
            const AZ::Vector3& rayDir,
            float& distanceNormalized,
            AZ::Vector3& normal) const
        {
            const BufferAssetView& indexBufferView = mesh.GetIndexBufferAssetView();
            const BufferAssetView* positionBufferView = mesh.GetSemanticBufferAssetView(m_positionName);

            if (positionBufferView && positionBufferView->GetBufferAsset().Get())
            {
                BufferAsset* bufferAssetViewPtr = positionBufferView->GetBufferAsset().Get();
                BufferAsset* indexAssetViewPtr = indexBufferView.GetBufferAsset().Get();

                if (!bufferAssetViewPtr || !indexAssetViewPtr)
                {
                    return false;
                }

                AZ::RHI::BufferViewDescriptor positionBufferViewDesc = positionBufferView->GetBufferViewDescriptor();
                AZStd::span<const uint8_t> positionRawBuffer = bufferAssetViewPtr->GetBuffer();

                const uint32_t positionElementSize = positionBufferViewDesc.m_elementSize;
                const uint32_t positionElementCount = positionBufferViewDesc.m_elementCount;

                // Position is 3 floats
                if (positionElementSize != sizeof(float) * 3)
                {
                    AZ_Warning(
                        "ModelAsset", false, "unsupported mesh posiiton format, only full 3 floats per vertex are supported at the moment");
                    return false;
                }

                AZ::RHI::BufferViewDescriptor indexBufferViewDesc = indexBufferView.GetBufferViewDescriptor();
                AZStd::span<const uint8_t> indexRawBuffer = indexAssetViewPtr->GetBuffer();

                const AZ::Vector3 rayEnd = rayStart + rayDir;
                AZ::Vector3 a, b, c;
                AZ::Vector3 intersectionNormal;

                bool anyHit = false;
                float shortestDistanceNormalized = AZStd::numeric_limits<float>::max();

                const AZ::u32* indexPtr = reinterpret_cast<const AZ::u32*>(
                    indexRawBuffer.data() + (size_t{indexBufferViewDesc.m_elementOffset} * indexBufferViewDesc.m_elementSize));
                const float* positionPtr = reinterpret_cast<const float*>(
                    positionRawBuffer.data() + (size_t{positionBufferViewDesc.m_elementOffset} * positionBufferViewDesc.m_elementSize));

                AZ::Intersect::SegmentTriangleHitTester hitTester(rayStart, rayEnd);

                constexpr int StepSize = 3; // number of values per vertex (x, y, z)
                for (uint32_t indexIter = 0; indexIter < indexBufferViewDesc.m_elementCount; indexIter += StepSize, indexPtr += StepSize)
                {
                    AZ::u32 index0 = indexPtr[0];
                    AZ::u32 index1 = indexPtr[1];
                    AZ::u32 index2 = indexPtr[2];

                    if (index0 >= positionElementCount || index1 >= positionElementCount || index2 >= positionElementCount)
                    {
                        AZ_Warning("ModelAsset", false, "mesh has a bad vertex index");
                        return false;
                    }

                    // faster than AZ::Vector3 c-tor
                    const float* aRef = &positionPtr[size_t{index0} * StepSize];
                    a.Set(aRef);
                    const float* bRef = &positionPtr[size_t{index1} * StepSize];
                    b.Set(bRef);
                    const float* cRef = &positionPtr[size_t{index2} * StepSize];
                    c.Set(cRef);

                    float currentDistanceNormalized;
                    if (hitTester.IntersectSegmentTriangleCCW(a, b, c, intersectionNormal, currentDistanceNormalized))
                    {
                        anyHit = true;

                        if (currentDistanceNormalized < shortestDistanceNormalized)
                        {
                            normal = intersectionNormal;
                            shortestDistanceNormalized = currentDistanceNormalized;
                        }
                    }
                }

                if (anyHit)
                {
                    distanceNormalized = shortestDistanceNormalized;
                }

                return anyHit;
            }

            return false;
        }

        AZStd::size_t ModelAsset::CalculateTriangleCount() const
        {
            AZStd::size_t modelTriangleCount = 0;

            if (GetLodAssets().empty() == false)
            {
                if (ModelLodAsset* loadAssetPtr = GetLodAssets()[0].Get())
                {
                    for (const ModelLodAsset::Mesh& mesh : loadAssetPtr->GetMeshes())
                    {
                        const AZStd::span<const ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferList = mesh.GetStreamBufferInfoList();

                        // find position semantic
                        const ModelLodAsset::Mesh::StreamBufferInfo* positionBuffer = nullptr;

                        for (const ModelLodAsset::Mesh::StreamBufferInfo& bufferInfo : streamBufferList)
                        {
                            if (bufferInfo.m_semantic.m_name == m_positionName)
                            {
                                positionBuffer = &bufferInfo;
                                break;
                            }
                        }

                        if (positionBuffer)
                        {
                            const AZ::RHI::BufferViewDescriptor& desc = positionBuffer->m_bufferAssetView.GetBufferViewDescriptor();
                            modelTriangleCount += desc.m_elementCount / 3;
                        }
                    }
                }
            }

            AZ_Warning("Model", modelTriangleCount < ((2<<23) / 3), "Model has too many vertices for the spatial optimization. Currently only up to 16,777,216 is supported");

            return modelTriangleCount;
        }

        void ModelAsset::InitData(
            AZ::Name name,
            AZStd::span<AZ::Data::Asset<ModelLodAsset>> lodAssets,
            const ModelMaterialSlotMap& materialSlots,
            const ModelMaterialSlot& fallbackSlot,
            AZStd::span<AZ::Name> tags)
        {
            AZ_Assert(!m_isKdTreeCalculationRunning, "Overwriting a ModelAsset while it is calculating its kd tree.");

            // Clear out the current AABB, we'll reset it with the data from the LOD assets.
            m_aabb = AZ::Aabb::CreateNull();

            // Copy the trivially-copyable data.
            m_name = name;
            m_materialSlots = materialSlots;
            m_fallbackSlot = fallbackSlot;

            // Clear out the runtime-calculated data.
            m_kdTree = {};
            m_isKdTreeCalculationRunning = false;
            m_modelTriangleCount = {};

            // Clear out tags and LOD Assets, we'll set those individually.
            m_lodAssets.clear();
            m_tags = {};

            for (const auto& lodAsset : lodAssets)
            {
                m_lodAssets.push_back(lodAsset);
                if (lodAsset.IsReady())
                {
                    m_aabb.AddAabb(lodAsset->GetAabb());
                }
            }

            for (const auto& tag : tags)
            {
                m_tags.push_back(tag);
            }
        }

        // Create a stable ID for our default fallback model.
        const AZ::Data::AssetId ModelAssetHandler::s_defaultModelAssetId{ "{D676DD3C-0560-4F39-99E0-B6DCBC7CEDAA}", 0 };

        AZ::Data::AssetHandler::LoadResult ModelAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            // If there's a 0-length stream, this must be trying to load our default fallback model.
            // Fill in the asset data with a generated unit X-shaped model.
            // We need to generate the data instead of load a fallback asset because model assets have dependencies
            // on buffer and material assets, and fallback assets need to not have any dependencies to be able to load
            // correctly when used as fallbacks. (The AssetManager doesn't currently support handling dependency pre-loading
            // for fallback assets)
            if (stream->GetLength() == 0)
            {
                auto assetData = asset.GetAs<JE::Render::RPI::ModelAsset>();
                if (assetData)
                {
                    ModelAssetHelpers::CreateUnitX(assetData);
                }

                return AZ::Data::AssetHandler::LoadResult::LoadComplete;
            }

            return AssetHandler::LoadAssetData(asset, stream, assetLoadFilterCB);
        }

        AZ::Data::AssetId ModelAssetHandler::AssetMissingInCatalog(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            AZ_Info(
                "Model",
                "Model id " AZ_STRING_FORMAT " not found in asset catalog, using fallback model.\n",
                AZ_STRING_ARG(asset.GetId().ToFixedString()));

            // escalate the asset to the top of the processing list, if it can be (this is a fire and forget message that puts it in the queue
            // and returns instantly, without waiting). 
            AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetByUuid, asset.GetId().m_guid);

            // Make sure the default model asset has an entry in the asset catalog so that the asset system will try to load it.
            // Note that we specifically give it a 0-byte size and a non-empty path so that the load will just trivially succeed
            // with a 0-byte asset stream. This will enable us to detect it in LoadAssetData and fill in the data with a generated
            // unit cube. We can't use an on-disk model asset because the AssetMissing system currently doesn't correctly handle
            // assets with dependent assets (like azmodel), so we need to just load an "empty" asset and then fill it in with data
            // in LoadAssetData.
            {
                AZ::Data::AssetInfo assetInfo;
                assetInfo.m_assetId = s_defaultModelAssetId;
                assetInfo.m_assetType = azrtti_typeid<JE::Render::RPI::ModelAsset>();
                assetInfo.m_relativePath = "default_fallback_model";
                assetInfo.m_sizeBytes = 0;
                AZ::Data::AssetCatalogRequestBus::Broadcast(
                    &AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, assetInfo.m_assetId, assetInfo);
            }

            return s_defaultModelAssetId;
        }


    } // namespace RPI
} // namespace AZ
