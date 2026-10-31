/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Geometry/GeometryExports.h>

#include "WhiteBoxMaterial.h"

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <JE/Geometry/SurfaceData/SurfaceDataTypes.h>

namespace AZ
{
    class ReflectContext;
    class Transform;
}

namespace JE::BSP
{
    //! Full caller-owned surface identity and a local Atom slot. Slot zero is the legacy fallback.
    struct JE_GEOMETRY_API SurfaceMaterialBinding
    {
        AZ_TYPE_INFO(SurfaceMaterialBinding, "{150374DB-4A94-46DB-A11E-C34EFB4DCC71}");
        static void Reflect(AZ::ReflectContext* context);
        AZ::Uuid m_surfaceId;
        uint32_t m_materialSlot = 0;
        bool operator==(const SurfaceMaterialBinding& rhs) const
        {
            return m_surfaceId == rhs.m_surfaceId && m_materialSlot == rhs.m_materialSlot;
        }
    };
    using SurfaceMaterialBindings = AZStd::vector<SurfaceMaterialBinding>;
    JE_GEOMETRY_API uint32_t FindSurfaceMaterialSlot(const SurfaceMaterialBindings& bindings, const AZ::Uuid& surfaceId);

    struct JE_GEOMETRY_API MaterialIndexRange
    {
        uint32_t m_materialSlot = 0;
        uint32_t m_firstIndex = 0;
        uint32_t m_indexCount = 0;
    };
}
namespace JE::Geometry
{
    struct WhiteBoxVertex;
    struct WhiteBoxFace;
    using WhiteBoxFaces = AZStd::vector<WhiteBoxFace>;

    struct JE_GEOMETRY_API WhiteBoxRenderData
    {
        AZ_TYPE_INFO(WhiteBoxRenderData, "{7B46EB9E-0CDF-492C-B015-240D8AB74A37}");
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxRenderData() = default;
        ~WhiteBoxRenderData() = default;

        WhiteBoxFaces m_faces;
        WhiteBoxMaterial m_material;
        SurfaceData::SurfaceSourceRevision m_sourceRevision{}; //!< Missing in v3 and older: remains Unavailable until authored rebuild.
    };

    //! Vertex layout for WhiteBox faces
    struct JE_GEOMETRY_API WhiteBoxVertex
    {
        AZ_TYPE_INFO(WhiteBoxVertex, "{617FFD68-3528-4627-92C6-4CC7ACCBD615}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::Vector3 m_position;
        AZ::Vector2 m_uv;
    };

    //! Triangle primitive with face normals
    struct JE_GEOMETRY_API WhiteBoxFace
    {
        AZ_TYPE_INFO(WhiteBoxFace, "{31293BF0-5789-489B-882A-119AC1797F9E}");
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxVertex m_v1;
        WhiteBoxVertex m_v2;
        WhiteBoxVertex m_v3;
        AZ::Vector3 m_normal;
        AZ::Uuid m_surfaceId; //!< Opaque UUID128, never used as an Atom slot or truncated.
        uint32_t m_materialSlot = 0;
    };

    using WhiteBoxRaySidedness = SurfaceData::SurfaceRaySidedness;
    using WhiteBoxRayQueryStatus = SurfaceData::SurfaceRayQueryStatus;
    using WhiteBoxRayHit = SurfaceData::SurfaceRayHit;
    using WhiteBoxRayQueryResult = SurfaceData::SurfaceRayQueryResult;

    //! Schema 1: domain + LE32 schema + LE64 face count; ordered corners xyz/uv, normal xyz,
    //! UUID bytes and LE32 slot, then tint xyz/useTexture/visible. IEEE float32 LE, signed zero canonicalized.
    //! Nonfinite payloads and encoded lengths exceeding the native Sha1 32-bit bit-count are unavailable.
    //! No pose, cache generation, path/hint, external MaterialAsset data or struct padding participates.
    JE_GEOMETRY_API AZStd::optional<SurfaceData::SurfaceSourceRevision> ComputeWhiteBoxSourceRevision(
        const WhiteBoxRenderData& renderData);
    //! Called by the authoring producer, never to upgrade an unversioned cooked runtime payload.
    JE_GEOMETRY_API bool UpdateWhiteBoxSourceRevision(WhiteBoxRenderData& renderData);

    //! Collect each triangle at most once using the existing segment/triangle intersection.
    //! Keep the nearest output.size() hits, breaking equal-distance ties by input primitive index.
    //! Distinct coincident primitives remain distinct, even when they share a surface UUID.
    //! worldFromLocal supports AZ::Transform's rotation/translation/uniform scale, not nonuniform scale.
    //! Nonfinite transform/endpoints/vertices or unrepresentable triangle arithmetic return InvalidInput
    //! before writing output. Zero transform scale, a zero/nonfinite-length segment or invalid sidedness are invalid.
    //! Finite degenerate triangles use BuildCulledWhiteBoxFaces' LOCAL area threshold; world normals must be finite
    //! and nonzero for retained faces, so supported uniform scaling does not silently cull a locally valid face.
    //! A zero-capacity output is valid: misses are Complete, hits are Truncated with a discarded count.
    JE_GEOMETRY_API WhiteBoxRayQueryResult IntersectWhiteBoxRenderData(
        const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal,
        const AZ::Vector3& startWorld, const AZ::Vector3& endWorld,
        WhiteBoxRaySidedness sidedness, AZStd::span<WhiteBoxRayHit> output);

    //! Closed AABB/triangle intersections, including boundary contacts. Keeps first original primitives, never proximity-dedups.
    //! Shared hit position is an actual triangle/box point; distance is to the box center, NOT penetration/minimum distance.
    //! Double sided, world winding normal; no solid occupancy/clearance claim for an empty box result.
    //! Invalid geometry/request/transform is rejected before output writes; finite degenerates retain LOCAL renderer culling.
    JE_GEOMETRY_API WhiteBoxRayQueryResult IntersectWhiteBoxRenderDataVolume(
        const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal,
        const SurfaceData::SurfaceVolumeRequest& request, AZStd::span<WhiteBoxRayHit> output);

    //! One geometry registration for an actual WhiteBox component; geometry is copied only when published.
    //! Publish/UpdateTransform/destruction run serially on the component owner thread. Queries may run in parallel.
    class JE_GEOMETRY_API WhiteBoxSurfaceDataProvider
    {
    public:
        explicit WhiteBoxSurfaceDataProvider(const AZ::EntityComponentIdPair& owner);
        ~WhiteBoxSurfaceDataProvider();
        WhiteBoxSurfaceDataProvider(const WhiteBoxSurfaceDataProvider&) = delete;
        WhiteBoxSurfaceDataProvider& operator=(const WhiteBoxSurfaceDataProvider&) = delete;
        bool Publish(const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal);
        bool UpdateTransform(const AZ::Transform& worldFromLocal);

    private:
        class State;
        AZStd::unique_ptr<State> m_state;
    };

    //! Builds a vector of visible faces by removing the degenerate faces from the source data
    JE_GEOMETRY_API WhiteBoxFaces BuildCulledWhiteBoxFaces(const WhiteBoxFaces& sourceData);

} // namespace JE::Geometry

namespace JE::BSP
{
    //! Stable partition of whole triangles; shared vertex streams retain their exact corner UVs.
    JE_GEOMETRY_API AZStd::vector<MaterialIndexRange> PartitionFacesByMaterial(JE::Geometry::WhiteBoxFaces& faces);
}
