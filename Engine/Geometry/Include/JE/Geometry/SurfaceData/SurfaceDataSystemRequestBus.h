/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/span.h>
#include <JE/Geometry/SurfaceData/SurfaceDataTypes.h>
#include <JE/Geometry/SurfaceData/SurfacePointList.h>

namespace JE::Geometry::SurfaceData
{
    class SurfaceDataSystem
    {
    public:
        AZ_RTTI(SurfaceDataSystem, "{381E1C98-F942-434D-B0C7-22F1AFB679A9}");

        // Get all surface points located at the inPosition that matches one or more of the desiredTags.  Only the XY components of
        // inPosition are used.
        virtual void GetSurfacePoints(
            const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointList) const = 0;

        // Get all surface points for every input position within an AABB region.  Only the XY dimensions of the AABB region are used.
        // The input positions are chosen by starting at the min sides of inRegion and incrementing by stepSize.  This method is inclusive
        // on the min sides of the AABB, and exclusive on the max sides (i.e. for a box of (0,0) - (4,4), the point (0,0) is included but
        // (4,4) isn't).
        virtual void GetSurfacePointsFromRegion(
            const AZ::Aabb& inRegion,
            const AZ::Vector2 stepSize,
            const SurfaceTagVector& desiredTags,
            SurfacePointList& surfacePointLists) const = 0;

        // Get all surface points for every passed-in input position.  Only the XY dimensions of each position are used.
        virtual void GetSurfacePointsFromList(
            AZStd::span<const AZ::Vector3> inPositions, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointLists) const = 0;

        //! Queries the existing registry, globally or for a declared EntityComponentIdPair.
        //! InvalidInput leaves output intact and all counts zero. Unsupported/Partial take priority over Truncated;
        //! discardedHitCount still reports known capacity loss in Partial. No source revision is implied.
        //! Query actual ray-capable geometry. Legacy providers answer Unsupported without fabricated hits.
        //! Provider results are Complete/Truncated/InvalidInput/Unsupported; the system aggregates Partial.
        virtual SurfaceRayQueryResult QueryRay(const SurfaceRayRequest&, AZStd::span<SurfaceRayHit>) const
        {
            SurfaceRayQueryResult result;
            result.m_status = SurfaceRayQueryStatus::Unsupported;
            return result;
        }

        virtual SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry) = 0;
        virtual void UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle) = 0;
        virtual void UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry) = 0;

        virtual SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry) = 0;
        virtual void UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle) = 0;
        virtual void UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry) = 0;

        // Notify any dependent systems that they need to refresh their surface data for the provided area.
        virtual void RefreshSurfaceData(const SurfaceDataRegistryHandle& providerHandle, const AZ::Aabb& dirtyArea) = 0;

        // Get the SurfaceDataRegistryHandle for a given entityId.
        virtual SurfaceDataRegistryHandle GetSurfaceDataProviderHandle(const AZ::EntityId& providerEntityId) = 0;
        virtual SurfaceDataRegistryHandle GetSurfaceDataModifierHandle(const AZ::EntityId& modifierEntityId) = 0;
        //! Batch oriented rays over the same registry. This is neither a volume/clearance query nor an atomic world snapshot.
        //! Each ray uses current provider snapshots as QueryRay does; no results/revisions are cached between calls.
        //! All syntax/ranges are validated before provider visits; non-Finished calls leave both output spans intact.
        //! Cancellation is checked between visits and immediately before commit, never by reentering a caller callback.
        //! Partial/Unsupported outrank known Truncated hit coverage; capacity loss remains in each ray's discard count.
        //! Appended virtual with a safe default for existing system implementations; no engine/caller ABI compatibility is implied.
        virtual SurfaceRayBatchResult QueryRays(AZStd::span<const SurfaceRayBatchProbe>, AZStd::span<SurfaceRayHit>,
            AZStd::span<SurfaceRayQueryResult>, const SurfaceRayBatchLimits&) const
        {
            SurfaceRayBatchResult result;
            result.m_completion = SurfaceRayBatchCompletion::Unsupported;
            return result;
        }

        //! Select the nearest winding normal inside the requested cone using one QueryRays probe.
        //! candidates is caller-owned scratch with the existing QueryRays commit rules. InvalidInput/abort leaves it intact;
        //! Finished Partial/Truncated may commit known candidates, but never a confirmed support or a NoSupport decision.
        //! Found/NoSupport require Finished+Complete. Tie order, identities, source revision and explicit owner scope are inherited.
        //! No retained hit cache, world transaction, volume, whole-prefab clearance or atomic-world-snapshot guarantee.
        //! Appended virtual requires coherent consumer rebuild; default safely declines unsupported implementations.
        virtual SurfaceSupportResult QuerySupport(const SurfaceSupportRequest&, AZStd::span<SurfaceRayHit>,
            const SurfaceRayBatchLimits&) const
        {
            SurfaceSupportResult result;
            result.m_status = SurfaceSupportStatus::Indeterminate;
            result.m_completion = SurfaceRayBatchCompletion::Unsupported;
            return result;
        }

        //! Real triangle/box intersections, double sided. Output order: owner/component/model address/primitive, not distance.
        //! Shared hit position is a confirmed point in triangle AND box; normal retains source winding.
        //! distance is that representative point's distance to the box center, NOT penetration or minimum surface distance.
        //! Complete with no hits means no selected surface crosses this box; it never certifies an unoccupied solid volume.
        //! Limits/cancellation/invalid provider data abort without output writes. Partial takes priority over Truncated.
        //! Appended virtual: rebuild affected images coherently; unsupported legacy implementations remain explicit.
        virtual SurfaceVolumeQueryResult QueryVolume(const SurfaceVolumeRequest&, AZStd::span<SurfaceRayHit>,
            const SurfaceVolumeLimits&) const
        {
            SurfaceVolumeQueryResult result;
            result.m_completion = SurfaceRayBatchCompletion::Unsupported;
            return result;
        }

    };

    /**
    * the EBus is used to request information about a surface
    */
    class SurfaceDataSystemRequestTraits
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////
    };

    using SurfaceDataSystemRequestBus = AZ::EBus<SurfaceDataSystem, SurfaceDataSystemRequestTraits>;
}
