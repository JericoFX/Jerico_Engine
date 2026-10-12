/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxRenderData.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/limits.h>

namespace JE::Geometry
{
    void WhiteBoxRenderData::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxFace::Reflect(context);
        WhiteBoxMaterial::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxRenderData>()
                ->Version(3)
                ->Field("Faces", &WhiteBoxRenderData::m_faces)
                ->Field("Material", &WhiteBoxRenderData::m_material);
        }
    }

    void WhiteBoxVertex::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxVertex>()
                ->Version(1)
                ->Field("Position", &WhiteBoxVertex::m_position)
                ->Field("UV", &WhiteBoxVertex::m_uv);
        }
    }

    void WhiteBoxFace::Reflect(AZ::ReflectContext* context)
    {
        WhiteBoxVertex::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxFace>()
                ->Version(2)
                ->Field("Vertex1", &WhiteBoxFace::m_v1)
                ->Field("Vertex2", &WhiteBoxFace::m_v2)
                ->Field("Vertex3", &WhiteBoxFace::m_v3)
                ->Field("Normal", &WhiteBoxFace::m_normal)
                ->Field("SurfaceId", &WhiteBoxFace::m_surfaceId)
                ->Field("MaterialSlot", &WhiteBoxFace::m_materialSlot);
        }
    }

    WhiteBoxFaces BuildCulledWhiteBoxFaces(const WhiteBoxFaces& inFaces)
    {
        // actual face count can be less than the original face count if any degenerate
        // faces are detected and removed
        const size_t inFaceCount = inFaces.size();
        size_t outFaceCount = 0;

        WhiteBoxFaces outFaces;

        // resize to the worst case number of faces
        outFaces.resize(inFaceCount);

        for (size_t inFaceIndex = 0; inFaceIndex < inFaceCount; ++inFaceIndex)
        {
            const WhiteBoxFace face = inFaces[inFaceIndex];
            const AZ::Vector3& vertex1 = face.m_v1.m_position;
            const AZ::Vector3& vertex2 = face.m_v2.m_position;
            const AZ::Vector3& vertex3 = face.m_v3.m_position;

            const auto areaSquared = 0.5f * ((vertex2 - vertex1).Cross(vertex3 - vertex1)).GetLengthSq();

            // only copy non-degenerate triangles (area is less than zero)
            if (areaSquared > DegenerateTriangleAreaSquareEpsilon)
            {
                outFaces[outFaceCount] = face;
                outFaceCount++;
            }
        }

        // now that we know the number of faces we need we can safely resize the vector without any
        // additional allocations as the number of faces will be equal to or less than the original size
        outFaces.resize(outFaceCount);

        return outFaces;
    }
} // namespace JE::Geometry

namespace JE::BSP
{
    void SurfaceMaterialBinding::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SurfaceMaterialBinding>()->Version(1)
                ->Field("SurfaceId", &SurfaceMaterialBinding::m_surfaceId)
                ->Field("MaterialSlot", &SurfaceMaterialBinding::m_materialSlot);
        }
    }

    uint32_t FindSurfaceMaterialSlot(const SurfaceMaterialBindings& bindings, const AZ::Uuid& surfaceId)
    {
        for (const auto& binding : bindings)
        {
            if (binding.m_surfaceId == surfaceId) { return binding.m_materialSlot; }
        }
        return 0;
    }

    AZStd::vector<MaterialIndexRange> PartitionFacesByMaterial(JE::Geometry::WhiteBoxFaces& faces)
    {
        if (faces.size() > AZStd::numeric_limits<uint32_t>::max() / 3 ||
            AZStd::any_of(faces.begin(), faces.end(), [](const auto& face)
                { return face.m_materialSlot == AZStd::numeric_limits<uint32_t>::max(); })) { return {}; }
        AZStd::stable_sort(faces.begin(), faces.end(), [](const auto& a, const auto& b)
        {
            return a.m_materialSlot < b.m_materialSlot;
        });
        AZStd::vector<MaterialIndexRange> ranges;
        for (size_t face = 0; face < faces.size(); ++face)
        {
            if (ranges.empty() || ranges.back().m_materialSlot != faces[face].m_materialSlot)
            {
                ranges.push_back({faces[face].m_materialSlot, static_cast<uint32_t>(face * 3), 0});
            }
            ranges.back().m_indexCount += 3;
        }
        return ranges;
    }
}