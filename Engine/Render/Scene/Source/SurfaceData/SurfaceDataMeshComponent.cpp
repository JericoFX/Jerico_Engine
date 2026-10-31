/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SurfaceDataMeshComponent.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/limits.h>
#include <cmath>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <JE/Render/RPI/Reflect/Model/ModelAssetCreator.h>

#include <JE/Geometry/SurfaceData/SurfaceDataSystemRequestBus.h>
#include <JE/Geometry/SurfaceData/Utility/SurfaceDataUtility.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace JE::Render::Scene
{
    namespace
    {
        // Ray and volume results authenticate references from precisely the same validated, locked snapshot.
        void SetModelHitIdentity(const RPI::ModelAsset& model, const AZ::Data::AssetId& assetId,
            const JE::Geometry::SurfaceData::SurfaceSourceRevision& revision, const AZ::EntityComponentIdPair& owner,
            AZ::u32 meshIndex, AZ::u32 primitiveIndex, AZStd::optional<RPI::ModelMaterialSlot::StableId> slot,
            JE::Geometry::SurfaceData::SurfaceRayHit& hit)
        {
            namespace SD = JE::Geometry::SurfaceData;
            hit.m_entityAndComponent = owner;
            hit.m_materialSlot = slot;
            hit.m_primitiveIndex = primitiveIndex;
            hit.m_sourceRevision = revision;
            hit.m_revisionState = revision.m_schema != 0 ? SD::SurfaceSourceRevisionState::Content : SD::SurfaceSourceRevisionState::Unavailable;
            hit.m_modelProvenance = SD::SurfaceModelProvenance{assetId, 0, meshIndex};
            const auto lods = model.GetLodAssets();
            if (revision.m_schema == RPI::ModelContentRevision::Schema && !lods.empty() && lods[0].IsReady() && meshIndex < lods[0]->GetMeshes().size())
            {
                if (const auto* reference = lods[0]->GetMeshes()[meshIndex].GetSurfaceIdentity(primitiveIndex))
                {
                    hit.m_surfaceId = reference->m_surfaceId;
                    hit.m_faceId = reference->m_faceId;
                    hit.m_regionId = reference->m_regionId;
                }
            }
        }
    }

    void SurfaceDataMeshConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataMeshConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("SurfaceTags", &SurfaceDataMeshConfig::m_tags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceDataMeshConfig>(
                    QT_TRANSLATE_NOOP("AtomLyIntegration", "Mesh Surface Tag Emitter"), "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &SurfaceDataMeshConfig::m_tags, QT_TRANSLATE_NOOP("AtomLyIntegration", "Generated Tags"), "")
                    ;
            }
        }
    }

    void SurfaceDataMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
    }

    void SurfaceDataMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
    }

    void SurfaceDataMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("MeshService"));
    }

    void SurfaceDataMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceDataMeshConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceDataMeshComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceDataMeshComponent::m_configuration)
                ;
        }
    }

    SurfaceDataMeshComponent::SurfaceDataMeshComponent()
        : m_nonUniformScaleChangedHandler([this]([[maybe_unused]] const AZ::Vector3& scale) { this->OnCompositionChanged(); })
    {
    }

    SurfaceDataMeshComponent::SurfaceDataMeshComponent(const SurfaceDataMeshConfig& configuration)
        : m_configuration(configuration)
        , m_nonUniformScaleChangedHandler([this]([[maybe_unused]] const AZ::Vector3& scale) { this->OnCompositionChanged(); })
    {
    }

    void SurfaceDataMeshComponent::Activate()
    {
        m_modelPreDestroyPending.store(false);
        OnModelContentChanged();
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        JE::Render::Scene::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());

        AZ::NonUniformScaleRequestBus::Event(
            GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);

        m_providerHandle = JE::Geometry::SurfaceData::InvalidSurfaceDataRegistryHandle;

        // Update the cached mesh data and bounds, then register the surface data provider.
        UpdateMeshData();
        // Lifetime connection avoids losing an invalidation against a per-refresh disconnect.
        // Steady ticks only compare the two generations; no renderer/registry work occurs.
        AZ::TickBus::Handler::BusConnect();
    }

    void SurfaceDataMeshComponent::Deactivate()
    {
        m_modelPreDestroyPending.store(true);
        OnModelContentChanged();
        if (m_providerHandle != JE::Geometry::SurfaceData::InvalidSurfaceDataRegistryHandle)
        {
            AZ::Interface<JE::Geometry::SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            m_providerHandle = JE::Geometry::SurfaceData::InvalidSurfaceDataRegistryHandle;
        }

        m_nonUniformScaleChangedHandler.Disconnect();
        JE::Geometry::SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        JE::Render::Scene::MeshComponentNotificationBus::Handler::BusDisconnect();

        // Clear the cached mesh data
        {
            AZStd::unique_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
            m_meshAssetData = {};
            m_validatedContentGeneration = 0;
            m_meshSourceRevision = {};
            m_meshBounds = AZ::Aabb::CreateNull();
            m_meshWorldTM = AZ::Transform::CreateIdentity();
            m_meshWorldTMInverse = AZ::Transform::CreateIdentity();
            m_meshNonUniformScale = AZ::Vector3::CreateOne();
            m_cachedTags.clear();
            m_newPointWeights = {};
            m_publishedGeneration.store(0);
        }
    }

    bool SurfaceDataMeshComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceDataMeshConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceDataMeshComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceDataMeshConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    AZStd::optional<AZ::Aabb> SurfaceDataMeshComponent::GetSurfaceRayBounds() const
    {
        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
        if (m_modelPreDestroyPending.load() || m_publishedGeneration.load() != m_requestedGeneration.load() ||
            m_validatedContentGeneration != m_requestedContentGeneration.load())
        {
            return AZ::Aabb::CreateNull(); // Unknown while pending: global culling must include this provider.
        }
        return m_meshBounds; // Steady state retains the ordinary broadphase.
    }

    JE::Geometry::SurfaceData::SurfaceRayQueryResult SurfaceDataMeshComponent::QueryRay(
        const JE::Geometry::SurfaceData::SurfaceRayRequest& request,
        AZStd::span<JE::Geometry::SurfaceData::SurfaceRayHit> output) const
    {
        namespace SD = JE::Geometry::SurfaceData;
        namespace RPI = JE::Render::RPI;
        SD::SurfaceRayQueryResult invalid;
        invalid.m_status = SD::SurfaceRayQueryStatus::InvalidInput;
        const AZ::Vector3 segment = request.m_endWorldPosition - request.m_startWorldPosition;
        const float worldLength = segment.GetLength();
        if (!request.m_startWorldPosition.IsFinite() || !request.m_endWorldPosition.IsFinite() ||
            !std::isfinite(worldLength) || worldLength <= 0.0f ||
            (request.m_sidedness != SD::SurfaceRaySidedness::FrontFace && request.m_sidedness != SD::SurfaceRaySidedness::DoubleSided) ||
            (request.m_filter != SD::SurfaceRayQueryFilter::AllRegisteredProviders && request.m_filter != SD::SurfaceRayQueryFilter::ExactOwner) ||
            (request.m_filter == SD::SurfaceRayQueryFilter::ExactOwner &&
                (!request.m_owner.GetEntityId().IsValid() || request.m_owner.GetComponentId() == AZ::InvalidComponentId)) ||
            (!output.empty() && output.data() == nullptr) ||
            output.size() > AZStd::numeric_limits<size_t>::max() / sizeof(SD::SurfaceRayHit))
        {
            return invalid;
        }
        SD::SurfaceRayQueryResult unsupported;
        unsupported.m_status = SD::SurfaceRayQueryStatus::Unsupported;
        unsupported.m_unsupportedProviderCount = 1;
        const AZ::EntityComponentIdPair owner(GetEntityId(), GetId());
        if (request.m_filter == SD::SurfaceRayQueryFilter::ExactOwner && request.m_owner != owner) { return unsupported; }

        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
        const AZ::u64 snapshotGeneration = m_publishedGeneration.load();
        if (snapshotGeneration != m_requestedGeneration.load() ||
            m_validatedContentGeneration != m_requestedContentGeneration.load() || m_modelPreDestroyPending.load()) { return unsupported; }
        auto* model = m_meshAssetData.GetAs<RPI::ModelAsset>();
        if (!model || !m_meshAssetData.IsReady() || !m_meshAssetData.GetId().IsValid()) { return unsupported; }
        const auto& scale = m_meshNonUniformScale;
        const float uniformScale = m_meshWorldTM.GetUniformScale();
        if (!m_meshWorldTM.IsFinite() || !m_meshWorldTMInverse.IsFinite() || !scale.IsFinite() ||
            uniformScale == 0.0f || scale.GetX() == 0.0f || scale.GetY() == 0.0f || scale.GetZ() == 0.0f)
        {
            return invalid;
        }
        // Endpoint transformation preserves segment t; never normalize the local direction.
        const AZ::Vector3 localStart = m_meshWorldTMInverse.TransformPoint(request.m_startWorldPosition) / scale;
        const AZ::Vector3 localEnd = m_meshWorldTMInverse.TransformPoint(request.m_endWorldPosition) / scale;
        if (!localStart.IsFinite() || !localEnd.IsFinite()) { return invalid; }
        const bool reflected = ((uniformScale < 0.0f) != (scale.GetX() < 0.0f)) != ((scale.GetY() < 0.0f) != (scale.GetZ() < 0.0f));
        const auto side = request.m_sidedness == SD::SurfaceRaySidedness::DoubleSided
            ? RPI::ModelRaySidedness::DoubleSided
            : reflected ? RPI::ModelRaySidedness::BackFace : RPI::ModelRaySidedness::FrontFace;
        AZStd::vector<RPI::ModelRayHit> localHits(output.size());
        const auto local = model->LocalRayIntersectionsAgainstModel(
            localStart, localEnd - localStart, true, side, {localHits.data(), localHits.size()});
        if (snapshotGeneration != m_requestedGeneration.load() ||
            m_validatedContentGeneration != m_requestedContentGeneration.load() || m_modelPreDestroyPending.load()) { return unsupported; }
        if (local.m_status == RPI::ModelRayQueryStatus::Unavailable) { return unsupported; }
        if (local.m_status != RPI::ModelRayQueryStatus::Complete && local.m_status != RPI::ModelRayQueryStatus::Truncated)
        {
            return invalid;
        }
        if (local.m_hitCount > localHits.size()) { return invalid; }
        AZStd::vector<SD::SurfaceRayHit> scratch(local.m_hitCount);
        for (size_t i = 0; i < local.m_hitCount; ++i)
        {
            const auto& hit = localHits[i];
            // TransformVector(n / scale) differs from L^-T*n only by positive uniformScale^2.
            // The determinant sign preserves world triangle winding even through a reflection.
            AZ::Vector3 normal = m_meshWorldTM.TransformVector(hit.m_normal / scale);
            if (reflected) { normal = -normal; }
            const float normalLengthSq = normal.GetLengthSq();
            if (!normal.IsFinite() || !std::isfinite(normalLengthSq) || normalLengthSq <= 0.0f ||
                !std::isfinite(hit.m_distanceNormalized) || hit.m_distanceNormalized < 0.0f || hit.m_distanceNormalized > 1.0f)
            {
                return invalid;
            }
            auto& result = scratch[i];
            result.m_worldPosition = request.m_startWorldPosition + segment * hit.m_distanceNormalized;
            result.m_worldNormal = normal.GetNormalized();
            result.m_distance = worldLength * hit.m_distanceNormalized;
            SetModelHitIdentity(*model, m_meshAssetData.GetId(), m_meshSourceRevision, owner,
                hit.m_meshIndex, hit.m_primitiveIndex, hit.m_materialSlotId, result);
            if (!result.m_worldPosition.IsFinite() || !result.m_worldNormal.IsFinite() || !std::isfinite(result.m_distance))
            {
                return invalid;
            }
        }
        if (snapshotGeneration != m_requestedGeneration.load() ||
            m_validatedContentGeneration != m_requestedContentGeneration.load() || m_modelPreDestroyPending.load()) { return unsupported; }
        if (!scratch.empty()) { AZStd::copy(scratch.begin(), scratch.end(), output.begin()); }
        SD::SurfaceRayQueryResult result;
        result.m_status = local.m_status == RPI::ModelRayQueryStatus::Truncated
            ? SD::SurfaceRayQueryStatus::Truncated : SD::SurfaceRayQueryStatus::Complete;
        result.m_hitCount = local.m_hitCount;
        result.m_discardedHitCount = local.m_discardedHitCount;
        result.m_supportedProviderCount = 1;
        return result;
    }

    JE::Geometry::SurfaceData::SurfaceRayQueryResult SurfaceDataMeshComponent::QueryVolume(
        const JE::Geometry::SurfaceData::SurfaceVolumeRequest& request,
        AZStd::span<JE::Geometry::SurfaceData::SurfaceRayHit> output) const
    {
        namespace SD = JE::Geometry::SurfaceData;
        SD::SurfaceRayQueryResult invalid; invalid.m_status = SD::SurfaceRayQueryStatus::InvalidInput;
        if (!request.IsValid() || (!output.empty() && output.data() == nullptr) ||
            output.size() > AZStd::numeric_limits<size_t>::max() / sizeof(SD::SurfaceRayHit)) { return invalid; }
        SD::SurfaceRayQueryResult unsupported; unsupported.m_status = SD::SurfaceRayQueryStatus::Unsupported;
        unsupported.m_unsupportedProviderCount = 1;
        const AZ::EntityComponentIdPair owner(GetEntityId(), GetId());
        if (request.m_filter == SD::SurfaceRayQueryFilter::ExactOwner && request.m_owner != owner) { return unsupported; }
        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
        const auto generation = m_publishedGeneration.load();
        const auto current = [&]()
        {
            return generation == m_requestedGeneration.load() &&
                m_validatedContentGeneration == m_requestedContentGeneration.load() && !m_modelPreDestroyPending.load();
        };
        if (!current()) { return unsupported; }
        const auto* model = m_meshAssetData.GetAs<RPI::ModelAsset>();
        if (!model || !m_meshAssetData.IsReady() || !m_meshAssetData.GetId().IsValid()) { return unsupported; }
        AZStd::vector<RPI::ModelVolumeHit> geometry(output.size());
        const auto local = model->WorldVolumeIntersectionsAgainstModel(request.m_worldBounds,
            m_meshWorldTM, m_meshNonUniformScale, true, {geometry.data(), geometry.size()});
        if (!current() || local.m_status == RPI::ModelRayQueryStatus::Unavailable) { return unsupported; }
        if ((local.m_status != RPI::ModelRayQueryStatus::Complete && local.m_status != RPI::ModelRayQueryStatus::Truncated) ||
            local.m_hitCount > geometry.size()) { return invalid; }
        AZStd::vector<SD::SurfaceRayHit> scratch(local.m_hitCount);
        for (size_t i = 0; i < local.m_hitCount; ++i)
        {
            const auto& source = geometry[i]; auto& hit = scratch[i];
            hit.m_worldPosition = source.m_worldPosition;
            hit.m_worldNormal = source.m_worldNormal;
            hit.m_distance = source.m_distance;
            if (!request.m_worldBounds.Contains(hit.m_worldPosition) || !hit.m_worldPosition.IsFinite() ||
                !hit.m_worldNormal.IsFinite() || !std::isfinite(hit.m_worldNormal.GetLengthSq()) || hit.m_worldNormal.GetLengthSq() <= 0 ||
                !std::isfinite(hit.m_distance) || hit.m_distance < 0) { return invalid; }
            SetModelHitIdentity(*model, m_meshAssetData.GetId(), m_meshSourceRevision, owner,
                source.m_meshIndex, source.m_primitiveIndex, source.m_materialSlotId, hit);
        }
        if (!current()) { return unsupported; }
        if (!scratch.empty()) { AZStd::copy(scratch.begin(), scratch.end(), output.begin()); }
        SD::SurfaceRayQueryResult result;
        result.m_status = local.m_status == RPI::ModelRayQueryStatus::Truncated ? SD::SurfaceRayQueryStatus::Truncated : SD::SurfaceRayQueryStatus::Complete;
        result.m_hitCount = local.m_hitCount; result.m_discardedHitCount = local.m_discardedHitCount;
        result.m_supportedProviderCount = 1;
        return result;
    }

    void SurfaceDataMeshComponent::GetSurfacePoints(const AZ::Vector3& inPosition, JE::Geometry::SurfaceData::SurfacePointList& surfacePointList) const
    {
        GetSurfacePointsFromList(AZStd::span<const AZ::Vector3>(&inPosition, 1), surfacePointList);
    }

    void SurfaceDataMeshComponent::GetSurfacePointsFromList(
        AZStd::span<const AZ::Vector3> inPositions, JE::Geometry::SurfaceData::SurfacePointList& surfacePointList) const
    {
        AZ::Vector3 hitPosition;
        AZ::Vector3 hitNormal;

        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);

        JE::Render::RPI::ModelAsset* mesh = m_meshAssetData.GetAs<JE::Render::RPI::ModelAsset>();
        if (!mesh)
        {
            return;
        }

        for (auto& inPosition : inPositions)
        {
            // test AABB as first pass to claim the point
            if (JE::Geometry::SurfaceData::AabbContains2D(m_meshBounds, inPosition))
            {
                const AZ::Vector3 rayStart =
                    AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_meshBounds.GetMax().GetZ() + s_rayAABBHeightPadding);
                const AZ::Vector3 rayEnd =
                    AZ::Vector3(inPosition.GetX(), inPosition.GetY(), m_meshBounds.GetMin().GetZ() - s_rayAABBHeightPadding);
                bool rayHit = JE::Geometry::SurfaceData::GetMeshRayIntersection(
                    *mesh, m_meshWorldTM, m_meshWorldTMInverse, m_meshNonUniformScale, rayStart, rayEnd, hitPosition, hitNormal);

                if (rayHit)
                {
                    surfacePointList.AddSurfacePoint(GetEntityId(), inPosition, hitPosition, hitNormal, m_newPointWeights);
                }
            }
        }
    }


    AZ::Aabb SurfaceDataMeshComponent::GetSurfaceAabb() const
    {
        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
        return m_meshBounds;
    }

    JE::Geometry::SurfaceData::SurfaceTagVector SurfaceDataMeshComponent::GetSurfaceTags() const
    {
        AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
        return m_cachedTags;
    }

    void SurfaceDataMeshComponent::OnCompositionChanged()
    {
        // Notifications can arrive during snapshot capture; never clear a later request.
        // No cache/registry lock or TickBus mutation from this callback.
        m_requestedGeneration.fetch_add(1);
    }

    void SurfaceDataMeshComponent::OnModelContentChanged()
    {
        m_requestedContentGeneration.fetch_add(1);
        OnCompositionChanged();
    }

    void SurfaceDataMeshComponent::OnModelReady([[maybe_unused]] const AZ::Data::Asset<JE::Render::RPI::ModelAsset>& modelAsset, [[maybe_unused]] const AZ::Data::Instance<JE::Render::RPI::Model>& model)
    {
        m_modelPreDestroyPending.store(false);
        OnModelContentChanged();
    }

    void SurfaceDataMeshComponent::OnModelPreDestroy()
    {
        m_modelPreDestroyPending.store(true);
        OnModelContentChanged();
    }

    void SurfaceDataMeshComponent::OnTransformChanged(const AZ::Transform & local, const AZ::Transform & world)
    {
        (void)local;
        (void)world;
        OnCompositionChanged();
    }

    void SurfaceDataMeshComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        if (m_publishedGeneration.load() != m_requestedGeneration.load())
        {
            UpdateMeshData();
        }
    }

    void SurfaceDataMeshComponent::UpdateMeshData()
    {
        AZ_PROFILE_SCOPE(Entity, "SurfaceDataMeshComponent: UpdateMeshData");

        const AZ::u64 generation = m_requestedGeneration.load();
        const AZ::u64 contentGeneration = m_requestedContentGeneration.load();
        const bool preDestroyPending = m_modelPreDestroyPending.load();
        // Gather external values without holding the cache lock. A renderer callback may invalidate
        // the snapshot reentrantly here; publish nothing from that mixed capture in that case.
        AZ::Data::Asset<AZ::Data::AssetData> asset;
        JE::Render::Scene::MeshComponentRequestBus::EventResult(asset, GetEntityId(), &JE::Render::Scene::MeshComponentRequestBus::Events::GetModelAsset);
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        JE::Render::Scene::MeshComponentRequestBus::EventResult(bounds, GetEntityId(), &JE::Render::Scene::MeshComponentRequestBus::Events::GetWorldBounds);
        AZ::Transform world = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(world, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Vector3 scale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(scale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        const auto tags = m_configuration.m_tags;
        if (generation != m_requestedGeneration.load() || contentGeneration != m_requestedContentGeneration.load() ||
            preDestroyPending != m_modelPreDestroyPending.load()) { return; }
        JE::Geometry::SurfaceData::SurfaceSourceRevision sourceRevision;
        bool reuseContentSnapshot = false;
        if (!preDestroyPending)
        {
            {
                AZStd::shared_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
                // Identity alone cannot distinguish replacement/reload. A pose-only refresh reuses exactly this content snapshot.
                reuseContentSnapshot = contentGeneration == m_validatedContentGeneration &&
                    asset.Get() == m_meshAssetData.Get() && asset.GetId() == m_meshAssetData.GetId();
                if (reuseContentSnapshot) { sourceRevision = m_meshSourceRevision; }
            }
            if (!reuseContentSnapshot)
            {
                auto* model = asset.GetAs<JE::Render::RPI::ModelAsset>();
                if (model && asset.IsReady())
                {
                    // Bounded validation at the owner's quiescent content-snapshot boundary only; never on a ray or pose refresh.
                    if (const auto validated = model->ValidateContentRevision())
                    {
                        // The model codec hashes the JE.Model.PublishedContent domain. Model provenance identifies this producer;
                        // it is not the WhiteBox codec, an authored SurfaceId or the internal capture generation.
                        sourceRevision.m_schema = validated->m_schema;
                        sourceRevision.m_digest = validated->m_digest;
                    }
                }
            }
        }
        bool meshValidBeforeUpdate = false;
        bool meshValidAfterUpdate = false;
        {
            AZStd::unique_lock<decltype(m_cacheMutex)> lock(m_cacheMutex);
            if (generation != m_requestedGeneration.load() || contentGeneration != m_requestedContentGeneration.load() ||
                preDestroyPending != m_modelPreDestroyPending.load()) { return; }
            meshValidBeforeUpdate = (m_meshAssetData.GetAs<JE::Render::RPI::ModelAsset>() != nullptr) && m_meshBounds.IsValid();
            m_meshAssetData = AZStd::move(asset);
            m_validatedContentGeneration = contentGeneration;
            m_meshSourceRevision = sourceRevision;
            m_meshBounds = bounds;
            m_meshWorldTM = world;
            m_meshWorldTMInverse = world.IsFinite() && world.GetUniformScale() != 0.0f
                ? world.GetInverse() : AZ::Transform::CreateIdentity();
            m_meshNonUniformScale = scale;
            m_cachedTags = tags;
            m_newPointWeights.AssignSurfaceTagWeights(m_cachedTags, 1.0f);
            meshValidAfterUpdate = (m_meshAssetData.GetAs<JE::Render::RPI::ModelAsset>() != nullptr) && m_meshBounds.IsValid();
            m_publishedGeneration.store(generation);
        }

        JE::Geometry::SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_componentId = GetId();
        registryEntry.m_queryCapabilities = static_cast<JE::Geometry::SurfaceData::SurfaceDataQueryCapabilities>(
            static_cast<AZ::u8>(JE::Geometry::SurfaceData::SurfaceDataQueryCapabilities::PointsAndRays) |
            static_cast<AZ::u8>(JE::Geometry::SurfaceData::SurfaceDataQueryCapabilities::Volumes));
        registryEntry.m_bounds = GetSurfaceAabb();
        registryEntry.m_tags = GetSurfaceTags();
        registryEntry.m_maxPointsCreatedPerInput = 1;

        if (!meshValidBeforeUpdate && !meshValidAfterUpdate)
        {
            // We didn't have a valid mesh asset before or after running this, so do nothing.
        }
        else if (!meshValidBeforeUpdate && meshValidAfterUpdate)
        {
            // Our mesh has become valid, so register as a provider and save off the provider handle
            AZ_Assert((m_providerHandle == JE::Geometry::SurfaceData::InvalidSurfaceDataRegistryHandle), "Surface data handle is initialized before our mesh became active");
            AZ_Assert(m_meshBounds.IsValid(), "Mesh Geometry isn't correctly initialized.");
            m_providerHandle = AZ::Interface<JE::Geometry::SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataProvider(registryEntry);

            // Start listening for surface data events
            AZ_Assert((m_providerHandle != JE::Geometry::SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            JE::Geometry::SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
        }
        else if (meshValidBeforeUpdate && !meshValidAfterUpdate)
        {
            // Our mesh has stopped being valid, so unregister and stop listening for surface data events
            AZ_Assert((m_providerHandle != JE::Geometry::SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ::Interface<JE::Geometry::SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            m_providerHandle = JE::Geometry::SurfaceData::InvalidSurfaceDataRegistryHandle;

            JE::Geometry::SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        }
        else if (meshValidBeforeUpdate && meshValidAfterUpdate)
        {
            // Our mesh was valid before and after, it just changed in some way, so update our registry entry.
            AZ_Assert((m_providerHandle != JE::Geometry::SurfaceData::InvalidSurfaceDataRegistryHandle), "Invalid surface data handle");
            AZ::Interface<JE::Geometry::SurfaceData::SurfaceDataSystem>::Get()->UpdateSurfaceDataProvider(m_providerHandle, registryEntry);
        }
    }
}
