/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Native adaptation of Tetrahedron.py, Icosahedron.py, Cylinder.py, Sphere.py
 * and WhiteBoxMath.py. Preserve their original angles, radii and subdivision.
 */

#include "JEWhiteBoxDefaultShapes.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_map.h>
#include <cmath>

namespace JE::BSP::Editor
{
    namespace Api = JE::Geometry::Api;

    namespace
    {
        AZ::Vector3 Spherical(float phi, float theta, float radius)
        {
            phi = AZ::DegToRad(phi);
            theta = AZ::DegToRad(theta);
            return {radius * std::sin(phi) * std::cos(theta), radius * std::sin(phi) * std::sin(theta), radius * std::cos(phi)};
        }

        Api::FaceVertHandlesList IcosahedronFaces(JE::Geometry::WhiteBoxMesh& mesh, float radius)
        {
            const float angles[12][2] = {{0, 0}, {63.43f, 18}, {63.43f, 90}, {63.43f, 162}, {63.43f, 234}, {63.43f, 306},
                {180, 0}, {116.57f, 52}, {116.57f, 126}, {116.57f, 198}, {116.57f, 270}, {116.57f, 342}};
            Api::VertexHandles vertices;
            for (const auto& angle : angles)
            {
                vertices.push_back(Api::AddVertex(mesh, Spherical(angle[0], angle[1], radius)));
            }
            const int indices[20][3] = {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5}, {0, 5, 1},
                {6, 11, 10}, {6, 10, 9}, {6, 9, 8}, {6, 8, 7}, {6, 7, 11},
                {11, 7, 1}, {7, 8, 2}, {8, 9, 3}, {9, 10, 4}, {10, 11, 5},
                {1, 7, 2}, {2, 8, 3}, {3, 9, 4}, {4, 10, 5}, {5, 11, 1}};
            Api::FaceVertHandlesList faces;
            for (const auto& triangle : indices)
            {
                faces.push_back({{vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]]}});
            }
            return faces;
        }

        void Tetrahedron(JE::Geometry::WhiteBoxMesh& mesh)
        {
            constexpr float radius = 0.75f;
            const float offset = -(Spherical(0, 0, radius).GetZ() + Spherical(109.5f, 90, radius).GetZ()) * 0.5f;
            const auto translate = Spherical(0, 0, offset);
            const auto a = Api::AddVertex(mesh, Spherical(0, 0, radius) + translate);
            const auto b = Api::AddVertex(mesh, Spherical(109.5f, 90, radius) + translate);
            const auto c = Api::AddVertex(mesh, Spherical(109.5f, 210, radius) + translate);
            const auto d = Api::AddVertex(mesh, Spherical(109.5f, 330, radius) + translate);
            Api::AddTriPolygon(mesh, a, b, c);
            Api::AddTriPolygon(mesh, a, c, d);
            Api::AddTriPolygon(mesh, a, d, b);
            Api::AddTriPolygon(mesh, b, d, c);
        }

        void Cylinder(JE::Geometry::WhiteBoxMesh& mesh)
        {
            constexpr int sides = 16;
            constexpr float size = 0.5f;
            const auto top = Api::AddVertex(mesh, Spherical(0, 0, size));
            const auto bottom = Api::AddVertex(mesh, Spherical(180, 0, size));
            Api::VertexHandles topRing, bottomRing;
            for (int side = 0; side < sides; ++side)
            {
                const float angle = side * (360.0f / sides);
                topRing.push_back(Api::AddVertex(mesh, Spherical(45, angle, size * 1.414f)));
                bottomRing.push_back(Api::AddVertex(mesh, Spherical(135, angle, size * 1.414f)));
            }
            Api::FaceVertHandlesList topFaces, bottomFaces;
            for (int side = 0; side < sides; ++side)
            {
                const int next = (side + 1) % sides;
                topFaces.push_back({{top, topRing[side], topRing[next]}});
                bottomFaces.push_back({{bottom, bottomRing[next], bottomRing[side]}});
                Api::AddQuadPolygon(mesh, topRing[side], bottomRing[side], bottomRing[next], topRing[next]);
            }
            Api::AddPolygon(mesh, topFaces);
            Api::AddPolygon(mesh, bottomFaces);
        }

        Api::FaceVertHandlesList Subdivide(JE::Geometry::WhiteBoxMesh& mesh, const Api::FaceVertHandlesList& faces, float radius)
        {
            AZStd::unordered_map<AZ::u64, Api::VertexHandle> midpoints;
            const auto midpoint = [&](Api::VertexHandle a, Api::VertexHandle b)
            {
                const auto lo = static_cast<AZ::u32>(AZStd::min(a.Index(), b.Index()));
                const auto hi = static_cast<AZ::u32>(AZStd::max(a.Index(), b.Index()));
                const AZ::u64 key = (static_cast<AZ::u64>(lo) << 32) | hi;
                if (const auto found = midpoints.find(key); found != midpoints.end())
                {
                    return found->second;
                }
                const auto position = (Api::VertexPosition(mesh, a) + Api::VertexPosition(mesh, b)) * 0.5f;
                const auto vertex = Api::AddVertex(mesh, position.GetNormalized() * radius);
                midpoints.emplace(key, vertex);
                return vertex;
            };
            Api::FaceVertHandlesList divided;
            for (const auto& face : faces)
            {
                const auto& v = face.m_vertexHandles;
                const auto a = midpoint(v[0], v[1]);
                const auto b = midpoint(v[1], v[2]);
                const auto c = midpoint(v[0], v[2]);
                divided.push_back({{v[0], a, c}});
                divided.push_back({{a, v[1], b}});
                divided.push_back({{b, v[2], c}});
                divided.push_back({{a, b, c}});
            }
            return divided;
        }
    }

    bool InitializeDefaultShape(JE::Geometry::WhiteBoxMesh& mesh, JE::Geometry::DefaultShapeType shape)
    {
        if (shape < JE::Geometry::DefaultShapeType::Cube || shape >= JE::Geometry::DefaultShapeType::Asset)
        {
            return false;
        }
        auto candidate = Api::CreateWhiteBoxMesh();
        switch (shape)
        {
        case JE::Geometry::DefaultShapeType::Cube:
            Api::InitializeAsUnitCube(*candidate);
            break;
        case JE::Geometry::DefaultShapeType::Tetrahedron:
            Tetrahedron(*candidate);
            break;
        case JE::Geometry::DefaultShapeType::Cylinder:
            Cylinder(*candidate);
            break;
        case JE::Geometry::DefaultShapeType::Icosahedron:
        case JE::Geometry::DefaultShapeType::Sphere:
        {
            const bool sphere = shape == JE::Geometry::DefaultShapeType::Sphere;
            auto faces = IcosahedronFaces(*candidate, sphere ? 0.55f : 0.6f);
            if (sphere)
            {
                faces = Subdivide(*candidate, faces, 0.55f);
                faces = Subdivide(*candidate, faces, 0.55f);
            }
            for (const auto& face : faces)
            {
                Api::AddPolygon(*candidate, {face});
            }
            break;
        }
        default:
            return false;
        }
        Api::CalculateNormals(*candidate);
        Api::CalculatePlanarUVs(*candidate);
        Api::WhiteBoxMeshStream stream;
        return Api::WriteMesh(*candidate, stream) && Api::ReadMesh(mesh, stream) == Api::ReadResult::Full;
    }
}
