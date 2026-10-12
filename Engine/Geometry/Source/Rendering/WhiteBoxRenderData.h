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

namespace AZ
{
    class ReflectContext;
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

    //! Builds a vector of visible faces by removing the degenerate faces from the source data
    JE_GEOMETRY_API WhiteBoxFaces BuildCulledWhiteBoxFaces(const WhiteBoxFaces& sourceData);

} // namespace JE::Geometry

namespace JE::BSP
{
    //! Stable partition of whole triangles; shared vertex streams retain their exact corner UVs.
    JE_GEOMETRY_API AZStd::vector<MaterialIndexRange> PartitionFacesByMaterial(JE::Geometry::WhiteBoxFaces& faces);
}
