/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/limits.h>
#include <cmath>
#include <charconv>
#include <AzFramework/Translation/TranslationDef.h>

#include <JE/Geometry/SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <JE/Geometry/SurfaceData/SurfaceDataConstants.h>
#include <JE/Geometry/SurfaceData/SurfaceTag.h>
#include <JE/Geometry/SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <JE/Geometry/SurfaceData/SurfaceDataProviderRequestBus.h>
#include <JE/Geometry/SurfaceData/Utility/SurfaceDataUtility.h>
#include <SurfaceData/SurfaceDataProfiler.h>

AZ_DEFINE_BUDGET(JE_Geometry_SurfaceData);

namespace JE::Geometry::SurfaceData
{
#if !defined(AZ_RELEASE_BUILD)
    namespace
    {
        const char* SurfaceRayStatusName(SurfaceRayQueryStatus status)
        {
            switch (status)
            {
            case SurfaceRayQueryStatus::Complete: return "Complete";
            case SurfaceRayQueryStatus::Truncated: return "Truncated";
            case SurfaceRayQueryStatus::InvalidInput: return "InvalidInput";
            case SurfaceRayQueryStatus::Unsupported: return "Unsupported";
            case SurfaceRayQueryStatus::Partial: return "Partial";
            }
            return "Unknown";
        }

        const char* SurfaceBatchCompletionName(SurfaceRayBatchCompletion completion)
        {
            switch (completion)
            {
            case SurfaceRayBatchCompletion::Finished: return "Finished";
            case SurfaceRayBatchCompletion::InvalidInput: return "InvalidInput";
            case SurfaceRayBatchCompletion::Cancelled: return "Cancelled";
            case SurfaceRayBatchCompletion::BudgetExhausted: return "BudgetExhausted";
            case SurfaceRayBatchCompletion::Unsupported: return "Unsupported";
            }
            return "Unknown";
        }

        void PrintSurfaceRayHit(size_t index, const SurfaceRayHit& hit)
        {
            AZ_Printf("JE.SurfaceData",
                "hit=%zu entity=%s component=%llu distance=%.9g position=(%.9g,%.9g,%.9g) normal=(%.9g,%.9g,%.9g) "
                "surface=%s face=%s region=%s primitive=%zu\n",
                index, hit.m_entityAndComponent.GetEntityId().ToString().c_str(),
                static_cast<unsigned long long>(hit.m_entityAndComponent.GetComponentId()), hit.m_distance,
                hit.m_worldPosition.GetX(), hit.m_worldPosition.GetY(), hit.m_worldPosition.GetZ(),
                hit.m_worldNormal.GetX(), hit.m_worldNormal.GetY(), hit.m_worldNormal.GetZ(),
                hit.m_surfaceId.ToString<AZStd::string>().c_str(), hit.m_faceId.ToString<AZStd::string>().c_str(),
                hit.m_regionId ? hit.m_regionId->ToString<AZStd::string>().c_str() : "unavailable", hit.m_primitiveIndex);
            const auto& revision = hit.m_sourceRevision;
            AZ_Printf("JE.SurfaceData", "hit=%zu revision=%s schema=%u digest=%08x%08x%08x%08x%08x material=%s\n",
                index, hit.m_revisionState == SurfaceSourceRevisionState::Content ? "Content" : "Unavailable",
                revision.m_schema, revision.m_digest[0], revision.m_digest[1], revision.m_digest[2],
                revision.m_digest[3], revision.m_digest[4],
                hit.m_materialSlot ? AZStd::to_string(*hit.m_materialSlot).c_str() : "unavailable");
            if (hit.m_modelProvenance)
            {
                const auto& model = *hit.m_modelProvenance;
                AZ_Printf("JE.SurfaceData", "hit=%zu model=%s lod=%u submesh=%u\n", index,
                    model.m_modelAssetId.ToString<AZStd::string>().c_str(), model.m_lodIndex, model.m_submeshIndex);
            }
        }

        void DebugSurfaceRay(const AZ::ConsoleCommandContainer& arguments)
        {
            AZStd::array<float, 6> coordinates{};
            bool valid = arguments.size() == coordinates.size();
            for (size_t index = 0; valid && index < coordinates.size(); ++index)
            {
                const auto text = arguments[index];
                const auto parsed = std::from_chars(text.data(), text.data() + text.size(), coordinates[index]);
                valid = parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && std::isfinite(coordinates[index]);
            }
            if (!valid)
            {
                AZ_Printf("JE.SurfaceData", "status=InvalidInput; usage: je_surfaceRay startX startY startZ endX endY endZ\n");
                return;
            }
            const auto* system = AZ::Interface<SurfaceDataSystem>::Get();
            if (!system)
            {
                AZ_Printf("JE.SurfaceData", "status=Unavailable; SurfaceData system is inactive\n");
                return;
            }
            SurfaceRayRequest request;
            request.m_startWorldPosition = {coordinates[0], coordinates[1], coordinates[2]};
            request.m_endWorldPosition = {coordinates[3], coordinates[4], coordinates[5]};
            AZStd::array<SurfaceRayHit, 8> hits{};
            const auto result = system->QueryRay(request, hits);
            const char* status = SurfaceRayStatusName(result.m_status);
            AZ_Printf("JE.SurfaceData", "status=%s hits=%zu discarded=%zu supported=%zu unsupported=%zu capacity=%zu\n",
                status, result.m_hitCount, result.m_discardedHitCount, result.m_supportedProviderCount,
                result.m_unsupportedProviderCount, hits.size());
            for (size_t index = 0; index < result.m_hitCount; ++index)
            {
                PrintSurfaceRayHit(index, hits[index]);
            }
        }
        AZ_CONSOLEFREEFUNC("je_surfaceRay", DebugSurfaceRay, AZ::ConsoleFunctorFlags::DontReplicate,
            "Read-only double-sided world segment query; reports up to eight nearest hits and explicit truncation.");

        void DebugSurfaceSupport(const AZ::ConsoleCommandContainer& arguments)
        {
            AZStd::array<float, 10> values{};
            bool valid = arguments.size() == values.size();
            for (size_t index = 0; valid && index < values.size(); ++index)
            {
                const auto text = arguments[index];
                const auto parsed = std::from_chars(text.data(), text.data() + text.size(), values[index]);
                valid = parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && std::isfinite(values[index]);
            }
            if (!valid)
            {
                AZ_Printf("JE.SurfaceData", "support=InvalidInput; usage: je_surfaceSupport startX startY startZ endX endY endZ normalX normalY normalZ minDot\n");
                return;
            }
            const auto* system = AZ::Interface<SurfaceDataSystem>::Get();
            if (!system)
            {
                AZ_Printf("JE.SurfaceData", "support=Indeterminate completion=Unsupported query=Unsupported system=inactive\n");
                return;
            }
            SurfaceSupportRequest request;
            request.m_ray.m_startWorldPosition = {values[0], values[1], values[2]};
            request.m_ray.m_endWorldPosition = {values[3], values[4], values[5]};
            request.m_requiredWorldNormal = {values[6], values[7], values[8]};
            request.m_minNormalDot = values[9];
            AZStd::array<SurfaceRayHit, 64> candidates{};
            const SurfaceRayBatchLimits limits{1, 4096, candidates.size(), nullptr};
            const auto result = system->QuerySupport(request, candidates, limits);
            const char* support = "Unknown";
            switch (result.m_status)
            {
            case SurfaceSupportStatus::Found: support = "Found"; break;
            case SurfaceSupportStatus::NoSupport: support = "NoSupport"; break;
            case SurfaceSupportStatus::Indeterminate: support = "Indeterminate"; break;
            case SurfaceSupportStatus::InvalidInput: support = "InvalidInput"; break;
            }
            AZ_Printf("JE.SurfaceData", "support=%s completion=%s query=%s hits=%zu discarded=%zu providerVisits=%zu capacity=%zu\n",
                support, SurfaceBatchCompletionName(result.m_completion), SurfaceRayStatusName(result.m_query.m_status),
                result.m_query.m_hitCount, result.m_query.m_discardedHitCount, result.m_providerVisitCount, candidates.size());
            if (result.m_hit) { PrintSurfaceRayHit(0, *result.m_hit); }
        }
        AZ_CONSOLEFREEFUNC("je_surfaceSupport", DebugSurfaceSupport, AZ::ConsoleFunctorFlags::DontReplicate,
            "Read-only oriented point support; explicit normal and cosine [0,1]. Not footprint, volume or prefab clearance.");

        void DebugSurfaceVolume(const AZ::ConsoleCommandContainer& arguments)
        {
            AZStd::array<float, 6> coordinates{};
            bool valid = arguments.size() == coordinates.size();
            for (size_t index = 0; valid && index < coordinates.size(); ++index)
            {
                const auto text = arguments[index];
                const auto parsed = std::from_chars(text.data(), text.data() + text.size(), coordinates[index]);
                valid = parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && std::isfinite(coordinates[index]);
            }
            valid = valid && coordinates[3] > coordinates[0] && coordinates[4] > coordinates[1] && coordinates[5] > coordinates[2];
            if (!valid)
            {
                AZ_Printf("JE.SurfaceData", "volume=InvalidInput; usage: je_surfaceVolume minX minY minZ maxX maxY maxZ\n");
                return;
            }
            const auto* system = AZ::Interface<SurfaceDataSystem>::Get();
            if (!system) { AZ_Printf("JE.SurfaceData", "volume=Unsupported completion=Unsupported system=inactive\n"); return; }
            SurfaceVolumeRequest request;
            request.m_worldBounds = AZ::Aabb::CreateFromMinMax({coordinates[0], coordinates[1], coordinates[2]},
                {coordinates[3], coordinates[4], coordinates[5]});
            AZStd::array<SurfaceRayHit, 64> hits{};
            const SurfaceVolumeLimits limits{4096, hits.size(), nullptr};
            const auto result = system->QueryVolume(request, hits, limits);
            AZ_Printf("JE.SurfaceData",
                "volume=%s completion=%s hits=%zu discarded=%zu supported=%zu unsupported=%zu providerVisits=%zu capacity=%zu distanceBasis=boxCenter\n",
                SurfaceRayStatusName(result.m_query.m_status), SurfaceBatchCompletionName(result.m_completion),
                result.m_query.m_hitCount, result.m_query.m_discardedHitCount, result.m_query.m_supportedProviderCount,
                result.m_query.m_unsupportedProviderCount, result.m_providerVisitCount, hits.size());
            for (size_t index = 0; index < result.m_query.m_hitCount; ++index) { PrintSurfaceRayHit(index, hits[index]); }
        }
        AZ_CONSOLEFREEFUNC("je_surfaceVolume", DebugSurfaceVolume, AZ::ConsoleFunctorFlags::DontReplicate,
            "Read-only closed AABB/surface intersections. Not solid occupancy, penetration, minimum distance or prefab clearance.");
    }
#endif

    void SurfaceDataSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceTag::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SurfaceDataSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SurfaceDataSystemComponent>(
                    QT_TRANSLATE_NOOP("SurfaceData", "Surface Data System"),
                    QT_TRANSLATE_NOOP("SurfaceData", "Manages registration of surface data providers and forwards intersection data requests to them"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfacePointList>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ;

            behaviorContext->Class<SurfaceDataSystemComponent>()
                ->RequestBus("SurfaceDataSystemRequestBus")
                ;

            behaviorContext->EBus<SurfaceDataSystemRequestBus>("SurfaceDataSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Event(
                    "GetSurfacePoints",
                    [](JE::Geometry::SurfaceData::SurfaceDataSystem* handler, const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags) -> AZStd::vector<AzFramework::SurfaceData::SurfacePoint>
                    {
                        AZStd::vector<AzFramework::SurfaceData::SurfacePoint> result;
                        JE::Geometry::SurfaceData::SurfacePointList surfacePointList;
                        handler->GetSurfacePoints(inPosition, desiredTags, surfacePointList);
                        surfacePointList.EnumeratePoints(
                        [&result](
                            [[maybe_unused]] size_t inPositionIndex, const AZ::Vector3& position, const AZ::Vector3& normal, const JE::Geometry::SurfaceData::SurfaceTagWeights& masks)-> bool
                        {
                            AzFramework::SurfaceData::SurfacePoint point;
                            point.m_position = position;
                            point.m_normal = normal;
                            point.m_surfaceTags = masks.GetSurfaceTagWeightList();

                            result.emplace_back(point);
                            return true;
                        });
                        return result;
                    })
                ->Event("RefreshSurfaceData", &SurfaceDataSystemRequestBus::Events::RefreshSurfaceData)
                ->Event("GetSurfaceDataProviderHandle", &SurfaceDataSystemRequestBus::Events::GetSurfaceDataProviderHandle)
                ->Event("GetSurfaceDataModifierHandle", &SurfaceDataSystemRequestBus::Events::GetSurfaceDataModifierHandle)
                ;

            behaviorContext->EBus<SurfaceDataSystemNotificationBus>("SurfaceDataSystemNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Event("OnSurfaceChanged", &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged)
                ;
        }
    }

    void SurfaceDataSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SurfaceDataSystemService"));
    }

    void SurfaceDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SurfaceDataSystemService"));
    }

    void SurfaceDataSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void SurfaceDataSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SurfaceDataSystemComponent::Init()
    {
    }

    void SurfaceDataSystemComponent::Activate()
    {
#if !defined(AZ_RELEASE_BUILD)
        if (auto* console = AZ::Interface<AZ::IConsole>::Get())
        {
            console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
        }
#endif
        AZ::Interface<SurfaceDataSystem>::Register(this);
        SurfaceDataSystemRequestBus::Handler::BusConnect();
    }

    void SurfaceDataSystemComponent::Deactivate()
    {
        SurfaceDataSystemRequestBus::Handler::BusDisconnect();
        AZ::Interface<SurfaceDataSystem>::Unregister(this);
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry)
    {
        const SurfaceDataRegistryHandle handle = RegisterSurfaceDataProviderInternal(entry);
        if (handle != InvalidSurfaceDataRegistryHandle)
        {
            // Get the set of surface tags that can be affected by adding the surface data provider.
            // This includes all of the provider's tags, as well as any surface modifier tags that exist in the bounds,
            // because new surface points have the potential of getting the modifier tags applied as well.
            SurfaceTagSet affectedSurfaceTags = GetAffectedSurfaceTags(entry.m_bounds, entry.m_tags);

            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause a full refresh for any system listening, instead of just a refresh within the bounds.
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds,
                affectedSurfaceTags);
        }
        return handle;
    }

    void SurfaceDataSystemComponent::UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle)
    {
        const SurfaceDataRegistryEntry entry = UnregisterSurfaceDataProviderInternal(handle);
        if (entry.m_entityId.IsValid())
        {
            // Get the set of surface tags that can be affected by adding the surface data provider.
            // This includes all of the provider's tags, as well as any surface modifier tags that exist in the bounds,
            // because the removed surface points have the potential of getting the modifier tags applied as well.
            SurfaceTagSet affectedSurfaceTags = GetAffectedSurfaceTags(entry.m_bounds, entry.m_tags);

            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause a full refresh for any system listening, instead of just a refresh within the bounds.
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds,
                affectedSurfaceTags);
        }
    }

    void SurfaceDataSystemComponent::UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry)
    {
        AZ::Aabb oldBounds = AZ::Aabb::CreateNull();

        if (UpdateSurfaceDataProviderInternal(handle, entry, oldBounds))
        {
            // Get the set of surface tags that can be affected by adding the surface data provider.
            // This includes all of the provider's tags, as well as any surface modifier tags that exist in the bounds,
            // because the affected surface points have the potential of getting the modifier tags applied as well.
            // For now, we'll just merge the old and new bounds into a larger region. If this causes too much refreshing to occur,
            // this could eventually be improved by getting the tags from both sets of bounds separately and combining them.
            AZ::Aabb surfaceTagBounds = oldBounds;
            surfaceTagBounds.AddAabb(entry.m_bounds);
            SurfaceTagSet affectedSurfaceTags = GetAffectedSurfaceTags(surfaceTagBounds, entry.m_tags);

            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, oldBounds, entry.m_bounds,
                affectedSurfaceTags);
        }
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry)
    {
        const SurfaceDataRegistryHandle handle = RegisterSurfaceDataModifierInternal(entry);
        if (handle != InvalidSurfaceDataRegistryHandle)
        {
            // Get the set of surface tags that can be affected by adding a surface data modifier. Since this doesn't create
            // any new surface points, we only need to broadcast the modifier tags themselves as the ones that changed.
            const SurfaceTagSet affectedSurfaceTags = ConvertTagVectorToSet(entry.m_tags);

            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause a full refresh for any system listening, instead of just a refresh within the bounds.
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId,
                entry.m_bounds, entry.m_bounds, affectedSurfaceTags);
        }
        return handle;
    }

    void SurfaceDataSystemComponent::UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle)
    {
        const SurfaceDataRegistryEntry entry = UnregisterSurfaceDataModifierInternal(handle);
        if (entry.m_entityId.IsValid())
        {
            // Get the set of surface tags that can be affected by removing a surface data modifier. Since this doesn't create
            // any new surface points, we only need to broadcast the modifier tags themselves as the ones that changed.
            const SurfaceTagSet affectedSurfaceTags = ConvertTagVectorToSet(entry.m_tags);

            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause a full refresh for any system listening, instead of just a refresh within the bounds.
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds,
                affectedSurfaceTags);
        }
    }

    void SurfaceDataSystemComponent::UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry)
    {
        AZ::Aabb oldBounds = AZ::Aabb::CreateNull();

        // Get the previous set of surface tags for this modifier.
        SurfaceTagSet affectedSurfaceTags;
        {
            AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
            auto entryItr = m_registeredSurfaceDataModifiers.find(handle);
            if (entryItr != m_registeredSurfaceDataModifiers.end())
            {
                affectedSurfaceTags = ConvertTagVectorToSet(entryItr->second.m_tags);
            }
        }

        // Merge in the new set of surface tags for this modifier. Since modifiers don't create
        // any new surface points, we only need to broadcast the modifier tags themselves as the ones that changed.
        for (auto& tag : entry.m_tags)
        {
            affectedSurfaceTags.emplace(tag);
        }

        if (UpdateSurfaceDataModifierInternal(handle, entry, oldBounds))
        {
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, oldBounds, entry.m_bounds,
                affectedSurfaceTags);
        }
    }

    void SurfaceDataSystemComponent::RefreshSurfaceData(const SurfaceDataRegistryHandle& providerHandle, const AZ::Aabb& dirtyBounds)
    {
        auto entryItr = m_registeredSurfaceDataProviders.find(providerHandle);
        if (entryItr != m_registeredSurfaceDataProviders.end())
        {
            // Get the set of surface tags that can be affected by refreshing a surface data provider.
            // This includes all of the provider's tags, as well as any surface modifier tags that exist in the bounds,
            // because the affected surface points have the potential of getting the modifier tags applied as well.
            SurfaceTagSet affectedSurfaceTags = GetAffectedSurfaceTags(dirtyBounds, entryItr->second.m_tags);

            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, AZ::EntityId(), dirtyBounds, dirtyBounds, affectedSurfaceTags);
        }

    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::GetSurfaceDataProviderHandle(const AZ::EntityId& providerEntityId)
    {
        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        for (auto& [providerHandle, providerEntry] : m_registeredSurfaceDataProviders)
        {
            if (providerEntry.m_entityId == providerEntityId)
            {
                return providerHandle;
            }
        }
        return {};
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::GetSurfaceDataModifierHandle(const AZ::EntityId& modifierEntityId)
    {
        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        for (auto& [modifierHandle, modifierEntry] : m_registeredSurfaceDataModifiers)
        {
            if (modifierEntry.m_entityId == modifierEntityId)
            {
                return modifierHandle;
            }
        }
        return {};
    }

    namespace
    {
        bool SurfaceOwnerKeyBefore(const SurfaceRayHit& a, const SurfaceRayHit& b)
        {
            if (a.m_entityAndComponent.GetEntityId() != b.m_entityAndComponent.GetEntityId())
            { return a.m_entityAndComponent.GetEntityId() < b.m_entityAndComponent.GetEntityId(); }
            if (a.m_entityAndComponent.GetComponentId() != b.m_entityAndComponent.GetComponentId())
            { return a.m_entityAndComponent.GetComponentId() < b.m_entityAndComponent.GetComponentId(); }
            // An absent model address stays explicit and precedes a present one; it never invents LOD/submesh zero.
            if (a.m_modelProvenance.has_value() != b.m_modelProvenance.has_value())
            { return !a.m_modelProvenance.has_value(); }
            if (a.m_modelProvenance)
            {
                if (a.m_modelProvenance->m_lodIndex != b.m_modelProvenance->m_lodIndex)
                { return a.m_modelProvenance->m_lodIndex < b.m_modelProvenance->m_lodIndex; }
                if (a.m_modelProvenance->m_submeshIndex != b.m_modelProvenance->m_submeshIndex)
                { return a.m_modelProvenance->m_submeshIndex < b.m_modelProvenance->m_submeshIndex; }
            }
            return a.m_primitiveIndex < b.m_primitiveIndex;
        }

        // A synchronous checkpoint, never a caller callback. One geometry call is not preempted.
        bool ContinueRayBatch(const SurfaceRayBatchLimits* limits, SurfaceRayBatchResult* batch, bool visit = false)
        {
            if (!limits) { return true; }
            if (limits->m_cancellationFlag && limits->m_cancellationFlag->load(AZStd::memory_order_relaxed))
            { batch->m_completion = SurfaceRayBatchCompletion::Cancelled; return false; }
            if (visit)
            {
                if (batch->m_providerVisitCount == limits->m_maxProviderVisits)
                { batch->m_completion = SurfaceRayBatchCompletion::BudgetExhausted; return false; }
                ++batch->m_providerVisitCount;
            }
            return true;
        }
    }

    bool SurfaceDataSystemComponent::IsValidRayRequest(const SurfaceRayRequest& request)
    {
        const float length = (request.m_endWorldPosition - request.m_startWorldPosition).GetLength();
        return request.m_startWorldPosition.IsFinite() && request.m_endWorldPosition.IsFinite() &&
            std::isfinite(length) && length > 0.0f &&
            (request.m_sidedness == SurfaceRaySidedness::FrontFace || request.m_sidedness == SurfaceRaySidedness::DoubleSided) &&
            (request.m_filter == SurfaceRayQueryFilter::AllRegisteredProviders || request.m_filter == SurfaceRayQueryFilter::ExactOwner) &&
            (request.m_filter != SurfaceRayQueryFilter::ExactOwner ||
                (request.m_owner.GetEntityId().IsValid() && request.m_owner.GetComponentId() != AZ::InvalidComponentId));
    }

    bool SurfaceDataSystemComponent::GatherRayCandidates(AZStd::span<const SurfaceRayBatchProbe> probes, const AZ::Aabb& region,
        AZStd::vector<SurfaceDataRegistryHandle>& candidates, const SurfaceRayBatchLimits* limits, SurfaceRayBatchResult* batch) const
    {
        if (!ContinueRayBatch(limits, batch)) { return false; }
        const bool global = AZStd::any_of(probes.begin(), probes.end(), [](const auto& probe)
            { return probe.m_ray.m_filter == SurfaceRayQueryFilter::AllRegisteredProviders; });
        for (const auto& [handle, entry] : m_registeredSurfaceDataProviders)
        {
            if (!ContinueRayBatch(limits, batch, true)) { return false; }
            const bool declaredOwner = AZStd::any_of(probes.begin(), probes.end(), [&entry](const auto& probe)
            {
                return probe.m_ray.m_filter == SurfaceRayQueryFilter::ExactOwner &&
                    probe.m_ray.m_owner.GetEntityId() == entry.m_entityId &&
                    (entry.m_componentId == probe.m_ray.m_owner.GetComponentId() || entry.m_componentId == AZ::InvalidComponentId);
            });
            if (!global && !declaredOwner) { continue; }
            // Batch membership is shared, spatial rejection is not: an excluded provider could become pending
            // or move into a later ray's region before that ray runs. Recheck its current bounds per ray.
            if (limits) { candidates.push_back(handle); continue; }
            AZ::Aabb bounds = entry.m_bounds;
            if (HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Rays) &&
                entry.m_componentId != AZ::InvalidComponentId)
            {
                AZStd::optional<AZ::Aabb> currentBounds;
                SurfaceDataProviderRequestBus::EventResult(currentBounds, handle, &SurfaceDataProviderRequests::GetSurfaceRayBounds);
                if (currentBounds) { bounds = *currentBounds; }
            }
            // ExactOwner must still validate its provider on a miss; unknown pending bounds remain candidates.
            if (declaredOwner || !bounds.IsValid() || bounds.Overlaps(region)) { candidates.push_back(handle); }
        }
        AZStd::sort(candidates.begin(), candidates.end(), [this](auto lhs, auto rhs)
        {
            const auto& a = m_registeredSurfaceDataProviders.at(lhs);
            const auto& b = m_registeredSurfaceDataProviders.at(rhs);
            if (a.m_entityId != b.m_entityId) { return a.m_entityId < b.m_entityId; }
            if (a.m_componentId != b.m_componentId) { return a.m_componentId < b.m_componentId; }
            return lhs < rhs;
        });
        return ContinueRayBatch(limits, batch);
    }

    SurfaceRayQueryResult SurfaceDataSystemComponent::QueryRay(
        const SurfaceRayRequest& request, AZStd::span<SurfaceRayHit> output) const
    {
        if (!IsValidRayRequest(request))
        { SurfaceRayQueryResult invalid; invalid.m_status = SurfaceRayQueryStatus::InvalidInput; return invalid; }
        // Hold through output commit. Publish must release its snapshot lock before registry updates.
        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        const SurfaceRayBatchProbe probe{request, 0, output.size()};
        const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(request.m_startWorldPosition.GetMin(request.m_endWorldPosition),
            request.m_startWorldPosition.GetMax(request.m_endWorldPosition));
        AZStd::vector<SurfaceDataRegistryHandle> candidates;
        GatherRayCandidates({&probe, 1}, bounds, candidates);
        return QueryRayInternal(request, output, candidates);
    }

    SurfaceVolumeQueryResult SurfaceDataSystemComponent::QueryVolume(const SurfaceVolumeRequest& request,
        AZStd::span<SurfaceRayHit> output, const SurfaceVolumeLimits& limits) const
    {
        SurfaceVolumeQueryResult result;
        if (!request.IsValid()) { result.m_query.m_status = SurfaceRayQueryStatus::InvalidInput; return result; }
        const SurfaceRayBatchLimits workLimits{1, limits.m_maxProviderVisits, limits.m_maxTotalHits, limits.m_cancellationFlag};
        SurfaceRayBatchResult work;
        work.m_completion = SurfaceRayBatchCompletion::Finished;
        const auto finish = [&]()
        {
            result.m_completion = work.m_completion;
            result.m_providerVisitCount = work.m_providerVisitCount;
            return result;
        };
        const auto invalid = [&]()
        {
            work.m_completion = SurfaceRayBatchCompletion::InvalidInput;
            result.m_query.m_status = SurfaceRayQueryStatus::InvalidInput;
            return finish();
        };
        if (!ContinueRayBatch(&workLimits, &work)) { return finish(); }
        const size_t capacity = AZStd::min(output.size(), limits.m_maxTotalHits);
        AZStd::vector<SurfaceRayHit> staged(capacity), providerOutput(capacity);
        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        // Reuse registration membership/order and bounded work accounting. This diagonal is a FILTER only, never a geometry query.
        SurfaceRayBatchProbe probe;
        probe.m_ray.m_startWorldPosition = request.m_worldBounds.GetMin();
        probe.m_ray.m_endWorldPosition = request.m_worldBounds.GetMax();
        probe.m_ray.m_filter = request.m_filter;
        probe.m_ray.m_owner = request.m_owner;
        AZStd::vector<SurfaceDataRegistryHandle> candidates;
        if (!GatherRayCandidates({&probe, 1}, request.m_worldBounds, candidates, &workLimits, &work)) { return finish(); }
        SurfaceRayQueryResult query;
        bool exactOwnerRegistered = false;
        for (const auto handle : candidates)
        {
            const auto& entry = m_registeredSurfaceDataProviders.at(handle);
            if (request.m_filter == SurfaceRayQueryFilter::ExactOwner)
            {
                if (entry.m_entityId != request.m_owner.GetEntityId()) { continue; }
                if (entry.m_componentId == request.m_owner.GetComponentId()) { exactOwnerRegistered = true; }
                else if (entry.m_componentId != AZ::InvalidComponentId) { continue; }
            }
            if (!ContinueRayBatch(&workLimits, &work, true)) { return finish(); }
            auto bounds = entry.m_bounds;
            // Refresh before rejection, including a ray-only model that may be pending/moving. Unknown means include.
            if (entry.m_componentId != AZ::InvalidComponentId &&
                (HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Rays) ||
                    HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Volumes)))
            {
                AZStd::optional<AZ::Aabb> currentBounds;
                SurfaceDataProviderRequestBus::EventResult(currentBounds, handle, &SurfaceDataProviderRequests::GetSurfaceRayBounds);
                if (currentBounds) { bounds = *currentBounds; }
            }
            if (request.m_filter != SurfaceRayQueryFilter::ExactOwner && bounds.IsValid() && !bounds.Overlaps(request.m_worldBounds))
            { continue; }
            if (!HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Volumes) ||
                entry.m_componentId == AZ::InvalidComponentId)
            { ++query.m_unsupportedProviderCount; continue; }
            SurfaceRayQueryResult local;
            local.m_status = SurfaceRayQueryStatus::Unsupported;
            SurfaceDataProviderRequestBus::EventResult(local, handle, &SurfaceDataProviderRequests::QueryVolume,
                request, AZStd::span<SurfaceRayHit>(providerOutput));
            if (!ContinueRayBatch(&workLimits, &work)) { return finish(); }
            if (local.m_status == SurfaceRayQueryStatus::InvalidInput) { return invalid(); }
            if (local.m_status != SurfaceRayQueryStatus::Complete && local.m_status != SurfaceRayQueryStatus::Truncated)
            { ++query.m_unsupportedProviderCount; continue; }
            if (local.m_hitCount > providerOutput.size() ||
                ((local.m_status == SurfaceRayQueryStatus::Truncated) != (local.m_discardedHitCount != 0)) ||
                local.m_discardedHitCount > AZStd::numeric_limits<size_t>::max() - query.m_discardedHitCount) { return invalid(); }
            ++query.m_supportedProviderCount;
            query.m_discardedHitCount += local.m_discardedHitCount;
            for (size_t hitIndex = 0; hitIndex < local.m_hitCount; ++hitIndex)
            {
                const auto& hit = providerOutput[hitIndex];
                if (!hit.m_worldPosition.IsFinite() || !request.m_worldBounds.Contains(hit.m_worldPosition) ||
                    !hit.m_worldNormal.IsFinite() || !std::isfinite(hit.m_worldNormal.GetLengthSq()) || hit.m_worldNormal.GetLengthSq() <= 0.0f ||
                    !std::isfinite(hit.m_distance) || hit.m_distance < 0.0f || hit.m_distance > request.m_worldBounds.GetExtents().GetLength() ||
                    hit.m_entityAndComponent != AZ::EntityComponentIdPair(entry.m_entityId, entry.m_componentId) ||
                    (hit.m_modelProvenance && !hit.m_modelProvenance->m_modelAssetId.IsValid())) { return invalid(); }
                size_t insertion = 0;
                while (insertion < query.m_hitCount && !SurfaceOwnerKeyBefore(hit, staged[insertion])) { ++insertion; }
                if (query.m_hitCount == staged.size())
                {
                    if (query.m_discardedHitCount == AZStd::numeric_limits<size_t>::max()) { return invalid(); }
                    ++query.m_discardedHitCount;
                    if (insertion == staged.size()) { continue; }
                }
                else { ++query.m_hitCount; }
                for (size_t index = query.m_hitCount - 1; index > insertion; --index) { staged[index] = staged[index - 1]; }
                staged[insertion] = hit;
            }
        }
        query.m_status = query.m_unsupportedProviderCount != 0
            ? (query.m_supportedProviderCount != 0 ? SurfaceRayQueryStatus::Partial : SurfaceRayQueryStatus::Unsupported)
            : (request.m_filter == SurfaceRayQueryFilter::ExactOwner && !exactOwnerRegistered ? SurfaceRayQueryStatus::Unsupported
                : (query.m_discardedHitCount != 0 ? SurfaceRayQueryStatus::Truncated : SurfaceRayQueryStatus::Complete));
        if (!ContinueRayBatch(&workLimits, &work)) { return finish(); }
        // Same atomic commit boundary as QueryRays; no provider call or cancellation checkpoint while committing.
        for (size_t index = 0; index < query.m_hitCount; ++index) { output[index] = staged[index]; }
        result.m_query = query;
        return finish();
    }

    SurfaceSupportResult SurfaceDataSystemComponent::QuerySupport(const SurfaceSupportRequest& request,
        AZStd::span<SurfaceRayHit> candidates, const SurfaceRayBatchLimits& limits) const
    {
        SurfaceSupportResult result;
        if (!IsValidRayRequest(request.m_ray) || !request.m_requiredWorldNormal.IsFinite() ||
            !std::isfinite(request.m_minNormalDot) || request.m_minNormalDot < 0.0f || request.m_minNormalDot > 1.0f)
        { return result; }
        // Normalize arbitrary finite nonzero directions without overflowing or underflowing their squared length.
        const float scale = AZStd::max(std::fabs(request.m_requiredWorldNormal.GetX()),
            AZStd::max(std::fabs(request.m_requiredWorldNormal.GetY()), std::fabs(request.m_requiredWorldNormal.GetZ())));
        if (scale == 0.0f) { return result; }
        const AZ::Vector3 scaledNormal(request.m_requiredWorldNormal.GetX() / scale,
            request.m_requiredWorldNormal.GetY() / scale, request.m_requiredWorldNormal.GetZ() / scale);
        const float normalLength = std::sqrt(scaledNormal.GetLengthSq());
        if (!scaledNormal.IsFinite() || !std::isfinite(normalLength) || normalLength <= 0.0f) { return result; }
        const auto requiredNormal = scaledNormal / normalLength;
        const SurfaceRayBatchProbe probe{request.m_ray, 0, candidates.size()};
        SurfaceRayQueryResult query;
        const auto batch = QueryRays({&probe, 1}, candidates, {&query, 1}, limits);
        result.m_completion = batch.m_completion;
        result.m_providerVisitCount = batch.m_providerVisitCount;
        result.m_status = batch.m_completion == SurfaceRayBatchCompletion::InvalidInput
            ? SurfaceSupportStatus::InvalidInput : SurfaceSupportStatus::Indeterminate;
        if (batch.m_completion != SurfaceRayBatchCompletion::Finished) { return result; }
        result.m_query = query;
        if (query.m_status != SurfaceRayQueryStatus::Complete) { return result; }
        result.m_status = SurfaceSupportStatus::NoSupport;
        // QueryRays already validates and orders these original hits. No provider locks/calls or cache are added here.
        for (size_t index = 0; index < query.m_hitCount; ++index)
        {
            const auto& hit = candidates[index];
            const auto normal = hit.m_worldNormal / std::sqrt(hit.m_worldNormal.GetLengthSq());
            if (requiredNormal.Dot(normal) >= request.m_minNormalDot)
            {
                result.m_hit = hit;
                result.m_status = SurfaceSupportStatus::Found;
                break;
            }
        }
        return result;
    }

    SurfaceRayBatchResult SurfaceDataSystemComponent::QueryRays(AZStd::span<const SurfaceRayBatchProbe> probes,
        AZStd::span<SurfaceRayHit> hits, AZStd::span<SurfaceRayQueryResult> rayResults, const SurfaceRayBatchLimits& limits) const
    {
        SurfaceRayBatchResult result;
        if (rayResults.size() != probes.size()) { return result; }
        if (probes.size() > limits.m_maxRayCount)
        { result.m_completion = SurfaceRayBatchCompletion::BudgetExhausted; return result; }
        size_t capacities = 0;
        AZStd::vector<size_t> slices;
        slices.reserve(probes.size());
        for (size_t i = 0; i != probes.size(); ++i)
        {
            const auto& probe = probes[i];
            if (!IsValidRayRequest(probe.m_ray) || probe.m_hitOffset > hits.size() ||
                probe.m_hitCapacity > hits.size() - probe.m_hitOffset ||
                probe.m_hitCapacity > AZStd::numeric_limits<size_t>::max() - capacities)
            { return result; }
            capacities += probe.m_hitCapacity;
            if (probe.m_hitCapacity != 0) { slices.push_back(i); }
        }
        AZStd::sort(slices.begin(), slices.end(), [&probes](size_t a, size_t b) { return probes[a].m_hitOffset < probes[b].m_hitOffset; });
        size_t end = 0;
        for (const auto i : slices)
        {
            if (probes[i].m_hitOffset < end) { return result; }
            end = probes[i].m_hitOffset + probes[i].m_hitCapacity; // Bounds/subtraction validation above makes this addition safe.
        }
        result.m_completion = SurfaceRayBatchCompletion::Finished;
        if (!ContinueRayBatch(&limits, &result)) { return result; }
        if (probes.empty()) { result.m_queryStatus = SurfaceRayQueryStatus::Complete; return result; }
        // Scratch is caller-bounded: O(maxRayCount + maxProviderVisits + maxTotalHits), with no retained cache.
        AZStd::vector<SurfaceRayHit> stagedHits(AZStd::min(capacities, limits.m_maxTotalHits));
        AZStd::vector<SurfaceRayQueryResult> stagedResults(probes.size());
        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        AZStd::vector<SurfaceDataRegistryHandle> candidates;
        if (!GatherRayCandidates(probes, AZ::Aabb::CreateNull(), candidates, &limits, &result)) { return result; }
        size_t retained = 0;
        bool supported = false, unsupported = false, truncated = false;
        for (size_t i = 0; i != probes.size(); ++i)
        {
            if (!ContinueRayBatch(&limits, &result)) { return result; }
            const size_t capacity = AZStd::min(probes[i].m_hitCapacity, stagedHits.size() - retained);
            stagedResults[i] = QueryRayInternal(probes[i].m_ray, AZStd::span<SurfaceRayHit>(stagedHits).subspan(retained, capacity),
                candidates, &limits, &result);
            if (result.m_completion != SurfaceRayBatchCompletion::Finished) { return result; }
            const auto& ray = stagedResults[i];
            if (ray.m_status == SurfaceRayQueryStatus::InvalidInput)
            { result.m_completion = SurfaceRayBatchCompletion::InvalidInput; return result; }
            retained += ray.m_hitCount;
            supported |= ray.m_status != SurfaceRayQueryStatus::Unsupported;
            unsupported |= ray.m_status == SurfaceRayQueryStatus::Unsupported || ray.m_status == SurfaceRayQueryStatus::Partial;
            truncated |= ray.m_discardedHitCount != 0;
        }
        if (!ContinueRayBatch(&limits, &result)) { return result; }
        // No cancellation checkpoints during commit: cancellation arriving after this boundary belongs to a later call.
        size_t offset = 0;
        for (size_t i = 0; i != probes.size(); ++i)
        {
            for (size_t hit = 0; hit != stagedResults[i].m_hitCount; ++hit)
            { hits[probes[i].m_hitOffset + hit] = stagedHits[offset + hit]; }
            offset += stagedResults[i].m_hitCount;
            rayResults[i] = stagedResults[i];
        }
        result.m_committedRayCount = probes.size(); result.m_hitCount = retained;
        result.m_queryStatus = unsupported ? (supported ? SurfaceRayQueryStatus::Partial : SurfaceRayQueryStatus::Unsupported)
            : (truncated ? SurfaceRayQueryStatus::Truncated : SurfaceRayQueryStatus::Complete);
        return result;
    }

    SurfaceRayQueryResult SurfaceDataSystemComponent::QueryRayInternal(const SurfaceRayRequest& request,
        AZStd::span<SurfaceRayHit> output, AZStd::span<const SurfaceDataRegistryHandle> candidates,
        const SurfaceRayBatchLimits* limits, SurfaceRayBatchResult* batch) const
    {
        SurfaceRayQueryResult invalid; invalid.m_status = SurfaceRayQueryStatus::InvalidInput;
        const float length = (request.m_endWorldPosition - request.m_startWorldPosition).GetLength();
        const AZ::Aabb rayBounds = AZ::Aabb::CreateFromMinMax(request.m_startWorldPosition.GetMin(request.m_endWorldPosition),
            request.m_startWorldPosition.GetMax(request.m_endWorldPosition));
        bool exactOwnerRegistered = false;
        AZStd::vector<SurfaceRayHit> nearest(output.size());
        AZStd::vector<SurfaceRayHit> providerOutput(output.size());
        SurfaceRayQueryResult result;
        const auto earlier = [](const SurfaceRayHit& a, const SurfaceRayHit& b)
        {
            return a.m_distance != b.m_distance ? a.m_distance < b.m_distance : SurfaceOwnerKeyBefore(a, b);
        };
        for (const auto handle : candidates)
        {
            const auto& entry = m_registeredSurfaceDataProviders.at(handle);
            if (request.m_filter == SurfaceRayQueryFilter::ExactOwner)
            {
                if (entry.m_entityId != request.m_owner.GetEntityId()) { continue; }
                if (entry.m_componentId == request.m_owner.GetComponentId()) { exactOwnerRegistered = true; }
                else if (entry.m_componentId != AZ::InvalidComponentId) { continue; }
            }
            if (!ContinueRayBatch(limits, batch, true)) { return {}; }
            // Single-ray candidates already used current bounds. Batch candidates are shared,
            // so only that path must refresh spatial eligibility for each ray.
            if (limits)
            {
                AZ::Aabb queryBounds = entry.m_bounds;
                if (HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Rays) &&
                    entry.m_componentId != AZ::InvalidComponentId)
                {
                    AZStd::optional<AZ::Aabb> currentBounds;
                    SurfaceDataProviderRequestBus::EventResult(currentBounds, handle, &SurfaceDataProviderRequests::GetSurfaceRayBounds);
                    if (currentBounds) { queryBounds = *currentBounds; }
                }
                if (request.m_filter != SurfaceRayQueryFilter::ExactOwner && queryBounds.IsValid() && !queryBounds.Overlaps(rayBounds))
                { continue; }
            }
            if (!HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Rays) ||
                entry.m_componentId == AZ::InvalidComponentId)
            {
                ++result.m_unsupportedProviderCount;
                continue;
            }
            SurfaceRayQueryResult local;
            local.m_status = SurfaceRayQueryStatus::Unsupported; // Also covers a registered entry without a bus handler.
            SurfaceDataProviderRequestBus::EventResult(local, handle, &SurfaceDataProviderRequests::QueryRay,
                request, AZStd::span<SurfaceRayHit>(providerOutput));
            if (!ContinueRayBatch(limits, batch)) { return {}; }
            if (local.m_status == SurfaceRayQueryStatus::InvalidInput) { return invalid; }
            if (local.m_status != SurfaceRayQueryStatus::Complete && local.m_status != SurfaceRayQueryStatus::Truncated)
            {
                ++result.m_unsupportedProviderCount;
                continue;
            }
            if (local.m_hitCount > providerOutput.size() ||
                ((local.m_status == SurfaceRayQueryStatus::Truncated) != (local.m_discardedHitCount != 0)) ||
                local.m_discardedHitCount > AZStd::numeric_limits<size_t>::max() - result.m_discardedHitCount)
            { return invalid; }
            ++result.m_supportedProviderCount;
            result.m_discardedHitCount += local.m_discardedHitCount;
            for (size_t i = 0; i < local.m_hitCount; ++i)
            {
                const auto& hit = providerOutput[i];
                if (!hit.m_worldPosition.IsFinite() || !hit.m_worldNormal.IsFinite() ||
                    !std::isfinite(hit.m_worldNormal.GetLengthSq()) || hit.m_worldNormal.GetLengthSq() <= 0.0f ||
                    !std::isfinite(hit.m_distance) || hit.m_distance < 0.0f || hit.m_distance > length ||
                    hit.m_entityAndComponent != AZ::EntityComponentIdPair(entry.m_entityId, entry.m_componentId) ||
                    (hit.m_modelProvenance && !hit.m_modelProvenance->m_modelAssetId.IsValid()))
                { return invalid; }
                size_t insertion = 0;
                while (insertion < result.m_hitCount && !earlier(hit, nearest[insertion])) { ++insertion; }
                if (result.m_hitCount == nearest.size())
                {
                    if (result.m_discardedHitCount == AZStd::numeric_limits<size_t>::max()) { return invalid; }
                    ++result.m_discardedHitCount;
                    if (insertion == nearest.size()) { continue; }
                }
                else { ++result.m_hitCount; }
                for (size_t index = result.m_hitCount - 1; index > insertion; --index) { nearest[index] = nearest[index - 1]; }
                nearest[insertion] = hit;
            }
        }
        if (result.m_unsupportedProviderCount != 0)
        {
            result.m_status = result.m_supportedProviderCount != 0 ? SurfaceRayQueryStatus::Partial : SurfaceRayQueryStatus::Unsupported;
        }
        else if (request.m_filter == SurfaceRayQueryFilter::ExactOwner && !exactOwnerRegistered)
        { result.m_status = SurfaceRayQueryStatus::Unsupported; }
        else if (result.m_discardedHitCount != 0) { result.m_status = SurfaceRayQueryStatus::Truncated; }
        if (!ContinueRayBatch(limits, batch)) { return {}; }
        for (size_t index = 0; index < result.m_hitCount; ++index) { output[index] = nearest[index]; }
        return result;
    }

    void SurfaceDataSystemComponent::GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointList) const
    {
        GetSurfacePointsFromListInternal(
            AZStd::span<const AZ::Vector3>(&inPosition, 1), AZ::Aabb::CreateFromPoint(inPosition), desiredTags, surfacePointList);
    }

    void SurfaceDataSystemComponent::GetSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize,
        const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointLists) const
    {
        const size_t totalQueryPositions = aznumeric_cast<size_t>(ceil(inRegion.GetXExtent() / stepSize.GetX())) *
            aznumeric_cast<size_t>(ceil(inRegion.GetYExtent() / stepSize.GetY()));

        AZStd::vector<AZ::Vector3> inPositions;
        inPositions.reserve(totalQueryPositions);

        // Initialize our list-per-position list with every input position to query from the region.
        // This is inclusive on the min sides of inRegion, and exclusive on the max sides.
        for (float y = inRegion.GetMin().GetY(); y < inRegion.GetMax().GetY(); y += stepSize.GetY())
        {
            for (float x = inRegion.GetMin().GetX(); x < inRegion.GetMax().GetX(); x += stepSize.GetX())
            {
                inPositions.emplace_back(x, y, AZ::Constants::FloatMax);
            }
        }

        GetSurfacePointsFromListInternal(inPositions, inRegion, desiredTags, surfacePointLists);
    }

    void SurfaceDataSystemComponent::GetSurfacePointsFromList(
        AZStd::span<const AZ::Vector3> inPositions,
        const SurfaceTagVector& desiredTags,
        SurfacePointList& surfacePointLists) const
    {
        AZ::Aabb inBounds = AZ::Aabb::CreateNull();
        for (auto& position : inPositions)
        {
            inBounds.AddPoint(position);
        }

        GetSurfacePointsFromListInternal(inPositions, inBounds, desiredTags, surfacePointLists);
    }

    void SurfaceDataSystemComponent::GetSurfacePointsFromListInternal(
        AZStd::span<const AZ::Vector3> inPositions, const AZ::Aabb& inPositionBounds,
        const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointLists) const
    {
        JE_SURFACE_DATA_PROFILE_FUNCTION_VERBOSE

        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        const bool useTagFilters = HasValidTags(desiredTags);
        const bool hasModifierTags = useTagFilters && HasAnyMatchingTags(desiredTags, m_registeredModifierTags);

        // Clear our output structure.
        surfacePointLists.Clear();

        auto ProviderIsApplicable = [useTagFilters, hasModifierTags, &desiredTags, &inPositionBounds]
            (const SurfaceDataRegistryEntry& provider) -> bool
        {
            if (!HasSurfaceQueryCapability(provider.m_queryCapabilities, SurfaceDataQueryCapabilities::Points)) { return false; }
            bool hasInfiniteBounds = !provider.m_bounds.IsValid();

            // Only allow surface providers that match our tag filters. However, if we aren't using tag filters,
            // or if there's at least one surface modifier that can *add* a filtered tag to a created point, then
            // allow all the surface providers.
            if (!useTagFilters || hasModifierTags || HasAnyMatchingTags(desiredTags, provider.m_tags))
            {
                // Only allow surface providers that overlap the input position area.
                if (hasInfiniteBounds || AabbOverlaps2D(provider.m_bounds, inPositionBounds))
                {
                    return true;
                }
            }

            return false;
        };

        // Gather up the subset of surface providers that overlap the input positions.
        size_t maxPointsCreatedPerInput = 0;
        for (const auto& [providerHandle, provider] : m_registeredSurfaceDataProviders)
        {
            if (ProviderIsApplicable(provider))
            {
                maxPointsCreatedPerInput += provider.m_maxPointsCreatedPerInput;
            }
        }

        // If we don't have any surface providers that will create any new surface points, then there's nothing more to do.
        if (maxPointsCreatedPerInput == 0)
        {
            return;
        }

        // Notify our output structure that we're starting to build up the list of output points.
        // This will reserve memory and allocate temporary structures to help build up the list efficiently.
        AZStd::span<const SurfaceTag> tagFilters;
        if (useTagFilters)
        {
            tagFilters = desiredTags;
        }

        {
            JE_SURFACE_DATA_PROFILE_SCOPE_VERBOSE("GetSurfacePointsFromListInternal: StartListConstruction");
            surfacePointLists.StartListConstruction(inPositions, maxPointsCreatedPerInput, tagFilters);
        }

        // Loop through each data provider and generate surface points from the set of input positions.
        // Any generated points that have the same XY coordinates and extremely similar Z values will get combined together.
        {
            JE_SURFACE_DATA_PROFILE_SCOPE_VERBOSE("GetSurfacePointsFromListInternal: GetSurfacePointsFromList");
            for (const auto& [providerHandle, provider] : m_registeredSurfaceDataProviders)
            {
                if (ProviderIsApplicable(provider))
                {
                    SurfaceDataProviderRequestBus::Event(
                        providerHandle, &SurfaceDataProviderRequestBus::Events::GetSurfacePointsFromList, inPositions, surfacePointLists);
                }
            }
        }

        // Once we have our list of surface points created, run through the list of surface data modifiers to potentially add
        // surface tags / values onto each point.  The difference between this and the above loop is that surface data *providers*
        // create new surface points, but surface data *modifiers* simply annotate points that have already been created.  The modifiers
        // are used to annotate points that occur within a volume.  A common example is marking points as "underwater" for points that occur
        // within a water volume.
        {
            JE_SURFACE_DATA_PROFILE_SCOPE_VERBOSE("GetSurfacePointsFromListInternal: ModifySurfaceWeights");
            for (const auto& [modifierHandle, modifier] : m_registeredSurfaceDataModifiers)
            {
                bool hasInfiniteBounds = !modifier.m_bounds.IsValid();

                if (hasInfiniteBounds || AabbOverlaps2D(modifier.m_bounds, surfacePointLists.GetSurfacePointAabb()))
                {
                    surfacePointLists.ModifySurfaceWeights(modifierHandle);
                }
            }
        }

        // Notify the output structure that we're done building up the list.
        // This will filter out any remaining points that don't match the desired tag list. This can happen when a surface provider
        // doesn't add a desired tag, and a surface modifier has the *potential* to add it, but then doesn't.
        // It may also compact the memory and free any temporary structures.
        surfacePointLists.EndListConstruction();
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataProviderInternal(const SurfaceDataRegistryEntry& entry)
    {
        AZ_Assert(HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Points)
                ? entry.m_maxPointsCreatedPerInput > 0
                : ((HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Rays) ||
                    HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Volumes)) && entry.m_maxPointsCreatedPerInput == 0),
            "Point providers require a positive XY capacity; geometry-only providers require zero XY capacity.");
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryHandle handle = ++m_registeredSurfaceDataProviderHandleCounter;
        m_registeredSurfaceDataProviders[handle] = entry;
        return handle;
    }

    SurfaceDataRegistryEntry SurfaceDataSystemComponent::UnregisterSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle)
    {
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryEntry entry;
        auto entryItr = m_registeredSurfaceDataProviders.find(handle);
        if (entryItr != m_registeredSurfaceDataProviders.end())
        {
            entry = entryItr->second;
            m_registeredSurfaceDataProviders.erase(entryItr);
        }
        return entry;
    }

    bool SurfaceDataSystemComponent::UpdateSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, AZ::Aabb& oldBounds)
    {
        AZ_Assert(HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Points)
                ? entry.m_maxPointsCreatedPerInput > 0
                : ((HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Rays) ||
                    HasSurfaceQueryCapability(entry.m_queryCapabilities, SurfaceDataQueryCapabilities::Volumes)) && entry.m_maxPointsCreatedPerInput == 0),
            "Point providers require a positive XY capacity; geometry-only providers require zero XY capacity.");
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        auto entryItr = m_registeredSurfaceDataProviders.find(handle);
        if (entryItr != m_registeredSurfaceDataProviders.end())
        {
            oldBounds = entryItr->second.m_bounds;
            entryItr->second = entry;
            return true;
        }
        return false;
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataModifierInternal(const SurfaceDataRegistryEntry& entry)
    {
        AZ_Assert(entry.m_maxPointsCreatedPerInput == 0, "Surface data modifiers cannot create any points.");
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryHandle handle = ++m_registeredSurfaceDataModifierHandleCounter;
        m_registeredSurfaceDataModifiers[handle] = entry;
        m_registeredModifierTags.insert(entry.m_tags.begin(), entry.m_tags.end());
        return handle;
    }

    SurfaceDataRegistryEntry SurfaceDataSystemComponent::UnregisterSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle)
    {
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryEntry entry;
        auto entryItr = m_registeredSurfaceDataModifiers.find(handle);
        if (entryItr != m_registeredSurfaceDataModifiers.end())
        {
            entry = entryItr->second;
            m_registeredSurfaceDataModifiers.erase(entryItr);
        }
        return entry;
    }

    bool SurfaceDataSystemComponent::UpdateSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, AZ::Aabb& oldBounds)
    {
        AZ_Assert(entry.m_maxPointsCreatedPerInput == 0, "Surface data modifiers cannot create any points.");
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        auto entryItr = m_registeredSurfaceDataModifiers.find(handle);
        if (entryItr != m_registeredSurfaceDataModifiers.end())
        {
            oldBounds = entryItr->second.m_bounds;
            entryItr->second = entry;
            m_registeredModifierTags.insert(entry.m_tags.begin(), entry.m_tags.end());
            return true;
        }
        return false;
    }

    SurfaceTagSet SurfaceDataSystemComponent::GetTagsFromBounds(
        const AZ::Aabb& bounds, const SurfaceDataRegistryMap& registeredEntries) const
    {
        SurfaceTagSet tags;

        const bool inputHasInfiniteBounds = !bounds.IsValid();

        for (const auto& [entryHandle, entry] : registeredEntries)
        {
            const bool entryHasInfiniteBounds = !entry.m_bounds.IsValid();

            if (inputHasInfiniteBounds || entryHasInfiniteBounds || AabbOverlaps2D(entry.m_bounds, bounds))
            {
                for (auto& entryTag : entry.m_tags)
                {
                    tags.emplace(entryTag);
                }
            }
        }

        return tags;
    }

    SurfaceTagSet SurfaceDataSystemComponent::GetProviderTagsFromBounds(const AZ::Aabb& bounds) const
    {
        return GetTagsFromBounds(bounds, m_registeredSurfaceDataProviders);
    }

    SurfaceTagSet SurfaceDataSystemComponent::GetModifierTagsFromBounds(const AZ::Aabb& bounds) const
    {
        return GetTagsFromBounds(bounds, m_registeredSurfaceDataModifiers);
    }

    SurfaceTagSet SurfaceDataSystemComponent::ConvertTagVectorToSet(const SurfaceTagVector& surfaceTags) const
    {
        SurfaceTagSet tags;
        for (auto& tag : surfaceTags)
        {
            tags.emplace(tag);
        }

        return tags;
    }

    SurfaceTagSet SurfaceDataSystemComponent::GetAffectedSurfaceTags(const AZ::Aabb& bounds, const SurfaceTagVector& providerTags) const
    {
        // Get all of the surface tags that can be affected by surface provider changes within the given bounds.
        // Because a change to a surface provider can cause changes to a surface modifier as well, we need to merge all of the
        // surface provider tags with all of the potential surface modifier tags in the given bounds.

        // Get all of the modifier tags that occur in the bounds.
        SurfaceTagSet tagSet = GetModifierTagsFromBounds(bounds);

        // Merge the provider tags with them.
        for (auto& tag : providerTags)
        {
            tagSet.emplace(tag);
        }

        return tagSet;
    }

} // namespace JE::Geometry::SurfaceData
