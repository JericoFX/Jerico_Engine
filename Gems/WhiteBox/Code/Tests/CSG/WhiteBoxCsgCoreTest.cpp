/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "WhiteBoxCsgCore.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>

namespace
{
    using JE::BSP::Csg::BooleanOperation;
    using JE::BSP::Csg::MeshBoolean;
    using JE::BSP::Csg::TriangleMesh;
    using Point = std::array<double, 3>;

    TriangleMesh Cube(const double offsetX = 0.0)
    {
        TriangleMesh mesh{
            {-0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5,
             -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5},
            {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
             3, 7, 6, 3, 6, 2, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5}};
        for (size_t vertex = 0; vertex < mesh.VertexCount(); ++vertex)
        {
            mesh.m_positions[vertex * 3] += offsetX;
        }
        return mesh;
    }

    int failures = 0;

    void Check(const bool condition, const char* name)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        failures += condition ? 0 : 1;
    }

    TriangleMesh Box(const Point& minimum, const Point& maximum)
    {
        TriangleMesh mesh = Cube();
        for (size_t vertex = 0; vertex < mesh.VertexCount(); ++vertex)
        {
            for (size_t axis = 0; axis < 3; ++axis)
            {
                double& coordinate = mesh.m_positions[vertex * 3 + axis];
                coordinate = minimum[axis] + (coordinate + 0.5) * (maximum[axis] - minimum[axis]);
            }
        }
        return mesh;
    }

    Point Vertex(const TriangleMesh& mesh, const uint32_t index)
    {
        const size_t offset = static_cast<size_t>(index) * 3;
        return {mesh.m_positions[offset], mesh.m_positions[offset + 1], mesh.m_positions[offset + 2]};
    }

    double Volume(const TriangleMesh& mesh)
    {
        double volume = 0.0;
        for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
        {
            const Point a = Vertex(mesh, mesh.m_indices[triangle * 3]);
            const Point b = Vertex(mesh, mesh.m_indices[triangle * 3 + 1]);
            const Point c = Vertex(mesh, mesh.m_indices[triangle * 3 + 2]);
            volume += a[0] * (b[1] * c[2] - b[2] * c[1]) +
                a[1] * (b[2] * c[0] - b[0] * c[2]) + a[2] * (b[0] * c[1] - b[1] * c[0]);
        }
        return volume / 6.0;
    }

    bool HasClosedOrientedEdges(const TriangleMesh& mesh)
    {
        std::map<std::pair<uint32_t, uint32_t>, std::pair<int, int>> edges;
        for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
        {
            for (size_t edge = 0; edge < 3; ++edge)
            {
                const uint32_t a = mesh.m_indices[triangle * 3 + edge];
                const uint32_t b = mesh.m_indices[triangle * 3 + (edge + 1) % 3];
                if (a == b)
                {
                    return false;
                }
                auto& incidence = edges[{std::min(a, b), std::max(a, b)}];
                ++incidence.first;
                incidence.second += a < b ? 1 : -1;
            }
        }
        return !edges.empty() && std::all_of(edges.begin(), edges.end(), [](const auto& edge)
            { return edge.second.first == 2 && edge.second.second == 0; });
    }

    // Test-only point classification: cast +X and count distinct intersections.
    // Probes are authored away from the boundaries, so the expected solid/air
    // classification is independent of the CSG implementation.
    bool ContainsPoint(const TriangleMesh& mesh, const Point& point)
    {
        std::vector<double> crossings;
        for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
        {
            const Point a = Vertex(mesh, mesh.m_indices[triangle * 3]);
            const Point b = Vertex(mesh, mesh.m_indices[triangle * 3 + 1]);
            const Point c = Vertex(mesh, mesh.m_indices[triangle * 3 + 2]);
            const double determinant = (b[1] - a[1]) * (c[2] - a[2]) - (c[1] - a[1]) * (b[2] - a[2]);
            if (std::abs(determinant) < 1e-12)
            {
                continue;
            }
            const double u = ((point[1] - a[1]) * (c[2] - a[2]) - (c[1] - a[1]) * (point[2] - a[2])) / determinant;
            const double v = ((b[1] - a[1]) * (point[2] - a[2]) - (point[1] - a[1]) * (b[2] - a[2])) / determinant;
            const double x = a[0] + u * (b[0] - a[0]) + v * (c[0] - a[0]);
            if (u >= -1e-12 && v >= -1e-12 && u + v <= 1.0 + 1e-12 && x > point[0] + 1e-12)
            {
                crossings.push_back(x);
            }
        }
        std::sort(crossings.begin(), crossings.end());
        const auto end = std::unique(crossings.begin(), crossings.end(), [](const double a, const double b)
            { return std::abs(a - b) < 1e-9; });
        return static_cast<size_t>(end - crossings.begin()) % 2 != 0;
    }

    void RoomCorridorFixture()
    {
        TriangleMesh room;
        Check(MeshBoolean(Box({-5.0, -4.0, 0.0}, {5.0, 4.0, 4.0}),
            Box({-4.75, -3.75, 0.25}, {4.75, 3.75, 3.75}), BooleanOperation::Subtraction, room),
            "room shell is constructed");
        Check(std::abs(Volume(room) - 70.625) < 1e-8, "room shell has authored volume");

        TriangleMesh corridor;
        Check(MeshBoolean(Box({-1.5, -10.0, 0.0}, {1.5, -3.75, 4.0}),
            Box({-1.25, -9.75, 0.25}, {1.25, -4.0, 3.75}), BooleanOperation::Subtraction, corridor),
            "corridor shell is constructed");
        Check(std::abs(Volume(corridor) - 24.6875) < 1e-8, "corridor shell has authored volume");

        Check(MeshBoolean(room, corridor, BooleanOperation::Union, room), "room and corridor are united in place");
        Check(std::abs(Volume(room) - 92.3125) < 1e-8, "shared wall is counted once");
        Check(MeshBoolean(room, Box({-0.75, -4.25, 0.25}, {0.75, -3.5, 2.75}),
            BooleanOperation::Subtraction, room), "doorway is cut through the shared wall");
        Check(std::abs(Volume(room) - 91.375) < 1e-8, "doorway removes the authored wall volume");
        Check(MeshBoolean(room, Box({4.5, -0.75, 1.25}, {5.25, 0.75, 2.5}),
            BooleanOperation::Subtraction, room), "window is cut through the room wall");
        Check(std::abs(Volume(room) - 90.90625) < 1e-8, "window removes the authored wall volume");
        Check(HasClosedOrientedEdges(room), "room corridor openings retain closed oriented topology");
        if (room.m_positions.empty())
        {
            return;
        }

        Point minimum = Vertex(room, 0);
        Point maximum = minimum;
        for (size_t vertex = 0; vertex < room.VertexCount(); ++vertex)
        {
            for (size_t axis = 0; axis < 3; ++axis)
            {
                const double coordinate = room.m_positions[vertex * 3 + axis];
                minimum[axis] = std::min(minimum[axis], coordinate);
                maximum[axis] = std::max(maximum[axis], coordinate);
            }
        }
        Check(minimum == Point{-5.0, -10.0, 0.0} && maximum == Point{5.0, 4.0, 4.0}, "layout retains authored bounds");
        Check(!ContainsPoint(room, {0.137, 0.173, 1.53}), "room interior remains clear");
        Check(!ContainsPoint(room, {0.137, -6.213, 1.53}), "corridor interior remains clear");
        Check(!ContainsPoint(room, {0.137, -3.87, 1.53}), "doorway remains clear");
        Check(!ContainsPoint(room, {4.875, 0.173, 1.53}), "window remains clear");
        Check(ContainsPoint(room, {4.875, 1.73, 1.53}), "uncut wall remains solid");
    }
}

int main()
{
    const TriangleMesh cube = Cube();
    TriangleMesh result = Cube(10.0);
    Check(MeshBoolean(cube, cube, BooleanOperation::Subtraction, result), "A-A is successful");
    Check(result.m_positions.empty() && result.m_indices.empty(), "A-A publishes empty geometry");

    result = Cube(10.0);
    Check(MeshBoolean(cube, Cube(3.0), BooleanOperation::Intersection, result), "disjoint intersection is successful");
    Check(result.m_positions.empty() && result.m_indices.empty(), "disjoint intersection publishes empty geometry");

    result = Cube(10.0);
    const TriangleMesh previous = result;
    TriangleMesh openMesh = cube;
    openMesh.m_indices.resize(openMesh.m_indices.size() - 3);
    Check(!MeshBoolean(cube, openMesh, BooleanOperation::Subtraction, result), "open operand fails");
    Check(result.m_positions == previous.m_positions && result.m_indices == previous.m_indices,
        "failed operation preserves previous output");

    TriangleMesh malformed = cube;
    malformed.m_positions.push_back(2.0);
    Check(!MeshBoolean(cube, malformed, BooleanOperation::Union, result), "incomplete vertex is rejected");
    Check(result.m_positions == previous.m_positions && result.m_indices == previous.m_indices,
        "incomplete vertex preserves previous output");

    result = previous;
    malformed = cube;
    malformed.m_indices.push_back(0);
    Check(!MeshBoolean(cube, malformed, BooleanOperation::Union, result), "incomplete triangle is rejected");
    Check(result.m_positions == previous.m_positions && result.m_indices == previous.m_indices,
        "incomplete triangle preserves previous output");

    result = previous;
    Check(!MeshBoolean(cube, Cube(0.5), static_cast<BooleanOperation>(99), result), "unknown operation is rejected");
    Check(result.m_positions == previous.m_positions && result.m_indices == previous.m_indices,
        "unknown operation preserves previous output");

    result = previous;
    TriangleMesh tinyCube = cube;
    for (double& coordinate : tinyCube.m_positions)
    {
        coordinate *= 1e-8;
    }
    const bool tinySucceeded = MeshBoolean(tinyCube, tinyCube, BooleanOperation::Union, result);
    Check(tinySucceeded || (result.m_positions == previous.m_positions && result.m_indices == previous.m_indices),
        "submicrometer cleanup failure preserves previous output");
    Check(!tinySucceeded || (!result.m_positions.empty() && !result.m_indices.empty()),
        "nonempty submicrometer union is never published as empty success");

    Check(MeshBoolean(cube, Cube(0.5), BooleanOperation::Union, result), "overlapping union succeeds");
    Check(!result.m_positions.empty() && !result.m_indices.empty(), "overlapping union publishes geometry");
    RoomCorridorFixture();
    return failures == 0 ? 0 : 1;
}
