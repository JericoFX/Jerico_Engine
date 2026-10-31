// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/limits.h>
#include <cstring>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <JE/Geometry/SurfaceData/SurfaceDataSystemRequestBus.h>
#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/Buffer/BufferAssetCreator.h>
#include <JE/Render/RPI/Reflect/Model/ModelAssetCreator.h>
#include <JE/Render/RPI/Reflect/Model/ModelLodAssetCreator.h>
#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>
#include <SurfaceData/SurfaceDataMeshComponent.h>

namespace UnitTest
{
    namespace SD = JE::Geometry::SurfaceData;
    namespace RPI = JE::Render::RPI;
    namespace Scene = JE::Render::Scene;

    // Only the renderer's asset/bounds/notification boundary is supplied here. Intersections,
    // transform components, asset creators, provider, registry and system aggregation are real.
    // No renderer, GPU, filesystem loading or imported face identity is accredited; revisions validate real CPU assets.
    class CpuMeshSource final : public AZ::Component, private Scene::MeshComponentRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CpuMeshSource, "{1102DF48-F1E3-4FC7-BF79-5C054CC5C431}");
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            { serialize->Class<CpuMeshSource, AZ::Component>()->Version(0); }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        { services.push_back(AZ_CRC_CE("MeshService")); }
        void Activate() override { Scene::MeshComponentRequestBus::Handler::BusConnect(GetEntityId()); }
        void Deactivate() override { Scene::MeshComponentRequestBus::Handler::BusDisconnect(); }
        void SetModelAsset(AZ::Data::Asset<RPI::ModelAsset> asset) override
        {
            if (m_asset.Get())
            { Scene::MeshComponentNotificationBus::Event(GetEntityId(), &Scene::MeshComponentNotifications::OnModelPreDestroy); }
            m_asset = AZStd::move(asset);
            if (m_asset.IsReady())
            { Scene::MeshComponentNotificationBus::Event(GetEntityId(), &Scene::MeshComponentNotifications::OnModelReady, m_asset, AZ::Data::Instance<RPI::Model>{}); }
        }
        AZ::Data::Asset<const RPI::ModelAsset> GetModelAsset() const override { return m_asset; }
        void SetModelAssetId(AZ::Data::AssetId) override {}
        AZ::Data::AssetId GetModelAssetId() const override { return m_asset.GetId(); }
        void SetModelAssetPath(const AZStd::string&) override {}
        AZStd::string GetModelAssetPath() const override { return {}; }
        AZ::Data::Instance<RPI::Model> GetModel() const override { return {}; }
        void SetSortKey(AZ::RHI::DrawItemSortKey) override {}
        AZ::RHI::DrawItemSortKey GetSortKey() const override { return 0; }
        void SetIsAlwaysDynamic(bool) override {}
        bool GetIsAlwaysDynamic() const override { return false; }
        void SetLodType(RPI::Cullable::LodType) override {}
        RPI::Cullable::LodType GetLodType() const override { return RPI::Cullable::LodType{}; }
        void SetLodOverride(RPI::Cullable::LodOverride) override {}
        RPI::Cullable::LodOverride GetLodOverride() const override { return {}; }
        void SetMinimumScreenCoverage(float) override {}
        float GetMinimumScreenCoverage() const override { return 0; }
        void SetQualityDecayRate(float) override {}
        float GetQualityDecayRate() const override { return 0; }
        void SetVisibility(bool) override {}
        bool GetVisibility() const override { return true; }
        void SetRayTracingEnabled(bool) override {}
        bool GetRayTracingEnabled() const override { return false; }
        void SetExcludeFromReflectionCubeMaps(bool) override {}
        bool GetExcludeFromReflectionCubeMaps() const override { return false; }
        AZ::Aabb GetLocalBounds() const override
        {
            if (!m_asset.IsReady()) { return AZ::Aabb::CreateNull(); }
            auto bounds = m_asset->GetAabb();
            AZ::Vector3 scale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(scale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
            bounds.MultiplyByScale(scale);
            return bounds;
        }
        AZ::Aabb GetWorldBounds() const override
        {
            auto bounds = GetLocalBounds();
            if (m_onBoundsRead)
            {
                auto callback = AZStd::move(m_onBoundsRead);
                m_onBoundsRead = {};
                callback(); // Renderer-side notification interleaved with the actual snapshot capture, not a fake query result.
            }
            if (!bounds.IsValid()) { return bounds; }
            AZ::Transform world = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(world, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            return bounds.GetTransformedAabb(world);
        }
        AZ::Data::Asset<RPI::ModelAsset> m_asset;
        mutable AZStd::function<void()> m_onBoundsRead;
    };

    // Deterministic interleaving after the real provider query; never supplies or rewrites geometry/results.
    class BatchQueryInterleaveProvider : public Scene::SurfaceDataMeshComponent
    {
    public:
        AZ_COMPONENT(BatchQueryInterleaveProvider, "{9CF625D5-14BF-4F1A-BDE3-53FE34A1F791}", Scene::SurfaceDataMeshComponent);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            { serialize->Class<BatchQueryInterleaveProvider, Scene::SurfaceDataMeshComponent>()->Version(0); }
        }
        SD::SurfaceRayQueryResult QueryRay(const SD::SurfaceRayRequest& request, AZStd::span<SD::SurfaceRayHit> output) const override
        {
            ++m_queryCount;
            const auto result = Scene::SurfaceDataMeshComponent::QueryRay(request, output);
            if (m_afterQuery)
            {
                auto callback = AZStd::move(m_afterQuery);
                m_afterQuery = {};
                callback();
            }
            return result;
        }
        mutable AZStd::function<void()> m_afterQuery;
        mutable size_t m_queryCount = 0;
    };

    class SurfaceDataMeshQueryFixture : public ToolsApplicationFixture<false>
    {
    protected:
        struct Triangle
        {
            AZStd::array<AZ::Vector3, 3> m_vertices;
            RPI::ModelMaterialSlot::StableId m_slot;
        };
        static Triangle Plane(float z, RPI::ModelMaterialSlot::StableId slot)
        { return {{AZ::Vector3(-2, -2, z), AZ::Vector3(2, -2, z), AZ::Vector3(0, 2, z)}, slot}; }

        template<class T> void EnsureReflected()
        {
            auto* context = GetApplication()->GetSerializeContext();
            if (!context->FindClassData(azrtti_typeid<T>()))
            {
                T::Reflect(context);
                m_reflectionRemoval.push_back([](AZ::SerializeContext* c) { T::Reflect(c); });
            }
        }
        template<class T> void EnsureDescriptor()
        {
            auto* descriptor = AZ::ComponentDescriptorBus::FindFirstHandler(azrtti_typeid<T>());
            if (!descriptor)
            {
                m_descriptors.emplace_back(T::CreateDescriptor());
                descriptor = m_descriptors.back().get();
            }
            auto* context = GetApplication()->GetSerializeContext();
            if (!context->FindClassData(azrtti_typeid<T>()))
            {
                descriptor->Reflect(context);
                m_reflectionRemoval.push_back([descriptor](AZ::SerializeContext* c) { descriptor->Reflect(c); });
            }
        }
        template<class T> void EnsureHandler()
        {
            EnsureReflected<T>();
            if (!AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<T>()))
            {
                // ToolsApplication owns the catalogs; this fixture only supplies missing CPU asset handlers.
                auto handler = AZStd::make_unique<RPI::AssetHandler<T>>();
                AZ::Data::AssetManager::Instance().RegisterHandler(handler.get(), azrtti_typeid<T>());
                m_handlers.push_back(AZStd::move(handler));
            }
        }
        void SetUpEditorFixtureImpl() override
        {
            EnsureHandler<RPI::BufferAsset>();
            EnsureHandler<RPI::ModelLodAsset>();
            EnsureHandler<RPI::ModelAsset>();
            EnsureDescriptor<SD::SurfaceDataSystemComponent>();
            EnsureDescriptor<Scene::SurfaceDataMeshComponent>();
            EnsureDescriptor<CpuMeshSource>();
            EnsureDescriptor<AzFramework::TransformComponent>();
            EnsureDescriptor<AzFramework::NonUniformScaleComponent>();
            if (!AZ::Interface<SD::SurfaceDataSystem>::Get())
            {
                m_systemEntity = AZStd::make_unique<AZ::Entity>("Real SurfaceData query system");
                m_systemEntity->CreateComponent<SD::SurfaceDataSystemComponent>();
                m_systemEntity->Init(); m_systemEntity->Activate();
            }
            m_system = AZ::Interface<SD::SurfaceDataSystem>::Get();
            ASSERT_NE(m_system, nullptr);
        }
        void TearDownEditorFixtureImpl() override
        {
            m_entity.reset();
            m_systemEntity.reset();
            m_models.clear();
            AZ::Data::AssetManager::Instance().DispatchEvents();
            m_handlers.clear(); // Only handlers created by this fixture; external registrations are never replaced.
            auto* context = GetApplication()->GetSerializeContext();
            context->EnableRemoveReflection();
            for (auto it = m_reflectionRemoval.rbegin(); it != m_reflectionRemoval.rend(); ++it) { (*it)(context); }
            context->DisableRemoveReflection();
            m_reflectionRemoval.clear();
            m_descriptors.clear(); // Keep descriptors alive through their reflection-removal callbacks.
        }
        AZ::Data::Asset<RPI::BufferAsset> Buffer(const void* bytes, size_t size, uint32_t count, uint32_t stride, bool noCpu = false)
        {
            AZ::RHI::BufferDescriptor descriptor;
            descriptor.m_byteCount = size;
            descriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly;
            RPI::BufferAssetCreator creator;
            creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetUseCommonPool(RPI::CommonBufferPoolType::StaticInputAssembly);
            creator.SetBuffer(noCpu ? nullptr : bytes, noCpu ? 0 : size, descriptor);
            creator.SetBufferViewDescriptor(AZ::RHI::BufferViewDescriptor::CreateStructured(0, count, stride));
            AZ::Data::Asset<RPI::BufferAsset> result;
            EXPECT_TRUE(creator.End(result));
            return result;
        }
        AZ::Data::Asset<RPI::ModelAsset> Model(AZStd::initializer_list<Triangle> triangles, bool noCpu = false, bool withUv = false, float uv = 0.25f, const RPI::ModelSurfaceIdentityTable& identity = {})
        {
            RPI::ModelLodAssetCreator lod;
            lod.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            for (const auto& triangle : triangles)
            {
                AZStd::array<float, 9> positions;
                auto bounds = AZ::Aabb::CreateNull();
                for (size_t i = 0; i < 3; ++i)
                {
                    const auto& v = triangle.m_vertices[i]; bounds.AddPoint(v);
                    positions[i * 3] = v.GetX(); positions[i * 3 + 1] = v.GetY(); positions[i * 3 + 2] = v.GetZ();
                }
                const AZStd::array<uint32_t, 3> indices{0, 1, 2};
                auto vertexBuffer = Buffer(positions.data(), sizeof(positions), 3, sizeof(float) * 3);
                auto indexBuffer = Buffer(indices.data(), sizeof(indices), 3, sizeof(uint32_t), noCpu);
                lod.BeginMesh(); lod.SetMeshAabb(bounds); lod.SetMeshMaterialSlot(triangle.m_slot);
                lod.SetMeshSurfaceIdentityTable(identity);
                lod.SetMeshIndexBuffer({indexBuffer, AZ::RHI::BufferViewDescriptor::CreateStructured(0, 3, sizeof(uint32_t))});
                lod.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic(AZ::Name("POSITION")), AZ::Name(),
                    {vertexBuffer, AZ::RHI::BufferViewDescriptor::CreateStructured(0, 3, sizeof(float) * 3)});
                if (withUv)
                {
                    const AZStd::array<float, 6> values{{0, 0, 1, 0, uv, 1}};
                    auto uvBuffer = Buffer(values.data(), sizeof(values), 3, sizeof(float) * 2);
                    lod.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic(AZ::Name("UV")), AZ::Name(),
                        {uvBuffer, AZ::RHI::BufferViewDescriptor::CreateTyped(0, 3, AZ::RHI::Format::R32G32_FLOAT)});
                }
                lod.EndMesh();
            }
            AZ::Data::Asset<RPI::ModelLodAsset> lodAsset;
            EXPECT_TRUE(lod.End(lodAsset));
            RPI::ModelAssetCreator creator;
            creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom())); creator.SetName("Real CPU provider model");
            creator.AddLodAsset(AZStd::move(lodAsset));
            AZ::Data::Asset<RPI::ModelAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            m_models.push_back(asset);
            return asset;
        }
        static SD::SurfaceSourceRevision SourceRevision(const RPI::ModelContentRevision& modelRevision)
        {
            SD::SurfaceSourceRevision result;
            result.m_schema = modelRevision.m_schema; result.m_digest = modelRevision.m_digest;
            return result;
        }
        void DeserializeModelSnapshot(const AZ::Data::Asset<RPI::ModelAsset>& source,
            AZ::Data::Asset<RPI::ModelAsset>& target, bool legacy = false)
        {
            ASSERT_TRUE(target.IsReady()); ASSERT_NE(target.Get(), nullptr);
            const auto id = target.GetId(); const auto* pointer = target.Get();
            bool ownedSource = false;
            for (const auto& model : m_models) { ownedSource |= model.Get() == source.Get() && model.GetId() == source.GetId(); }
            ASSERT_TRUE(ownedSource); ASSERT_TRUE(source.IsReady());
            for (const auto& lod : source->GetLodAssets())
            {
                ASSERT_TRUE(lod.IsReady()); ASSERT_NE(lod.Get(), nullptr); ASSERT_EQ(lod.GetId(), lod->GetId());
                for (const auto& mesh : lod->GetMeshes())
                {
                    const auto& indices = mesh.GetIndexBufferAssetView().GetBufferAsset();
                    ASSERT_TRUE(indices.IsReady()); ASSERT_EQ(indices.GetId(), indices->GetId());
                    for (const auto& stream : mesh.GetStreamBufferInfoList())
                    {
                        const auto& buffer = stream.m_bufferAssetView.GetBufferAsset();
                        ASSERT_TRUE(buffer.IsReady()); ASSERT_EQ(buffer.GetId(), buffer->GetId());
                    }
                }
            }
            AZStd::vector<char> bytes;
            AZ::IO::ByteContainerStream<AZStd::vector<char>> output(&bytes);
            ASSERT_TRUE(AZ::Utils::SaveObjectToStream(output,
                legacy ? AZ::DataStream::ST_XML : AZ::DataStream::ST_BINARY, source.Get(), GetApplication()->GetSerializeContext()));
            if (legacy)
            {
                // Remove the claim from an actual serialized legacy field set, not from an in-place reference patch.
                bytes.push_back(0);
                AZ::rapidxml::xml_document<> document; ASSERT_TRUE(document.parse<0>(bytes.data()));
                auto* root = document.first_node("ObjectStream"); ASSERT_NE(root, nullptr);
                auto* model = root->first_node("Class"); ASSERT_NE(model, nullptr);
                const auto* type = model->first_attribute("type"); ASSERT_NE(type, nullptr);
                ASSERT_EQ(AZ::Uuid(type->value()), azrtti_typeid<RPI::ModelAsset>());
                auto* version = model->first_attribute("version"); ASSERT_NE(version, nullptr);
                version->value("1", 1);
                bool removed = false;
                for (auto* field = model->first_node("Class"); field; field = field->next_sibling("Class"))
                {
                    const auto* name = field->first_attribute("field");
                    if (name && AZStd::string_view(name->value(), name->value_size()) == "ContentRevision")
                    { model->remove_node(field); removed = true; break; }
                }
                ASSERT_TRUE(removed);
                AZStd::string xml; AZ::rapidxml::print(AZStd::back_inserter(xml), document, 0);
                bytes.assign(xml.begin(), xml.end());
            }
            size_t references = 0; bool ownedReferences = true;
            AZ::ObjectStream::FilterDescriptor filter([&](const AZ::Data::AssetFilterInfo& info)
            {
                ++references; bool owned = false;
                const auto resident = AZ::Data::AssetManager::Instance().FindAsset(info.m_assetId, AZ::Data::AssetLoadBehavior::NoLoad);
                for (const auto& lod : source->GetLodAssets())
                { owned |= info.m_assetId == lod.GetId() && resident.Get() == lod.Get() && resident.IsReady(); }
                ownedReferences &= owned;
                return AZ::Data::AssetFilterNoAssetLoading(info); // FindAsset only; never QueueLoad or external IO.
            }, AZ::ObjectStream::FILTERFLAG_STRICT);
            AZ::IO::MemoryStream input(static_cast<const void*>(bytes.data()), bytes.size());
            ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(input, *target.Get(), GetApplication()->GetSerializeContext(), filter));
            ASSERT_TRUE(ownedReferences); ASSERT_EQ(references, source->GetLodAssets().size());
            ASSERT_EQ(target.GetId(), id); ASSERT_EQ(target.Get(), pointer);
            ASSERT_EQ(target->GetLodAssets().size(), source->GetLodAssets().size());
            for (size_t i = 0; i != source->GetLodAssets().size(); ++i)
            {
                const auto& actual = target->GetLodAssets()[i]; const auto& expected = source->GetLodAssets()[i];
                ASSERT_EQ(actual.GetId(), expected.GetId()); ASSERT_EQ(actual.Get(), expected.Get());
                ASSERT_TRUE(actual.IsReady()); ASSERT_EQ(actual.GetId(), actual->GetId());
            }
            if (legacy) { ASSERT_EQ(target->GetContentRevision().m_schema, 0); ASSERT_TRUE(target->ComputeContentRevision()); }
            else { ASSERT_EQ(target->GetContentRevision(), source->GetContentRevision()); ASSERT_TRUE(target->ValidateContentRevision()); }
        }
        void ReloadCurrentModelFrom(const AZ::Data::Asset<RPI::ModelAsset>& replacement)
        {
            Scene::MeshComponentNotificationBus::Event(m_entity->GetId(), &Scene::MeshComponentNotifications::OnModelPreDestroy);
            ASSERT_NO_FATAL_FAILURE(DeserializeModelSnapshot(replacement, m_source->m_asset));
            Scene::MeshComponentNotificationBus::Event(m_entity->GetId(), &Scene::MeshComponentNotifications::OnModelReady,
                m_source->m_asset, AZ::Data::Instance<RPI::Model>{});
        }
        void Open(AZ::Data::Asset<RPI::ModelAsset> model)
        {
            m_entity = AZStd::make_unique<AZ::Entity>("Real imported-model provider");
            m_entity->CreateComponent<AzFramework::TransformComponent>();
            m_entity->CreateComponent<AzFramework::NonUniformScaleComponent>();
            m_source = m_entity->CreateComponent<CpuMeshSource>();
            m_source->m_asset = AZStd::move(model);
            m_provider = m_entity->CreateComponent<Scene::SurfaceDataMeshComponent>();
            m_entity->Init(); m_entity->Activate();
        }
        template<class Provider = Scene::SurfaceDataMeshComponent>
        AZStd::unique_ptr<AZ::Entity> BatchEntity(AZ::Data::Asset<RPI::ModelAsset> model, const AZ::Vector3& translation = AZ::Vector3::CreateZero())
        {
            EnsureDescriptor<Provider>();
            auto entity = AZStd::make_unique<AZ::Entity>("Real batch imported-model provider");
            entity->CreateComponent<AzFramework::TransformComponent>();
            entity->CreateComponent<AzFramework::NonUniformScaleComponent>();
            entity->CreateComponent<CpuMeshSource>()->m_asset = AZStd::move(model);
            entity->CreateComponent<Provider>();
            entity->Init(); entity->Activate();
            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation(translation));
            return entity;
        }
        static SD::SurfaceRayBatchProbe BatchProbe(const AZ::Vector3& start, const AZ::Vector3& end,
            size_t offset, size_t capacity, AZ::EntityComponentIdPair owner = {AZ::EntityId(), AZ::InvalidComponentId})
        {
            SD::SurfaceRayBatchProbe probe;
            probe.m_ray.m_startWorldPosition = start; probe.m_ray.m_endWorldPosition = end;
            probe.m_ray.m_sidedness = SD::SurfaceRaySidedness::FrontFace;
            if (owner.GetEntityId().IsValid())
            { probe.m_ray.m_filter = SD::SurfaceRayQueryFilter::ExactOwner; probe.m_ray.m_owner = owner; }
            probe.m_hitOffset = offset; probe.m_hitCapacity = capacity;
            return probe;
        }
        static SD::SurfaceRayBatchLimits BatchLimits(size_t rays, size_t visits, size_t hits)
        {
            SD::SurfaceRayBatchLimits limits;
            limits.m_maxRayCount = rays; limits.m_maxProviderVisits = visits; limits.m_maxTotalHits = hits;
            return limits;
        }
        void Tick()
        { AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.0f, AZ::ScriptTimePoint{}); }
        SD::SurfaceRayRequest Ray(AZ::Vector3 start = {0, 0, 5}, AZ::Vector3 end = {0, 0, -3}) const
        {
            SD::SurfaceRayRequest request;
            request.m_startWorldPosition = start; request.m_endWorldPosition = end;
            request.m_filter = SD::SurfaceRayQueryFilter::ExactOwner;
            request.m_owner = {m_entity->GetId(), m_provider->GetId()};
            request.m_sidedness = SD::SurfaceRaySidedness::FrontFace;
            return request;
        }
        SD::SurfaceDataSystem* m_system = nullptr;
        AZStd::unique_ptr<AZ::Entity> m_entity, m_systemEntity;
        CpuMeshSource* m_source = nullptr;
        Scene::SurfaceDataMeshComponent* m_provider = nullptr;
        RPI::AssetHandlerPtrList m_handlers;
        AZStd::vector<AZStd::unique_ptr<AZ::ComponentDescriptor>> m_descriptors;
        AZStd::vector<AZStd::function<void(AZ::SerializeContext*)>> m_reflectionRemoval;
        AZStd::vector<AZ::Data::Asset<RPI::ModelAsset>> m_models;
    };

    TEST_F(SurfaceDataMeshQueryFixture, VolumeIntersectsActualTriangleInteriorAndPreservesAuthoredIdentityAcrossReflectedPose)
    {
        RPI::ModelSurfaceIdentityTable table; table.m_schema = RPI::ModelSurfaceIdentityTable::Schema;
        const AZ::Uuid surface("{10000000-0000-0000-0000-000000000001}");
        const AZ::Uuid face("{20000000-0000-0000-0000-000000000002}");
        const AZ::Uuid region("{30000000-0000-0000-0000-000000000003}");
        table.m_entries = {{surface, face, region}};
        const auto model = Model({Plane(0, 7u)}, false, true, 0.25f, table);
        Open(model);
        // Negative nonuniform axes are clamped by NonUniformScaleComponent. Use its supported positive scale
        // with a uniform reflection: -Rx(-pi/2)*diag(2,3,4) == Rx(pi/2)*diag(-2,3,4).
        auto pose = AZ::Transform::CreateRotationX(-AZ::Constants::HalfPi);
        pose.SetTranslation({10, 20, 30}); pose.SetUniformScale(-0.001f);
        AZ::NonUniformScaleRequestBus::Event(m_entity->GetId(), &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(2, 3, 4));
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, pose); Tick();
        AZ::Vector3 effectiveScale = AZ::Vector3::CreateZero();
        AZ::Transform effectivePose = AZ::Transform::CreateIdentity();
        AZ::NonUniformScaleRequestBus::EventResult(effectiveScale, m_entity->GetId(), &AZ::NonUniformScaleRequests::GetScale);
        AZ::TransformBus::EventResult(effectivePose, m_entity->GetId(), &AZ::TransformBus::Events::GetWorldTM);
        ASSERT_TRUE(effectiveScale.IsClose(AZ::Vector3(2, 3, 4)));
        ASSERT_FLOAT_EQ(effectivePose.GetUniformScale(), -0.001f);
        const auto xAxis = effectivePose.TransformVector(AZ::Vector3(effectiveScale.GetX(), 0, 0));
        const auto yAxis = effectivePose.TransformVector(AZ::Vector3(0, effectiveScale.GetY(), 0));
        const auto zAxis = effectivePose.TransformVector(AZ::Vector3(0, 0, effectiveScale.GetZ()));
        ASSERT_LT(xAxis.Dot(yAxis.Cross(zAxis)), 0.0f); // Actual entity transform is reflected, not a setter assumption.
        ASSERT_TRUE(xAxis.Cross(yAxis).GetNormalized().IsClose(AZ::Vector3::CreateAxisY()));
        SD::SurfaceVolumeRequest request;
        request.m_worldBounds = AZ::Aabb::CreateCenterHalfExtents({10, 20, 30}, {0.0001f, 0.0001f, 0.0001f});
        request.m_filter = SD::SurfaceRayQueryFilter::ExactOwner; request.m_owner = {m_entity->GetId(), m_provider->GetId()};
        SD::SurfaceVolumeLimits limits; limits.m_maxProviderVisits = 2; limits.m_maxTotalHits = 1;
        SD::SurfaceRayHit hit;
        const auto result = m_system->QueryVolume(request, {&hit, 1}, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        ASSERT_EQ(result.m_query.m_status, SD::SurfaceRayQueryStatus::Complete); ASSERT_EQ(result.m_query.m_hitCount, 1);
        EXPECT_TRUE(request.m_worldBounds.Contains(hit.m_worldPosition));
        EXPECT_TRUE(hit.m_worldNormal.IsClose(AZ::Vector3::CreateAxisY())); // Reflect X flips the rotated +Z winding.
        EXPECT_EQ(hit.m_surfaceId, surface); EXPECT_EQ(hit.m_faceId, face); ASSERT_TRUE(hit.m_regionId); EXPECT_EQ(*hit.m_regionId, region);
        EXPECT_EQ(hit.m_sourceRevision, SourceRevision(model->GetContentRevision()));
        EXPECT_EQ(hit.m_materialSlot, 7u); ASSERT_TRUE(hit.m_modelProvenance);
        EXPECT_EQ(hit.m_modelProvenance->m_modelAssetId, model.GetId());
        EXPECT_EQ(hit.m_modelProvenance->m_submeshIndex, 0u); EXPECT_EQ(hit.m_primitiveIndex, 0u);
    }

    TEST_F(SurfaceDataMeshQueryFixture, VolumeRejectsAabbOnlyContactsAndPendingSnapshotsWithoutOutputWrites)
    {
        Open(Model({Plane(0, 7u)}));
        SD::SurfaceVolumeRequest request;
        request.m_worldBounds = AZ::Aabb::CreateFromMinMax({1.7f, 1.7f, -0.2f}, {1.9f, 1.9f, 0.2f});
        request.m_filter = SD::SurfaceRayQueryFilter::ExactOwner; request.m_owner = {m_entity->GetId(), m_provider->GetId()};
        SD::SurfaceVolumeLimits limits; limits.m_maxProviderVisits = 2; limits.m_maxTotalHits = 1;
        SD::SurfaceRayHit hit; hit.m_distance = 42;
        const auto miss = m_system->QueryVolume(request, {&hit, 1}, limits);
        ASSERT_EQ(miss.m_query.m_status, SD::SurfaceRayQueryStatus::Complete); EXPECT_EQ(miss.m_query.m_hitCount, 0);
        EXPECT_FLOAT_EQ(hit.m_distance, 42);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation({10, 0, 0}));
        request.m_worldBounds = AZ::Aabb::CreateCenterHalfExtents({10, 0, 0}, {0.1f, 0.1f, 0.1f});
        EXPECT_EQ(m_system->QueryVolume(request, {&hit, 1}, limits).m_query.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_FLOAT_EQ(hit.m_distance, 42); Tick();
        ASSERT_EQ(m_system->QueryVolume(request, {&hit, 1}, limits).m_query.m_hitCount, 1);
        AZStd::atomic_bool cancel{true}; limits.m_cancellationFlag = &cancel; hit.m_distance = 42;
        EXPECT_EQ(m_system->QueryVolume(request, {&hit, 1}, limits).m_completion, SD::SurfaceRayBatchCompletion::Cancelled);
        EXPECT_FLOAT_EQ(hit.m_distance, 42);
    }

    TEST_F(SurfaceDataMeshQueryFixture, RayBatchTwoRegionsSelectRealFloorCeilingWallAndRejectPendingTransformBeforeTick)
    {
        const Triangle ceiling{{AZ::Vector3(-2, -2, 4), AZ::Vector3(0, 2, 4), AZ::Vector3(2, -2, 4)}, 11u};
        const Triangle wall{{AZ::Vector3(2, -2, 0), AZ::Vector3(2, 0, 4), AZ::Vector3(2, 2, 0)}, 13u};
        const auto model = Model({Plane(0, 7u), ceiling, wall});
        auto a = BatchEntity(model);
        auto b = BatchEntity(model, {10, 0, 0});
        Tick();
        const auto handleA = m_system->GetSurfaceDataProviderHandle(a->GetId());
        const auto handleB = m_system->GetSurfaceDataProviderHandle(b->GetId());
        ASSERT_NE(handleA, SD::InvalidSurfaceDataRegistryHandle); ASSERT_NE(handleB, handleA);
        AZStd::array<SD::SurfaceRayBatchProbe, 4> probes{{
            BatchProbe({0, 0, 2}, {0, 0, -2}, 0, 1), BatchProbe({0, 0, 2}, {0, 0, 6}, 1, 1),
            BatchProbe({0, 0, 2}, {4, 0, 2}, 2, 1), BatchProbe({10, 0, 2}, {10, 0, -2}, 3, 1)}};
        AZStd::array<SD::SurfaceRayHit, 4> hits{};
        AZStd::array<SD::SurfaceRayQueryResult, 4> perRay{};
        const auto limits = BatchLimits(probes.size(), 2 * (probes.size() + 1), hits.size());
        auto result = m_system->QueryRays(probes, hits, perRay, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Complete);
        EXPECT_EQ(result.m_committedRayCount, probes.size()); EXPECT_EQ(result.m_hitCount, hits.size());
        const AZStd::array<AZ::Vector3, 4> expectedPositions{{{0, 0, 0}, {0, 0, 4}, {2, 0, 2}, {10, 0, 0}}};
        const AZStd::array<AZ::Vector3, 4> expectedNormals{{{0, 0, 1}, {0, 0, -1}, {-1, 0, 0}, {0, 0, 1}}};
        const AZStd::array<uint32_t, 4> expectedMaterialSlots{{7, 11, 13, 7}};
        for (size_t i = 0; i != probes.size(); ++i)
        {
            ASSERT_EQ(perRay[i].m_hitCount, 1); EXPECT_EQ(perRay[i].m_status, SD::SurfaceRayQueryStatus::Complete);
            EXPECT_TRUE(hits[i].m_worldPosition.IsClose(expectedPositions[i]));
            EXPECT_TRUE(hits[i].m_worldNormal.IsClose(expectedNormals[i]));
            EXPECT_EQ(hits[i].m_entityAndComponent.GetEntityId(), i == 3 ? b->GetId() : a->GetId());
            ASSERT_TRUE(hits[i].m_materialSlot); EXPECT_EQ(*hits[i].m_materialSlot, expectedMaterialSlots[i]);
            ASSERT_TRUE(hits[i].m_modelProvenance); EXPECT_EQ(hits[i].m_modelProvenance->m_modelAssetId, model.GetId());
            EXPECT_EQ(hits[i].m_revisionState, SD::SurfaceSourceRevisionState::Content);
            EXPECT_EQ(hits[i].m_sourceRevision, SourceRevision(model->GetContentRevision()));
        }
        const auto bHit = hits[3];
        AZ::TransformBus::Event(a->GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation({20, 0, 0}));
        for (auto& hit : hits) { hit.m_distance = 42; }
        result = m_system->QueryRays(probes, hits, perRay, limits); // Dirty cache has not ticked/published.
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Partial);
        for (size_t i = 0; i != 3; ++i)
        { EXPECT_EQ(perRay[i].m_status, SD::SurfaceRayQueryStatus::Unsupported); EXPECT_EQ(perRay[i].m_hitCount, 0); EXPECT_FLOAT_EQ(hits[i].m_distance, 42); }
        // Unknown A bounds conservatively affect completeness, but B's actual geometry remains unchanged.
        EXPECT_EQ(perRay[3].m_status, SD::SurfaceRayQueryStatus::Partial);
        ASSERT_EQ(perRay[3].m_hitCount, 1); EXPECT_EQ(hits[3].m_worldPosition, bHit.m_worldPosition);
        EXPECT_EQ(hits[3].m_entityAndComponent, bHit.m_entityAndComponent);
        Tick();
        result = m_system->QueryRays(probes, hits, perRay, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Complete);
        for (size_t i = 0; i != 3; ++i) { EXPECT_EQ(perRay[i].m_hitCount, 0); }
        for (size_t i = 0; i != 3; ++i)
        { probes[i].m_ray.m_startWorldPosition += AZ::Vector3(20, 0, 0); probes[i].m_ray.m_endWorldPosition += AZ::Vector3(20, 0, 0); }
        result = m_system->QueryRays(probes, hits, perRay, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Complete); EXPECT_EQ(result.m_hitCount, 4);
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(a->GetId()), handleA);
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(b->GetId()), handleB);
    }

    TEST_F(SurfaceDataMeshQueryFixture, RayBatchPerRayZeroCapacityAndTotalHitLimitPreserveNearestAndKnownDiscardCounts)
    {
        Open(Model({Plane(3, 7u), Plane(1, 9u), Plane(-1, 11u)}));
        const AZ::EntityComponentIdPair owner{m_entity->GetId(), m_provider->GetId()};
        AZStd::array<SD::SurfaceRayBatchProbe, 3> probes{{BatchProbe({0, 0, 5}, {0, 0, -3}, 0, 2, owner),
            BatchProbe({0, 0, 5}, {0, 0, -3}, 2, 0, owner), BatchProbe({0, 0, 5}, {0, 0, -3}, 2, 2, owner)}};
        AZStd::array<SD::SurfaceRayHit, 4> hits{};
        for (auto& hit : hits) { hit.m_distance = 42; }
        AZStd::array<SD::SurfaceRayQueryResult, 3> perRay{};
        auto limits = BatchLimits(probes.size(), 1 + probes.size(), 3);
        auto result = m_system->QueryRays(probes, hits, perRay, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Truncated); EXPECT_EQ(result.m_hitCount, 3);
        EXPECT_EQ(perRay[0].m_hitCount, 2); EXPECT_EQ(perRay[0].m_discardedHitCount, 1);
        EXPECT_EQ(perRay[1].m_hitCount, 0); EXPECT_EQ(perRay[1].m_discardedHitCount, 3);
        EXPECT_EQ(perRay[2].m_hitCount, 1); EXPECT_EQ(perRay[2].m_discardedHitCount, 2);
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 3); EXPECT_FLOAT_EQ(hits[1].m_worldPosition.GetZ(), 1);
        EXPECT_FLOAT_EQ(hits[2].m_worldPosition.GetZ(), 3); EXPECT_FLOAT_EQ(hits[3].m_distance, 42);
        // Disjoint output slices may be in a different order from the probes.
        probes[0].m_hitOffset = 2; probes[1].m_hitOffset = 0; probes[2].m_hitOffset = 0;
        for (auto& hit : hits) { hit.m_distance = 42; }
        result = m_system->QueryRays(probes, hits, perRay, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_FLOAT_EQ(hits[2].m_worldPosition.GetZ(), 3); EXPECT_FLOAT_EQ(hits[3].m_worldPosition.GetZ(), 1);
        EXPECT_FLOAT_EQ(hits[0].m_worldPosition.GetZ(), 3); EXPECT_FLOAT_EQ(hits[1].m_distance, 42);
        limits.m_maxTotalHits = 0;
        for (auto& hit : hits) { hit.m_distance = 42; }
        result = m_system->QueryRays(probes, hits, perRay, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished); EXPECT_EQ(result.m_hitCount, 0);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Truncated);
        for (const auto& ray : perRay) { EXPECT_EQ(ray.m_hitCount, 0); EXPECT_EQ(ray.m_discardedHitCount, 3); }
        for (const auto& hit : hits) { EXPECT_FLOAT_EQ(hit.m_distance, 42); }
    }

    TEST_F(SurfaceDataMeshQueryFixture, RayBatchEmptyAndMissesAreCompleteWhileUnsupportedAndTruncationRemainPartial)
    {
        const auto empty = m_system->QueryRays({}, {}, {}, {});
        EXPECT_EQ(empty.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(empty.m_queryStatus, SD::SurfaceRayQueryStatus::Complete); EXPECT_EQ(empty.m_providerVisitCount, 0);
        auto a = BatchEntity(Model({Plane(3, 7u), Plane(1, 9u)}));
        auto b = BatchEntity(Model({Plane(3, 11u)}, true), {10, 0, 0}); Tick();
        AZStd::array<SD::SurfaceRayBatchProbe, 2> probes{{
            BatchProbe({0, 0, 5}, {0, 0, -3}, 0, 1, {a->GetId(), a->FindComponent<Scene::SurfaceDataMeshComponent>()->GetId()}),
            BatchProbe({10, 0, 5}, {10, 0, -3}, 1, 1, {b->GetId(), b->FindComponent<Scene::SurfaceDataMeshComponent>()->GetId()})}};
        AZStd::array<SD::SurfaceRayHit, 2> hits{}; AZStd::array<SD::SurfaceRayQueryResult, 2> perRay{};
        const auto result = m_system->QueryRays(probes, hits, perRay, BatchLimits(2, 2 + probes.size(), 2));
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Partial);
        EXPECT_EQ(perRay[0].m_status, SD::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(perRay[1].m_status, SD::SurfaceRayQueryStatus::Unsupported);
        auto miss = BatchProbe({100, 0, 5}, {100, 0, -3}, 0, 0);
        SD::SurfaceRayQueryResult missResult;
        const auto noHit = m_system->QueryRays({&miss, 1}, {}, {&missResult, 1}, BatchLimits(1, 4, 0));
        EXPECT_EQ(noHit.m_providerVisitCount, 4); // Two registry entries plus two bounds-rejected candidate visits.
        EXPECT_EQ(noHit.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(noHit.m_queryStatus, SD::SurfaceRayQueryStatus::Complete); EXPECT_EQ(missResult.m_hitCount, 0);
    }

    TEST_F(SurfaceDataMeshQueryFixture, RayBatchCancellationAfterRealQueryAndProviderBudgetAbortWithoutCommitting)
    {
        auto a = BatchEntity<BatchQueryInterleaveProvider>(Model({Plane(3, 7u)}));
        auto b = BatchEntity(Model({Plane(3, 9u)}), {10, 0, 0}); Tick();
        auto* hook = a->FindComponent<BatchQueryInterleaveProvider>(); ASSERT_NE(hook, nullptr);
        AZStd::array<SD::SurfaceRayBatchProbe, 2> probes{{
            BatchProbe({0, 0, 5}, {0, 0, -3}, 0, 1, {a->GetId(), hook->GetId()}),
            BatchProbe({10, 0, 5}, {10, 0, -3}, 1, 1, {b->GetId(), b->FindComponent<Scene::SurfaceDataMeshComponent>()->GetId()})}};
        AZStd::array<SD::SurfaceRayHit, 2> hits{}; AZStd::array<SD::SurfaceRayQueryResult, 2> perRay{};
        for (auto& hit : hits) { hit.m_distance = 42; }
        for (auto& ray : perRay) { ray.m_hitCount = 77; ray.m_status = SD::SurfaceRayQueryStatus::Partial; }
        AZStd::atomic_bool cancel{false};
        hook->m_afterQuery = [&cancel] { cancel.store(true); };
        auto limits = BatchLimits(2, 2 + probes.size(), 2); limits.m_cancellationFlag = &cancel;
        auto result = m_system->QueryRays(probes, hits, perRay, limits);
        EXPECT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Cancelled);
        EXPECT_EQ(result.m_committedRayCount, 0); EXPECT_EQ(result.m_hitCount, 0); EXPECT_EQ(hook->m_queryCount, 1);
        for (const auto& hit : hits) { EXPECT_FLOAT_EQ(hit.m_distance, 42); }
        for (const auto& ray : perRay) { EXPECT_EQ(ray.m_hitCount, 77); EXPECT_EQ(ray.m_status, SD::SurfaceRayQueryStatus::Partial); }
        cancel.store(false); hook->m_queryCount = 0;
        limits.m_maxProviderVisits = 3; // Two registry entries, then only A's real query fits.
        result = m_system->QueryRays(probes, hits, perRay, limits);
        EXPECT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::BudgetExhausted);
        EXPECT_EQ(result.m_providerVisitCount, 3); EXPECT_EQ(hook->m_queryCount, 1);
        EXPECT_EQ(result.m_committedRayCount, 0); EXPECT_EQ(result.m_hitCount, 0);
        for (const auto& hit : hits) { EXPECT_FLOAT_EQ(hit.m_distance, 42); }
        for (const auto& ray : perRay) { EXPECT_EQ(ray.m_hitCount, 77); }
    }

    TEST_F(SurfaceDataMeshQueryFixture, RayBatchDoesNotHideAProviderThatBecomesPendingOutsideTheInitialRayRegion)
    {
        auto a = BatchEntity<BatchQueryInterleaveProvider>(Model({Plane(3, 7u)}));
        auto b = BatchEntity(Model({Plane(3, 9u)}), {100, 0, 0}); Tick();
        auto* hook = a->FindComponent<BatchQueryInterleaveProvider>(); ASSERT_NE(hook, nullptr);
        const auto bId = b->GetId();
        hook->m_afterQuery = [bId]
        {
            AZ::TransformBus::Event(bId, &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation({10, 0, 0}));
        };
        AZStd::array<SD::SurfaceRayBatchProbe, 2> probes{{
            BatchProbe({0, 0, 5}, {0, 0, -3}, 0, 1, {a->GetId(), hook->GetId()}),
            BatchProbe({10, 0, 5}, {10, 0, -3}, 1, 1)}};
        AZStd::array<SD::SurfaceRayHit, 2> hits{}; hits[1].m_distance = 42;
        AZStd::array<SD::SurfaceRayQueryResult, 2> perRay{};
        const auto limits = BatchLimits(2, 2 + 1 + 2, 2); // Registry scan, A owner visit, then both current bounds.
        auto result = m_system->QueryRays(probes, hits, perRay, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Partial);
        ASSERT_EQ(perRay[0].m_hitCount, 1); EXPECT_EQ(hits[0].m_entityAndComponent.GetEntityId(), a->GetId());
        EXPECT_EQ(perRay[1].m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(perRay[1].m_hitCount, 0); EXPECT_FLOAT_EQ(hits[1].m_distance, 42);
        Tick();
        result = m_system->QueryRays(probes, hits, perRay, limits);
        ASSERT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::Finished);
        EXPECT_EQ(result.m_queryStatus, SD::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(perRay[1].m_hitCount, 1); EXPECT_EQ(hits[1].m_entityAndComponent.GetEntityId(), bId);
        EXPECT_TRUE(hits[1].m_worldPosition.IsClose(AZ::Vector3(10, 0, 3)));
    }

    TEST_F(SurfaceDataMeshQueryFixture, RayBatchLateMalformedGeometryAbortsEarlierStagedRealHits)
    {
        auto a = BatchEntity<BatchQueryInterleaveProvider>(Model({Plane(3, 7u)}));
        const auto laterAsset = Model({Plane(3, 9u)});
        auto b = BatchEntity(laterAsset, {10, 0, 0}); Tick();
        auto* hook = a->FindComponent<BatchQueryInterleaveProvider>(); ASSERT_NE(hook, nullptr);
        hook->m_afterQuery = [laterAsset]
        {
            const auto* view = laterAsset->GetLodAssets()[0]->GetMeshes()[0].GetSemanticBufferAssetView(AZ::Name("POSITION"));
            auto* bytes = const_cast<uint8_t*>(view->GetBufferAsset()->GetBuffer().data());
            const float invalidPosition = AZStd::numeric_limits<float>::quiet_NaN();
            std::memcpy(bytes, &invalidPosition, sizeof(invalidPosition));
        };
        AZStd::array<SD::SurfaceRayBatchProbe, 2> probes{{
            BatchProbe({0, 0, 5}, {0, 0, -3}, 0, 1, {a->GetId(), hook->GetId()}),
            BatchProbe({10, 0, 5}, {10, 0, -3}, 1, 1, {b->GetId(), b->FindComponent<Scene::SurfaceDataMeshComponent>()->GetId()})}};
        AZStd::array<SD::SurfaceRayHit, 2> hits{}; AZStd::array<SD::SurfaceRayQueryResult, 2> perRay{};
        for (auto& hit : hits) { hit.m_distance = 42; }
        for (auto& ray : perRay) { ray.m_hitCount = 77; }
        const auto result = m_system->QueryRays(probes, hits, perRay, BatchLimits(2, 2 + probes.size(), 2));
        EXPECT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::InvalidInput); EXPECT_EQ(hook->m_queryCount, 1);
        EXPECT_EQ(result.m_committedRayCount, 0); EXPECT_EQ(result.m_hitCount, 0);
        for (const auto& hit : hits) { EXPECT_FLOAT_EQ(hit.m_distance, 42); }
        for (const auto& ray : perRay) { EXPECT_EQ(ray.m_hitCount, 77); }
    }

    TEST_F(SurfaceDataMeshQueryFixture, RayBatchRejectsLateInvalidRayOverlappingRangesAndOverflowBeforeProviderQueries)
    {
        auto a = BatchEntity<BatchQueryInterleaveProvider>(Model({Plane(3, 7u)})); Tick();
        auto* hook = a->FindComponent<BatchQueryInterleaveProvider>(); ASSERT_NE(hook, nullptr);
        const auto valid = BatchProbe({0, 0, 5}, {0, 0, -3}, 0, 1, {a->GetId(), hook->GetId()});
        AZStd::array<SD::SurfaceRayBatchProbe, 2> probes{{valid, valid}}; probes[1].m_hitOffset = 1;
        AZStd::array<SD::SurfaceRayHit, 2> hits{}; AZStd::array<SD::SurfaceRayQueryResult, 2> perRay{};
        for (auto& hit : hits) { hit.m_distance = 42; }
        for (auto& ray : perRay) { ray.m_hitCount = 77; }
        const auto limits = BatchLimits(2, 4, 2);
        for (int invalid = 0; invalid != 3; ++invalid)
        {
            probes[1] = valid; probes[1].m_hitOffset = 1;
            if (invalid == 0) { probes[1].m_ray.m_endWorldPosition = probes[1].m_ray.m_startWorldPosition; }
            if (invalid == 1) { probes[1].m_hitOffset = 0; }
            if (invalid == 2) { probes[1].m_hitOffset = AZStd::numeric_limits<size_t>::max(); }
            const auto result = m_system->QueryRays(probes, hits, perRay, limits);
            EXPECT_EQ(result.m_completion, SD::SurfaceRayBatchCompletion::InvalidInput) << invalid;
            EXPECT_EQ(result.m_providerVisitCount, 0); EXPECT_EQ(hook->m_queryCount, 0);
            for (const auto& hit : hits) { EXPECT_FLOAT_EQ(hit.m_distance, 42); }
            for (const auto& ray : perRay) { EXPECT_EQ(ray.m_hitCount, 77); }
        }
        probes[1] = valid; probes[1].m_hitOffset = 1;
        auto rayBudget = limits; rayBudget.m_maxRayCount = 1;
        EXPECT_EQ(m_system->QueryRays(probes, hits, perRay, rayBudget).m_completion, SD::SurfaceRayBatchCompletion::BudgetExhausted);
        AZStd::atomic_bool cancel{true}; auto cancelled = limits; cancelled.m_cancellationFlag = &cancel;
        EXPECT_EQ(m_system->QueryRays(probes, hits, perRay, cancelled).m_completion, SD::SurfaceRayBatchCompletion::Cancelled);
        EXPECT_EQ(hook->m_queryCount, 0);
        for (const auto& hit : hits) { EXPECT_FLOAT_EQ(hit.m_distance, 42); }
        for (const auto& ray : perRay) { EXPECT_EQ(ray.m_hitCount, 77); }
    }

    TEST_F(SurfaceDataMeshQueryFixture, MultilayerSubmeshesKeepProvenanceOptionalSlotsAndNearestCapacity)
    {
        const auto asset = Model({Plane(3, 7u), Plane(3, 8u), Plane(1, RPI::ModelMaterialSlot::InvalidStableId)});
        Open(asset);
        AZStd::array<SD::SurfaceRayHit, 3> hits;
        auto result = m_system->QueryRay(Ray(), hits);
        ASSERT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Complete);
        ASSERT_EQ(result.m_hitCount, 3);
        for (uint32_t i = 0; i < 3; ++i)
        {
            EXPECT_EQ(hits[i].m_entityAndComponent, Ray().m_owner);
            ASSERT_TRUE(hits[i].m_modelProvenance.has_value());
            EXPECT_EQ(hits[i].m_modelProvenance->m_modelAssetId, asset.GetId());
            EXPECT_EQ(hits[i].m_modelProvenance->m_lodIndex, 0);
            EXPECT_EQ(hits[i].m_modelProvenance->m_submeshIndex, i);
            EXPECT_EQ(hits[i].m_primitiveIndex, 0);
            EXPECT_TRUE(hits[i].m_surfaceId.IsNull());
            EXPECT_EQ(hits[i].m_revisionState, SD::SurfaceSourceRevisionState::Content);
            EXPECT_EQ(hits[i].m_sourceRevision, SourceRevision(asset->GetContentRevision()));
        }
        EXPECT_FLOAT_EQ(hits[0].m_distance, 2);
        EXPECT_FLOAT_EQ(hits[1].m_distance, 2);
        EXPECT_FLOAT_EQ(hits[2].m_distance, 4);
        ASSERT_TRUE(hits[0].m_materialSlot.has_value()); EXPECT_EQ(*hits[0].m_materialSlot, 7u);
        ASSERT_TRUE(hits[1].m_materialSlot.has_value()); EXPECT_EQ(*hits[1].m_materialSlot, 8u);
        EXPECT_FALSE(hits[2].m_materialSlot.has_value());
        result = m_system->QueryRay(Ray(), AZStd::span<SD::SurfaceRayHit>(hits.data(), 1));
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(result.m_hitCount, 1); EXPECT_EQ(result.m_discardedHitCount, 2);
        EXPECT_EQ(hits[0].m_modelProvenance->m_submeshIndex, 0);
        result = m_system->QueryRay(Ray(), {});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Truncated);
        EXPECT_EQ(result.m_hitCount, 0); EXPECT_EQ(result.m_discardedHitCount, 3);
        result = m_system->QueryRay(Ray({20, 0, 5}, {20, 0, -3}), {});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Complete);
        EXPECT_EQ(result.m_discardedHitCount, 0);
    }

    TEST_F(SurfaceDataMeshQueryFixture, MissingCpuDataIsUnsupportedAndInvalidRequestLeavesOutputIntact)
    {
        Open(Model({Plane(0, 9u)}, true));
        SD::SurfaceRayHit hit; hit.m_distance = 42;
        auto request = Ray();
        auto result = m_system->QueryRay(request, {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_supportedProviderCount, 0); EXPECT_EQ(result.m_unsupportedProviderCount, 1);
        EXPECT_FLOAT_EQ(hit.m_distance, 42);
        request.m_endWorldPosition = request.m_startWorldPosition;
        result = m_system->QueryRay(request, {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::InvalidInput);
        EXPECT_EQ(result.m_hitCount, 0); EXPECT_FLOAT_EQ(hit.m_distance, 42);
    }

    TEST_F(SurfaceDataMeshQueryFixture, ModelReplacementEmptyAndTwoActivationCyclesLeaveNoOldHits)
    {
        Open(Model({Plane(3, 7u)}));
        SD::SurfaceRayHit hit;
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
        EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), 3);
        const auto replacement = Model({Plane(-1, 12u)});
        m_source->SetModelAsset(replacement); Tick();
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
        EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), -1);
        EXPECT_EQ(hit.m_modelProvenance->m_modelAssetId, replacement.GetId());
        m_source->SetModelAsset({}); Tick();
        EXPECT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_status, SD::SurfaceRayQueryStatus::Unsupported);
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            m_entity->Deactivate();
            EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), SD::InvalidSurfaceDataRegistryHandle);
            EXPECT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_status, SD::SurfaceRayQueryStatus::Unsupported);
            m_source->m_asset = replacement; m_entity->Activate(); Tick();
            ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
            EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), -1);
        }
    }

    TEST_F(SurfaceDataMeshQueryFixture, WorldPositionDistanceAndWindingUseRotationNonUniformScaleAndReflection)
    {
        const Triangle slope{{AZ::Vector3(0, 0, 0), AZ::Vector3(2, 0, 0), AZ::Vector3(0, 2, 2)}, 13u};
        Open(Model({slope}));
        AZ::NonUniformScaleRequestBus::Event(m_entity->GetId(), &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(2, 3, 4));
        for (float uniform : {2.0f, -2.0f})
        {
            auto world = AZ::Transform::CreateRotationZ(AZ::Constants::HalfPi);
            world.SetTranslation({10, 20, 30}); world.SetUniformScale(uniform);
            AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, world); Tick();
            const auto center = uniform > 0 ? AZ::Vector3(7, 22, 34) : AZ::Vector3(13, 18, 26);
            const auto offset = AZ::Vector3(4, 0, 3);
            SD::SurfaceRayHit hit;
            const auto result = m_system->QueryRay(Ray(center + offset, center - offset), {&hit, 1});
            ASSERT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Complete);
            ASSERT_EQ(result.m_hitCount, 1);
            EXPECT_TRUE(hit.m_worldPosition.IsClose(center));
            EXPECT_TRUE(hit.m_worldNormal.IsClose(AZ::Vector3(0.8f, 0, 0.6f)));
            EXPECT_NEAR(hit.m_distance, 5.0f, 0.0001f);
            const auto back = m_system->QueryRay(Ray(center - offset, center + offset), {&hit, 1});
            EXPECT_EQ(back.m_hitCount, 0);
        }
    }

    TEST_F(SurfaceDataMeshQueryFixture, ReflectionSelectsWorldFrontFacesBeforeTruncatingAndPreservesBackWinding)
    {
        Open(Model({Plane(2, 17u), Plane(0, 19u)}));
        auto reflected = AZ::Transform::CreateIdentity(); reflected.SetUniformScale(-1);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, reflected); Tick();
        SD::SurfaceRayHit hit;
        auto request = Ray({0, 0, 3}, {0, 0, -3});
        auto result = m_system->QueryRay(request, {&hit, 1});
        ASSERT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Truncated);
        ASSERT_EQ(result.m_hitCount, 1); EXPECT_EQ(result.m_discardedHitCount, 1);
        EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), 0); EXPECT_FLOAT_EQ(hit.m_distance, 3);
        EXPECT_EQ(hit.m_modelProvenance->m_submeshIndex, 1);
        EXPECT_TRUE(hit.m_worldNormal.IsClose(AZ::Vector3::CreateAxisZ()));
        result = m_system->QueryRay(request, {});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Truncated); EXPECT_EQ(result.m_discardedHitCount, 2);
        request = Ray({0, 0, -3}, {0, 0, 3});
        EXPECT_EQ(m_system->QueryRay(request, {&hit, 1}).m_hitCount, 0);
        request.m_sidedness = SD::SurfaceRaySidedness::DoubleSided;
        ASSERT_EQ(m_system->QueryRay(request, {&hit, 1}).m_hitCount, 1);
        EXPECT_TRUE(hit.m_worldNormal.IsClose(AZ::Vector3::CreateAxisZ()));
    }

    TEST_F(SurfaceDataMeshQueryFixture, RayCapabilityDoesNotChangeExistingSingleXyPointAndLegacyScopeIsPartial)
    {
        Open(Model({Plane(3, 7u), Plane(1, 9u)}));
        SD::SurfacePointList points;
        m_system->GetSurfacePoints({0, 0, 0}, {}, points);
        ASSERT_EQ(points.GetSize(), 1);
        EXPECT_FLOAT_EQ(points.GetHighestSurfacePoint(0).m_position.GetZ(), 3);
        SD::SurfaceDataRegistryEntry legacy;
        legacy.m_entityId = m_entity->GetId(); legacy.m_bounds = AZ::Aabb::CreateCenterRadius({0, 0, 0}, 10);
        legacy.m_maxPointsCreatedPerInput = 1;
        const auto handle = m_system->RegisterSurfaceDataProvider(legacy);
        SD::SurfaceRayHit hit;
        const auto result = m_system->QueryRay(Ray(), {&hit, 1});
        m_system->UnregisterSurfaceDataProvider(handle);
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Partial);
        EXPECT_EQ(result.m_hitCount, 1); EXPECT_EQ(result.m_discardedHitCount, 1);
        EXPECT_EQ(result.m_supportedProviderCount, 1); EXPECT_EQ(result.m_unsupportedProviderCount, 1);
    }
    TEST_F(SurfaceDataMeshQueryFixture, InvalidLaterSubmeshAndSingularTransformNeverPublishPartialOutput)
    {
        auto asset = Model({Plane(3, 7u), Plane(1, 9u)});
        Open(asset);
        const auto* view = asset->GetLodAssets()[0]->GetMeshes()[1].GetSemanticBufferAssetView(AZ::Name("POSITION"));
        auto* bytes = const_cast<uint8_t*>(view->GetBufferAsset()->GetBuffer().data());
        reinterpret_cast<float*>(bytes)[0] = AZStd::numeric_limits<float>::quiet_NaN();
        SD::SurfaceRayHit hit; hit.m_distance = 42; hit.m_materialSlot = 99;
        auto result = m_system->QueryRay(Ray(), {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::InvalidInput);
        EXPECT_EQ(result.m_hitCount, 0); EXPECT_EQ(result.m_discardedHitCount, 0);
        EXPECT_EQ(result.m_supportedProviderCount, 0); EXPECT_EQ(result.m_unsupportedProviderCount, 0);
        EXPECT_FLOAT_EQ(hit.m_distance, 42); EXPECT_EQ(*hit.m_materialSlot, 99u);
        EXPECT_FALSE(hit.m_modelProvenance.has_value());
        m_source->SetModelAsset(Model({Plane(0, 9u)})); Tick();
        auto singular = AZ::Transform::CreateIdentity(); singular.SetUniformScale(0);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, singular); Tick();
        result = m_system->QueryRay(Ray(), {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::InvalidInput);
        EXPECT_EQ(result.m_hitCount, 0); EXPECT_FLOAT_EQ(hit.m_distance, 42);
    }

    TEST_F(SurfaceDataMeshQueryFixture, TransformUpdatesExistingProviderAndXyWithoutLeavingOldHits)
    {
        Open(Model({Plane(3, 7u)}));
        const auto handle = m_system->GetSurfaceDataProviderHandle(m_entity->GetId());
        ASSERT_NE(handle, SD::InvalidSurfaceDataRegistryHandle);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation({10, 0, 0})); Tick();
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), handle);
        SD::SurfaceRayHit hit;
        EXPECT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 0);
        ASSERT_EQ(m_system->QueryRay(Ray({10, 0, 5}, {10, 0, -3}), {&hit, 1}).m_hitCount, 1);
        EXPECT_TRUE(hit.m_worldPosition.IsClose(AZ::Vector3(10, 0, 3)));
        SD::SurfacePointList points;
        m_system->GetSurfacePoints({0, 0, 0}, {}, points); EXPECT_TRUE(points.IsEmpty());
        m_system->GetSurfacePoints({10, 0, 0}, {}, points); EXPECT_EQ(points.GetSize(), 1);
    }
    TEST_F(SurfaceDataMeshQueryFixture, PreDestroyAndReplacementAreUnsupportedBeforeTickWhileXyRemainsDeferred)
    {
        Open(Model({Plane(3, 7u)}));
        const auto handle = m_system->GetSurfaceDataProviderHandle(m_entity->GetId());
        SD::SurfaceRayHit hit; hit.m_distance = 42; hit.m_materialSlot = 99;
        Scene::MeshComponentNotificationBus::Event(m_entity->GetId(), &Scene::MeshComponentNotifications::OnModelPreDestroy);
        auto result = m_system->QueryRay(Ray(), {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_hitCount, 0); EXPECT_FLOAT_EQ(hit.m_distance, 42);
        const auto pendingBounds = m_provider->GetSurfaceRayBounds();
        ASSERT_TRUE(pendingBounds.has_value()); EXPECT_FALSE(pendingBounds->IsValid());
        const auto replacement = Model({Plane(-1, 11u)});
        m_source->SetModelAsset(replacement);
        auto newRay = Ray({0, 0, -0.5f}, {0, 0, -1.5f});
        newRay.m_filter = SD::SurfaceRayQueryFilter::AllRegisteredProviders; // This segment misses the OLD registered AABB.
        result = m_system->QueryRay(newRay, {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_hitCount, 0); EXPECT_FLOAT_EQ(hit.m_distance, 42);
        EXPECT_EQ(*hit.m_materialSlot, 99u); EXPECT_FALSE(hit.m_modelProvenance.has_value());
        SD::SurfacePointList points;
        m_system->GetSurfacePoints({0, 0, 0}, {}, points);
        ASSERT_EQ(points.GetSize(), 1); EXPECT_FLOAT_EQ(points.GetHighestSurfacePoint(0).m_position.GetZ(), 3);
        Tick();
        ASSERT_EQ(m_system->QueryRay(newRay, {&hit, 1}).m_hitCount, 1);
        EXPECT_EQ(hit.m_modelProvenance->m_modelAssetId, replacement.GetId());
        EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), -1);
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), handle);
        const auto steady = m_provider->GetSurfaceRayBounds();
        ASSERT_TRUE(steady.has_value()); ASSERT_TRUE(steady->IsValid());
        EXPECT_FLOAT_EQ(steady->GetMin().GetZ(), -1);
    }

    TEST_F(SurfaceDataMeshQueryFixture, PendingMovementIsUnknownOutsideOldBoundsAndSteadyRestoresBroadphase)
    {
        Open(Model({Plane(0, 7u)}));
        const auto handle = m_system->GetSurfaceDataProviderHandle(m_entity->GetId());
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation({10, 0, 0}));
        SD::SurfaceRayHit hit; hit.m_distance = 42;
        auto destination = Ray({10, 0, 1}, {10, 0, -1});
        destination.m_filter = SD::SurfaceRayQueryFilter::AllRegisteredProviders;
        auto result = m_system->QueryRay(destination, {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_hitCount, 0); EXPECT_FLOAT_EQ(hit.m_distance, 42);
        result = m_system->QueryRay(Ray(), {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(result.m_hitCount, 0);
        Tick();
        ASSERT_EQ(m_system->QueryRay(destination, {&hit, 1}).m_hitCount, 1);
        EXPECT_TRUE(hit.m_worldPosition.IsClose(AZ::Vector3(10, 0, 0)));
        auto oldLocation = Ray(); oldLocation.m_filter = SD::SurfaceRayQueryFilter::AllRegisteredProviders;
        result = m_system->QueryRay(oldLocation, {&hit, 1});
        EXPECT_EQ(result.m_status, SD::SurfaceRayQueryStatus::Complete);
        EXPECT_EQ(result.m_hitCount, 0); EXPECT_EQ(result.m_supportedProviderCount, 0); // Steady bounds cull the model again.
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), handle);
    }

    TEST_F(SurfaceDataMeshQueryFixture, NotificationDuringSnapshotCaptureKeepsDirtyAndCommitsOnlyLatestSnapshotOnNextTick)
    {
        const auto initial = Model({Plane(3, 7u)});
        const auto intermediate = Model({Plane(1, 9u)});
        const auto latest = Model({Plane(-1, 11u)});
        Open(initial);
        const auto handle = m_system->GetSurfaceDataProviderHandle(m_entity->GetId());
        m_source->SetModelAsset(intermediate);
        m_source->m_onBoundsRead = [this, latest]() { m_source->SetModelAsset(latest); };
        Tick(); // A real OnModelReady/PreDestroy occurs while UpdateMeshData is capturing its fields.
        SD::SurfaceRayHit hit; hit.m_distance = 42;
        auto outsideOld = Ray({0, 0, -0.5f}, {0, 0, -1.5f});
        outsideOld.m_filter = SD::SurfaceRayQueryFilter::AllRegisteredProviders;
        const auto pending = m_system->QueryRay(outsideOld, {&hit, 1});
        EXPECT_EQ(pending.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(pending.m_hitCount, 0); EXPECT_FLOAT_EQ(hit.m_distance, 42);
        SD::SurfacePointList points;
        m_system->GetSurfacePoints({0, 0, 0}, {}, points);
        ASSERT_EQ(points.GetSize(), 1); EXPECT_FLOAT_EQ(points.GetHighestSurfacePoint(0).m_position.GetZ(), 3);
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), handle);
        Tick(); // The notification was not overwritten by publication or TickBus disconnection.
        const auto current = m_system->QueryRay(outsideOld, {&hit, 1});
        ASSERT_EQ(current.m_status, SD::SurfaceRayQueryStatus::Complete); ASSERT_EQ(current.m_hitCount, 1);
        EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), -1);
        EXPECT_EQ(hit.m_modelProvenance->m_modelAssetId, latest.GetId());
        m_system->GetSurfacePoints({0, 0, 0}, {}, points);
        ASSERT_EQ(points.GetSize(), 1); EXPECT_FLOAT_EQ(points.GetHighestSurfacePoint(0).m_position.GetZ(), -1);
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), handle);
    }

    TEST_F(SurfaceDataMeshQueryFixture, DeactivateWhilePendingRemovesRegistrationAndDoesNotRunDeferredCapture)
    {
        Open(Model({Plane(3, 7u)}));
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation({10, 0, 0}));
        m_entity->Deactivate();
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), SD::InvalidSurfaceDataRegistryHandle);
        Tick();
        SD::SurfaceRayHit hit; hit.m_distance = 42;
        const auto after = m_system->QueryRay(Ray(), {&hit, 1});
        EXPECT_EQ(after.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(after.m_hitCount, 0); EXPECT_FLOAT_EQ(hit.m_distance, 42);
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), SD::InvalidSurfaceDataRegistryHandle);
        m_entity->Activate(); Tick();
        ASSERT_EQ(m_system->QueryRay(Ray({10, 0, 5}, {10, 0, -3}), {&hit, 1}).m_hitCount, 1);
        EXPECT_TRUE(hit.m_worldPosition.IsClose(AZ::Vector3(10, 0, 3)));
    }
    TEST_F(SurfaceDataMeshQueryFixture, InvalidatedInitialCaptureDoesNotCreateGhostRegistration)
    {
        const auto first = Model({Plane(3, 7u)});
        const auto latest = Model({Plane(-1, 11u)});
        m_entity = AZStd::make_unique<AZ::Entity>("Initially invalidated real provider");
        m_entity->CreateComponent<AzFramework::TransformComponent>();
        m_entity->CreateComponent<AzFramework::NonUniformScaleComponent>();
        m_source = m_entity->CreateComponent<CpuMeshSource>(); m_source->m_asset = first;
        m_source->m_onBoundsRead = [this, latest]() { m_source->SetModelAsset(latest); };
        m_provider = m_entity->CreateComponent<Scene::SurfaceDataMeshComponent>();
        m_entity->Init(); m_entity->Activate();
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), SD::InvalidSurfaceDataRegistryHandle);
        const auto bounds = m_provider->GetSurfaceRayBounds();
        ASSERT_TRUE(bounds.has_value()); EXPECT_FALSE(bounds->IsValid());
        SD::SurfaceRayHit hit; hit.m_distance = 42;
        const auto pending = m_system->QueryRay(Ray(), {&hit, 1});
        EXPECT_EQ(pending.m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(pending.m_hitCount, 0); EXPECT_FLOAT_EQ(hit.m_distance, 42);
        Tick();
        EXPECT_NE(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), SD::InvalidSurfaceDataRegistryHandle);
        const auto current = m_system->QueryRay(Ray(), {&hit, 1});
        ASSERT_EQ(current.m_status, SD::SurfaceRayQueryStatus::Complete); ASSERT_EQ(current.m_hitCount, 1);
        EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), -1);
        EXPECT_EQ(hit.m_modelProvenance->m_modelAssetId, latest.GetId());
    }

    TEST_F(SurfaceDataMeshQueryFixture, ValidatedModelContentRevisionSurvivesPoseWithoutRevalidatingStoredClaim)
    {
        auto model = Model({Plane(0, 7u)}, false, true);
        const auto claim = model->GetContentRevision(); ASSERT_TRUE(model->ValidateContentRevision());
        Open(model);
        SD::SurfaceRayHit hit;
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
        ASSERT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(hit.m_sourceRevision, SourceRevision(claim));
        EXPECT_TRUE(hit.m_surfaceId.IsNull());
        const auto handle = m_system->GetSurfaceDataProviderHandle(m_entity->GetId());
        // Fault injection changes only the stored claim, never semantic payload. Revalidating it would now fail.
        auto& stored = const_cast<RPI::ModelContentRevision&>(model->GetContentRevision()); stored.m_schema = 0;
        auto world = AZ::Transform::CreateTranslation(AZ::Vector3(10, 0, 2));
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, world);
        AZ::NonUniformScaleRequestBus::Event(m_entity->GetId(), &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(2, 3, 4));
        Tick();
        ASSERT_EQ(m_system->QueryRay(Ray({10, 0, 5}, {10, 0, -3}), {&hit, 1}).m_hitCount, 1);
        EXPECT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(hit.m_sourceRevision, SourceRevision(claim)); EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), 2);
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), handle);
        Scene::MeshComponentNotificationBus::Event(m_entity->GetId(), &Scene::MeshComponentNotifications::OnModelReady,
            model, AZ::Data::Instance<RPI::Model>{}); Tick();
        ASSERT_EQ(m_system->QueryRay(Ray({10, 0, 5}, {10, 0, -3}), {&hit, 1}).m_hitCount, 1);
        EXPECT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Unavailable); EXPECT_EQ(hit.m_sourceRevision.m_schema, 0);
        stored = claim;
        Scene::MeshComponentNotificationBus::Event(m_entity->GetId(), &Scene::MeshComponentNotifications::OnModelReady,
            model, AZ::Data::Instance<RPI::Model>{}); Tick();
        ASSERT_EQ(m_system->QueryRay(Ray({10, 0, 5}, {10, 0, -3}), {&hit, 1}).m_hitCount, 1);
        EXPECT_EQ(hit.m_sourceRevision, SourceRevision(claim)); EXPECT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Content);
    }

    TEST_F(SurfaceDataMeshQueryFixture, SameAssetIdUvOnlyReloadChangesRevisionWithoutChangingGeometryOrPose)
    {
        auto initial = Model({Plane(0, 7u)}, false, true, 0.25f);
        auto replacement = Model({Plane(0, 7u)}, false, true, 0.75f);
        ASSERT_NE(initial->GetContentRevision(), replacement->GetContentRevision());
        ASSERT_EQ(initial->GetAabb(), replacement->GetAabb()); Open(initial);
        SD::SurfaceRayHit before; ASSERT_EQ(m_system->QueryRay(Ray(), {&before, 1}).m_hitCount, 1);
        const auto id = initial.GetId(); const auto* pointer = initial.Get();
        const auto handle = m_system->GetSurfaceDataProviderHandle(m_entity->GetId());
        ASSERT_NO_FATAL_FAILURE(ReloadCurrentModelFrom(replacement));
        SD::SurfaceRayHit after; after.m_distance = 42;
        EXPECT_EQ(m_system->QueryRay(Ray(), {&after, 1}).m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_FLOAT_EQ(after.m_distance, 42); Tick();
        ASSERT_EQ(m_system->QueryRay(Ray(), {&after, 1}).m_hitCount, 1);
        ASSERT_TRUE(after.m_modelProvenance);
        EXPECT_EQ(after.m_modelProvenance->m_modelAssetId, id); EXPECT_EQ(initial.Get(), pointer);
        EXPECT_TRUE(after.m_worldPosition.IsClose(before.m_worldPosition)); EXPECT_TRUE(after.m_worldNormal.IsClose(before.m_worldNormal));
        EXPECT_EQ(after.m_materialSlot, before.m_materialSlot); EXPECT_EQ(after.m_sourceRevision, SourceRevision(replacement->GetContentRevision()));
        EXPECT_NE(after.m_sourceRevision, before.m_sourceRevision); EXPECT_EQ(after.m_revisionState, SD::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), handle);
    }

    TEST_F(SurfaceDataMeshQueryFixture, LegacyUnknownAndMismatchedClaimsNeverAdvertiseContentForValidGeometry)
    {
        for (int kind = 0; kind != 3; ++kind)
        {
            auto model = Model({Plane(0, 7u)}, false, true);
            if (kind == 0)
            {
                // Real legacy stream into a native ready asset whose initial claim is unavailable.
                auto target = Model({Plane(0, 7u)}, true);
                ASSERT_EQ(target->GetContentRevision().m_schema, 0);
                ASSERT_NO_FATAL_FAILURE(DeserializeModelSnapshot(model, target, true)); model = target;
            }
            if (kind == 1) { const_cast<RPI::ModelContentRevision&>(model->GetContentRevision()).m_schema = 999; }
            if (kind == 2)
            {
                const auto& uv = model->GetLodAssets()[0]->GetMeshes()[0].GetStreamBufferInfoList()[1].m_bufferAssetView;
                const float changed = 0.5f;
                std::memcpy(const_cast<uint8_t*>(uv.GetBufferAsset()->GetBuffer().data()), &changed, sizeof(changed));
            }
            EXPECT_FALSE(model->ValidateContentRevision()); Open(model);
            SD::SurfaceRayHit hit; ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
            EXPECT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Unavailable); EXPECT_EQ(hit.m_sourceRevision.m_schema, 0);
            EXPECT_EQ(hit.m_sourceRevision.m_digest, SD::SurfaceSourceRevision{}.m_digest);
            EXPECT_TRUE(hit.m_surfaceId.IsNull()); ASSERT_TRUE(hit.m_modelProvenance);
            EXPECT_EQ(hit.m_modelProvenance->m_modelAssetId, model.GetId()); m_entity.reset();
        }
    }

    TEST_F(SurfaceDataMeshQueryFixture, PreDestroyStaysUnsupportedAcrossTickUntilReadyAndActivationRevalidates)
    {
        auto model = Model({Plane(0, 7u)}, false, true); const auto claim = model->GetContentRevision(); Open(model);
        Scene::MeshComponentNotificationBus::Event(m_entity->GetId(), &Scene::MeshComponentNotifications::OnModelPreDestroy);
        Tick(); // Renderer boundary deliberately still returns the previous Ready asset.
        SD::SurfaceRayHit hit; hit.m_distance = 42; hit.m_sourceRevision = SourceRevision(claim);
        EXPECT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_FLOAT_EQ(hit.m_distance, 42); EXPECT_EQ(hit.m_sourceRevision, SourceRevision(claim));
        const auto bounds = m_provider->GetSurfaceRayBounds(); ASSERT_TRUE(bounds); EXPECT_FALSE(bounds->IsValid());
        Scene::MeshComponentNotificationBus::Event(m_entity->GetId(), &Scene::MeshComponentNotifications::OnModelReady,
            model, AZ::Data::Instance<RPI::Model>{}); Tick();
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1); EXPECT_EQ(hit.m_sourceRevision, SourceRevision(claim));
        for (int cycle = 0; cycle != 2; ++cycle)
        {
            m_entity->Deactivate();
            EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), SD::InvalidSurfaceDataRegistryHandle);
            const_cast<RPI::ModelContentRevision&>(model->GetContentRevision()).m_schema = 0;
            m_entity->Activate();
            ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
            EXPECT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Unavailable);
            const_cast<RPI::ModelContentRevision&>(model->GetContentRevision()) = claim;
            Scene::MeshComponentNotificationBus::Event(m_entity->GetId(), &Scene::MeshComponentNotifications::OnModelReady,
                model, AZ::Data::Instance<RPI::Model>{}); Tick();
            ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1); EXPECT_EQ(hit.m_sourceRevision, SourceRevision(claim));
        }
    }

    TEST_F(SurfaceDataMeshQueryFixture, ReentrantContentReloadNeverPublishesMixedModelRevisionAndPoseSnapshot)
    {
        auto initial = Model({Plane(3, 7u)}, false, true, 0.25f);
        auto intermediate = Model({Plane(1, 9u)}, false, true, 0.5f);
        auto latest = Model({Plane(-1, 11u)}, false, true, 0.75f); Open(initial);
        const auto handle = m_system->GetSurfaceDataProviderHandle(m_entity->GetId());
        m_source->SetModelAsset(intermediate);
        m_source->m_onBoundsRead = [this, latest]() { m_source->SetModelAsset(latest); };
        Tick(); SD::SurfaceRayHit hit; hit.m_distance = 42; hit.m_sourceRevision = SourceRevision(initial->GetContentRevision());
        EXPECT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_FLOAT_EQ(hit.m_distance, 42); EXPECT_EQ(hit.m_sourceRevision, SourceRevision(initial->GetContentRevision()));
        EXPECT_EQ(m_system->GetSurfaceDataProviderHandle(m_entity->GetId()), handle); Tick();
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
        EXPECT_FLOAT_EQ(hit.m_worldPosition.GetZ(), -1); EXPECT_EQ(hit.m_modelProvenance->m_modelAssetId, latest.GetId());
        EXPECT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(hit.m_sourceRevision, SourceRevision(latest->GetContentRevision()));
        EXPECT_NE(hit.m_sourceRevision, SourceRevision(intermediate->GetContentRevision()));
    }
    TEST_F(SurfaceDataMeshQueryFixture, ValidatedAuthoredFaceReferencesReachMaterialSplitHitsAndSurvivePose)
    {
        RPI::ModelSurfaceIdentityTable table;
        table.m_schema = RPI::ModelSurfaceIdentityTable::Schema;
        const AZ::Uuid surface("{10000000-0000-0000-0000-000000000001}");
        const AZ::Uuid face("{20000000-0000-0000-0000-000000000002}");
        const AZ::Uuid region("{30000000-0000-0000-0000-000000000003}");
        table.m_entries = {{surface, face, region}};
        auto model = Model({Plane(1, 7u), Plane(0, 9u)}, false, true, 0.25f, table);
        ASSERT_TRUE(model->ValidateContentRevision());
        Open(model);
        AZStd::array<SD::SurfaceRayHit, 2> hits;
        ASSERT_EQ(m_system->QueryRay(Ray(), hits).m_hitCount, 2);
        EXPECT_EQ(hits[0].m_materialSlot, 7u); EXPECT_EQ(hits[1].m_materialSlot, 9u);
        for (const auto& hit : hits)
        {
            EXPECT_EQ(hit.m_surfaceId, surface); EXPECT_EQ(hit.m_faceId, face);
            ASSERT_TRUE(hit.m_regionId); EXPECT_EQ(*hit.m_regionId, region);
            EXPECT_EQ(hit.m_sourceRevision, SourceRevision(model->GetContentRevision()));
        }
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3(10, 0, 2)));
        Tick();
        ASSERT_EQ(m_system->QueryRay(Ray({10, 0, 5}, {10, 0, -3}), hits).m_hitCount, 2);
        for (const auto& hit : hits)
        {
            EXPECT_EQ(hit.m_surfaceId, surface); EXPECT_EQ(hit.m_faceId, face);
            ASSERT_TRUE(hit.m_regionId); EXPECT_EQ(*hit.m_regionId, region);
        }
    }

    TEST_F(SurfaceDataMeshQueryFixture, UnvalidatedClaimWithAuthoredTableReturnsGeometryWithoutIdentity)
    {
        RPI::ModelSurfaceIdentityTable table;
        table.m_schema = RPI::ModelSurfaceIdentityTable::Schema;
        table.m_entries = {{AZ::Uuid("{10000000-0000-0000-0000-000000000001}"),
            AZ::Uuid("{20000000-0000-0000-0000-000000000002}"), AZStd::nullopt}};
        auto model = Model({Plane(0, 7u)}, false, true, 0.25f, table);
        const_cast<RPI::ModelContentRevision&>(model->GetContentRevision()).m_schema = 999;
        Open(model);
        SD::SurfaceRayHit hit;
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
        EXPECT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Unavailable);
        EXPECT_TRUE(hit.m_surfaceId.IsNull()); EXPECT_TRUE(hit.m_faceId.IsNull()); EXPECT_FALSE(hit.m_regionId);
        EXPECT_EQ(hit.m_materialSlot, 7u);
    }
    TEST_F(SurfaceDataMeshQueryFixture, SameAssetIdIdentityReloadPublishesNewFaceAndClearsRemovedIdentity)
    {
        RPI::ModelSurfaceIdentityTable table;
        table.m_schema = RPI::ModelSurfaceIdentityTable::Schema;
        const AZ::Uuid surface("{10000000-0000-0000-0000-000000000001}");
        const AZ::Uuid firstFace("{20000000-0000-0000-0000-000000000002}");
        const AZ::Uuid nextFace("{20000000-0000-0000-0000-000000000004}");
        table.m_entries = {{surface, firstFace, AZStd::nullopt}};
        auto initial = Model({Plane(0, 7u)}, false, true, 0.25f, table);
        Open(initial);
        SD::SurfaceRayHit hit;
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
        ASSERT_EQ(hit.m_faceId, firstFace);
        const auto originalId = initial.GetId();
        const auto originalRevision = hit.m_sourceRevision;
        table.m_entries[0].m_faceId = nextFace;
        auto replacement = Model({Plane(0, 7u)}, false, true, 0.25f, table);
        ASSERT_NO_FATAL_FAILURE(ReloadCurrentModelFrom(replacement));
        EXPECT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_status, SD::SurfaceRayQueryStatus::Unsupported);
        EXPECT_EQ(hit.m_faceId, firstFace); // A pending query must not partially overwrite the output.
        Tick();
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
        EXPECT_EQ(initial.GetId(), originalId);
        EXPECT_EQ(hit.m_surfaceId, surface); EXPECT_EQ(hit.m_faceId, nextFace);
        EXPECT_NE(hit.m_sourceRevision, originalRevision);
        EXPECT_EQ(hit.m_sourceRevision, SourceRevision(replacement->GetContentRevision()));
        auto withoutIdentity = Model({Plane(0, 7u)}, false, true);
        ASSERT_NO_FATAL_FAILURE(ReloadCurrentModelFrom(withoutIdentity)); Tick();
        ASSERT_EQ(m_system->QueryRay(Ray(), {&hit, 1}).m_hitCount, 1);
        EXPECT_TRUE(hit.m_surfaceId.IsNull()); EXPECT_TRUE(hit.m_faceId.IsNull()); EXPECT_FALSE(hit.m_regionId);
        EXPECT_EQ(hit.m_revisionState, SD::SurfaceSourceRevisionState::Content);
        EXPECT_EQ(initial.GetId(), originalId);
    }
}
