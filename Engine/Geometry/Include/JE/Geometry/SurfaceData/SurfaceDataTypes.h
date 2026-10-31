/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Geometry/GeometryExports.h>
#include <cmath>

#include <AzCore/Math/Aabb.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/SurfaceData/SurfaceData.h>
#include <JE/Geometry/SurfaceData/SurfaceTag.h>

namespace JE::Geometry::SurfaceData
{
    //map of id or crc to contribution factor
    using SurfaceTagNameSet = AZStd::unordered_set<AZStd::string>;
    using SurfaceTagVector = AZStd::vector<SurfaceTag>;
    using SurfaceTagSet = AZStd::unordered_set<SurfaceTag>;

    //! SurfaceTagWeights stores a collection of surface tags and weights.
    //! A surface tag can only appear once in the collection. Attempting to add it multiple times will always preserve the
    //! highest weight value.
    class JE_GEOMETRY_API SurfaceTagWeights
    {
    public:
        SurfaceTagWeights() = default;

        //! Construct a collection of SurfaceTagWeights from the given SurfaceTagWeightList.
        //! @param weights - The list of weights to assign to the new instance.
        SurfaceTagWeights(const AzFramework::SurfaceData::SurfaceTagWeightList& weights)
        {
            AssignSurfaceTagWeights(weights);
        }

        //! Replace the existing surface tag weights with the given set.
        //! @param weights - The list of weights to assign to this instance.
        void AssignSurfaceTagWeights(const AzFramework::SurfaceData::SurfaceTagWeightList& weights);

        //! Replace the existing surface tag weights with the given set.
        //! @param tags - The list of tags to assign to this instance.
        //! @param weight - The weight to assign to each tag.
        void AssignSurfaceTagWeights(const SurfaceTagVector& tags, float weight);

        //! Add a surface tag weight to this collection. If the tag already exists, the higher weight will be preserved.
        //! (This method is intentionally inlined for its performance impact)
        //! @param tag - The surface tag.
        //! @param weight - The surface tag weight.
        void AddSurfaceTagWeight(const AZ::Crc32 tag, const float weight)
        {
            for (auto weightItr = m_weights.begin(); weightItr != m_weights.end(); ++weightItr)
            {
                // Since we need to scan for duplicate surface types, store the entries sorted by surface type so that we can
                // early-out once we pass the location for the entry instead of always searching every entry.
                if (weightItr->m_surfaceType > tag)
                {
                    if (m_weights.size() != AzFramework::SurfaceData::Constants::MaxSurfaceWeights)
                    {
                        // We didn't find the surface type, so add the new entry in sorted order.
                        m_weights.insert(weightItr, { tag, weight });
                    }
                    else
                    {
                        AZ_Assert(false, "SurfaceTagWeights has reached max capacity, it cannot add a new tag / weight.");
                    }
                    return;
                }
                else if (weightItr->m_surfaceType == tag)
                {
                    // We found the surface type, so just keep the higher of the two weights.
                    weightItr->m_weight = AZ::GetMax(weight, weightItr->m_weight);
                    return;
                }
            }

            // We didn't find the surface weight, and the sort order for it is at the end, so add it to the back of the list.
            if (m_weights.size() != AzFramework::SurfaceData::Constants::MaxSurfaceWeights)
            {
                m_weights.emplace_back(tag, weight);
            }
            else
            {
                AZ_Assert(false, "SurfaceTagWeights has reached max capacity, it cannot add a new tag / weight.");
            }
        }

        //! Add surface tags and weights to this collection. If a tag already exists, the higher weight will be preserved.
        //! (This method is intentionally inlined for its performance impact)
        //! @param tags - The surface tags to replace/add.
        //! @param weight - The surface tag weight to use for each tag.
        void AddSurfaceTagWeights(const SurfaceTagVector& tags, const float weight)
        {
            for (const auto& tag : tags)
            {
                AddSurfaceTagWeight(tag, weight);
            }
        }

        //! Add surface tags and weights to this collection. If a tag already exists, the higher weight will be preserved.
        //! (This method is intentionally inlined for its performance impact)
        //! @param weights - The surface tags and weights to replace/add.
        void AddSurfaceTagWeights(const SurfaceTagWeights& weights)
        {
            for (const auto& [tag, weight] : weights.m_weights)
            {
                AddSurfaceTagWeight(tag, weight);
            }
        }

        //! Equality comparison operator for SurfaceTagWeights.
        bool operator==(const SurfaceTagWeights& rhs) const;

        //! Inequality comparison operator for SurfaceTagWeights.
        bool operator!=(const SurfaceTagWeights& rhs) const
        {
            return !(*this == rhs);
        }

        //! Compares a SurfaceTagWeightList with a SurfaceTagWeights instance to look for equality.
        //! They will be equal if they have the exact same set of tags and weights.
        //! @param compareWeights - the set of weights to compare against.
        bool SurfaceWeightsAreEqual(const AzFramework::SurfaceData::SurfaceTagWeightList& compareWeights) const;

        //! Clear the surface tag weight collection.
        void Clear();

        //! Get the size of the surface tag weight collection.
        //! @return The size of the collection.
        size_t GetSize() const;

        //! Get the collection of surface tag weights as a SurfaceTagWeightList.
        //! @return SurfaceTagWeightList containing the same tags and weights as this collection.
        AzFramework::SurfaceData::SurfaceTagWeightList GetSurfaceTagWeightList() const;

        //! Enumerate every tag and weight and call a callback for each one found.
        //! Callback params:
        //!     AZ::Crc32 - The surface tag.
        //!     float - The surface tag weight.
        //!     return - true to keep enumerating, false to stop.
        //! @param weightCallback - the callback to use for each surface tag / weight found.
        void EnumerateWeights(AZStd::function<bool(AZ::Crc32 tag, float weight)> weightCallback) const;

        //! Check to see if the collection has any valid tags stored within it.
        //! A tag of "Unassigned" is considered an invalid tag.
        //! @return True if there is at least one valid tag, false if there isn't.
        bool HasValidTags() const;

        //! Check to see if the collection contains the given tag.
        //! @param sampleTag - The tag to look for.
        //! @return True if the tag is found, false if it isn't.
        bool HasMatchingTag(AZ::Crc32 sampleTag) const;

        //! Check to see if the collection contains the given tag with the given weight range.
        //! The range check is inclusive on both sides of the range: [weightMin, weightMax]
        //! @param sampleTag - The tag to look for.
        //! @param weightMin - The minimum weight for this tag.
        //! @param weightMax - The maximum weight for this tag.
        //! @return True if the tag is found, false if it isn't.
        bool HasMatchingTag(AZ::Crc32 sampleTag, float weightMin, float weightMax) const;

        //! Check to see if the collection contains any of the given tags.
        //! @param sampleTags - The tags to look for.
        //! @return True if any of the tags is found, false if none are found.
        bool HasAnyMatchingTags(AZStd::span<const SurfaceTag> sampleTags) const;

        //! Check to see if the collection contains the given tag with the given weight range.
        //! The range check is inclusive on both sides of the range: [weightMin, weightMax]
        //! @param sampleTags - The tags to look for.
        //! @param weightMin - The minimum weight for this tag.
        //! @param weightMax - The maximum weight for this tag.
        //! @return True if any of the tags is found, false if none are found.
        bool HasAnyMatchingTags(AZStd::span<const SurfaceTag> sampleTags, float weightMin, float weightMax) const;

    private:
        //! Search for the given tag entry.
        //! @param tag - The tag to search for.
        //! @return The pointer to the tag that's found, or end() if it wasn't found.
        const AzFramework::SurfaceData::SurfaceTagWeight* FindTag(AZ::Crc32 tag) const;

        AZStd::fixed_vector<AzFramework::SurfaceData::SurfaceTagWeight, AzFramework::SurfaceData::Constants::MaxSurfaceWeights> m_weights;
    };


    enum class SurfaceRaySidedness { FrontFace, DoubleSided };
    enum class SurfaceRayQueryFilter { AllRegisteredProviders, ExactOwner };
    enum class SurfaceRayQueryStatus { Complete, Truncated, InvalidInput, Unsupported, Partial };
    enum class SurfaceSourceRevisionState { Unavailable, Content };

    //! Opaque, schema-qualified local published-content fingerprint, not a monotonic counter or authentication token.
    //! A zero/unknown schema is unavailable. It excludes owner pose and external MaterialAsset dependencies.
    struct SurfaceSourceRevision
    {
        AZ_TYPE_INFO(SurfaceSourceRevision, "{CA21A79C-7D6A-476E-A0DE-39C2DDBF7018}");
        AZ::u32 m_schema = 0;
        AZStd::array<AZ::u32, 5> m_digest{};
        bool operator==(const SurfaceSourceRevision& rhs) const
        {
            return m_schema == rhs.m_schema && m_digest == rhs.m_digest;
        }
        bool operator!=(const SurfaceSourceRevision& rhs) const { return !(*this == rhs); }
    };

    struct SurfaceRayRequest
    {
        AZ::Vector3 m_startWorldPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 m_endWorldPosition = AZ::Vector3::CreateZero();
        SurfaceRaySidedness m_sidedness = SurfaceRaySidedness::DoubleSided;
        SurfaceRayQueryFilter m_filter = SurfaceRayQueryFilter::AllRegisteredProviders;
        AZ::EntityComponentIdPair m_owner{AZ::EntityId(), AZ::InvalidComponentId}; //!< Required only for ExactOwner.
    };

    //! Optional address of a primitive in the currently loaded model product; not a persistent Surface/Face/Region ID.
    //! The producer supplies the real product AssetId and selected LOD/submesh, without an RPI dependency here.
    struct SurfaceModelProvenance
    {
        AZ::Data::AssetId m_modelAssetId{};
        uint32_t m_lodIndex = 0;
        uint32_t m_submeshIndex = 0;
    };

    struct SurfaceRayHit
    {
        AZ::Vector3 m_worldPosition = AZ::Vector3::CreateZero();
        AZ::Vector3 m_worldNormal = AZ::Vector3::CreateZero(); //!< World triangle winding, including back-side hits.
        float m_distance = 0.0f;
        AZ::Uuid m_surfaceId = AZ::Uuid::CreateNull(); //!< Actual authored SurfaceId, or null when unavailable; never an ordinal/slot.
        AZStd::optional<uint32_t> m_materialSlot = AZStd::nullopt; //!< Missing differs from a real slot zero.
        size_t m_primitiveIndex = 0; //!< Local to this input's triangle array, not stable across topology changes.
        AZ::EntityComponentIdPair m_entityAndComponent{AZ::EntityId(), AZ::InvalidComponentId};
        SurfaceSourceRevisionState m_revisionState = SurfaceSourceRevisionState::Unavailable;
        //! Appended to preserve WhiteBox aggregate construction; absent for its directly authored geometry.
        AZStd::optional<SurfaceModelProvenance> m_modelProvenance = AZStd::nullopt;
        SurfaceSourceRevision m_sourceRevision{}; //!< Meaningful only when revisionState is Content; from the same snapshot as hits.
        //! Explicit authored Face identity, or null when unavailable; never derived from SurfaceId or a triangle ordinal.
        AZ::Uuid m_faceId = AZ::Uuid::CreateNull();
        AZStd::optional<AZ::Uuid> m_regionId = AZStd::nullopt; //!< Authored membership from that same face reference, when present.
    };

    //! Providers truncate by the same distance/owner/model-address/primitive key as the system merge.
    struct SurfaceRayQueryResult
    {
        size_t m_hitCount = 0;
        size_t m_discardedHitCount = 0; //!< Known hits discarded by capacity, even when status is Partial.
        size_t m_supportedProviderCount = 0; //!< Providers that answered a complete/truncated query, including misses.
        size_t m_unsupportedProviderCount = 0;
        SurfaceRayQueryStatus m_status = SurfaceRayQueryStatus::Complete;
    };

    //! Work completion is separate from query completeness. Aborted batches never commit hits or per-ray results.
    enum class SurfaceRayBatchCompletion { Finished, InvalidInput, Cancelled, BudgetExhausted, Unsupported };

    struct SurfaceRayBatchProbe
    {
        SurfaceRayRequest m_ray;
        size_t m_hitOffset = 0;
        size_t m_hitCapacity = 0; //!< Output slice; nonempty slices must be disjoint, but need not be ordered.
    };

    struct SurfaceRayBatchLimits
    {
        size_t m_maxRayCount = 0;
        //! Counts registry entries examined during the shared candidate scan and applicable candidate visits per ray.
        //! A visit can reject by bounds/capability without invoking geometry. It does not bound a single geometry call.
        size_t m_maxProviderVisits = 0;
        size_t m_maxTotalHits = 0; //!< Earlier probes consume this retained-hit capacity first; zero still counts discarded hits.
        const AZStd::atomic_bool* m_cancellationFlag = nullptr; //!< Borrowed only for this synchronous call; null disables cancellation.
    };

    struct SurfaceRayBatchResult
    {
        SurfaceRayBatchCompletion m_completion = SurfaceRayBatchCompletion::InvalidInput;
        SurfaceRayQueryStatus m_queryStatus = SurfaceRayQueryStatus::Unsupported; //!< Meaningful only when Finished.
        size_t m_committedRayCount = 0; //!< Zero on every abort, otherwise the complete input count.
        size_t m_hitCount = 0; //!< Retained and committed hits only; zero on abort.
        size_t m_providerVisitCount = 0; //!< Work actually visited, including on abort; never exceeds the caller's limit.
    };

    enum class SurfaceSupportStatus { Found, NoSupport, Indeterminate, InvalidInput };

    //! Point support along one explicit world segment, not a footprint, volume, clearance or placement operation.
    struct SurfaceSupportRequest
    {
        SurfaceRayRequest m_ray;
        AZ::Vector3 m_requiredWorldNormal = AZ::Vector3::CreateAxisZ(); //!< Finite nonzero direction; normalized for comparison only.
        float m_minNormalDot = 1.0f; //!< Inclusive cosine threshold in [0,1], against the original winding normal.
    };

    struct SurfaceSupportResult
    {
        SurfaceSupportStatus m_status = SurfaceSupportStatus::InvalidInput;
        SurfaceRayBatchCompletion m_completion = SurfaceRayBatchCompletion::InvalidInput;
        SurfaceRayQueryResult m_query{0, 0, 0, 0, SurfaceRayQueryStatus::Unsupported}; //!< Meaningful when completion is Finished.
        size_t m_providerVisitCount = 0;
        AZStd::optional<SurfaceRayHit> m_hit = AZStd::nullopt; //!< Confirmed only for Found; full original hit/revision, never reconstructed IDs.
    };

    //! Closed world-space AABB surface intersection, not solid occupancy or prefab clearance.
    struct SurfaceVolumeRequest
    {
        AZ::Aabb m_worldBounds = AZ::Aabb::CreateNull();
        SurfaceRayQueryFilter m_filter = SurfaceRayQueryFilter::AllRegisteredProviders;
        AZ::EntityComponentIdPair m_owner{AZ::EntityId(), AZ::InvalidComponentId};

        bool IsValid() const
        {
            if (!m_worldBounds.IsValid() || !m_worldBounds.GetMin().IsFinite() || !m_worldBounds.GetMax().IsFinite()) { return false; }
            const auto extent = m_worldBounds.GetExtents();
            return extent.IsFinite() && std::isfinite(extent.GetLengthSq()) && extent.GetLengthSq() > 0.0f && m_worldBounds.GetCenter().IsFinite() &&
                extent.GetX() > 0.0f && extent.GetY() > 0.0f && extent.GetZ() > 0.0f &&
                (m_filter == SurfaceRayQueryFilter::AllRegisteredProviders || m_filter == SurfaceRayQueryFilter::ExactOwner) &&
                (m_filter != SurfaceRayQueryFilter::ExactOwner ||
                    (m_owner.GetEntityId().IsValid() && m_owner.GetComponentId() != AZ::InvalidComponentId));
        }
    };

    struct SurfaceVolumeLimits
    {
        size_t m_maxProviderVisits = 0; //!< Registry entries examined plus candidates dispatched, as in QueryRays.
        size_t m_maxTotalHits = 0;
        const AZStd::atomic_bool* m_cancellationFlag = nullptr; //!< Checked between provider calls; one geometry call is not preempted.
    };

    struct SurfaceVolumeQueryResult
    {
        SurfaceRayQueryResult m_query{0, 0, 0, 0, SurfaceRayQueryStatus::Unsupported};
        SurfaceRayBatchCompletion m_completion = SurfaceRayBatchCompletion::InvalidInput;
        size_t m_providerVisitCount = 0;
    };

    enum class SurfaceDataQueryCapabilities : AZ::u8 { Points = 1, Rays = 2, PointsAndRays = 3, Volumes = 4, RaysAndVolumes = 6 };

    inline bool HasSurfaceQueryCapability(SurfaceDataQueryCapabilities capabilities, SurfaceDataQueryCapabilities query)
    {
        return (static_cast<AZ::u8>(capabilities) & static_cast<AZ::u8>(query)) == static_cast<AZ::u8>(query);
    }

    struct SurfaceDataRegistryEntry
    {
        //! The entity ID of the surface provider / modifier
        AZ::EntityId m_entityId;

        //! The AABB bounds that this surface provider / modifier can affect, or null if it has infinite bounds.
        AZ::Aabb m_bounds = AZ::Aabb::CreateNull();

        //! The set of surface tags that this surface provider / modifier can create or add to a point.
        SurfaceTagVector m_tags;

        //! The maximum number of surface points that this will create per input position.
        //! Modifiers and Rays-only providers use 0; providers with Points capability must use > 0.
        size_t m_maxPointsCreatedPerInput = 0;

        //! Default preserves legacy XY providers. Rays-only providers create no incidental XY points.
        SurfaceDataQueryCapabilities m_queryCapabilities = SurfaceDataQueryCapabilities::Points;
        AZ::ComponentId m_componentId = AZ::InvalidComponentId; //!< Legacy providers can leave component identity unavailable.
    };

    using SurfaceDataRegistryHandle = AZ::u32;
    const SurfaceDataRegistryHandle InvalidSurfaceDataRegistryHandle = 0;
}
