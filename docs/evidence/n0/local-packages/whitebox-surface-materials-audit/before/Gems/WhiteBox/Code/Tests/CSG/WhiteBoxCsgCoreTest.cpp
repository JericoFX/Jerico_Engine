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
#include <limits>
#include <map>
#include <string>

namespace
{
    using JE::BSP::Csg::BooleanOperation;
    using JE::BSP::Csg::MeshBoolean;
    using JE::BSP::Csg::SurfaceKey;
    using JE::BSP::Csg::TriangleMesh;
    using JE::BSP::Csg::WeldVertices;
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

    struct Source
    {
        uint64_t m_namespace;
        Point m_minimum;
        Point m_maximum;
        double m_uvOffset;
    };

    // Both namespaces and surface IDs exceed 32 bits; the namespaces have the
    // same low bits to detect lossy remapping through Manifold's OriginalIDs.
    constexpr uint64_t SurfaceBase = 0x100000000ULL;
    const Source SourceA{0x100000001ULL, {-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}, 10.0};
    const Source SourceB{0x200000001ULL, {0.0, -0.5, -0.25}, {2.0, 1.5, 1.75}, 100.0};
    const Source SourceC{0x300000001ULL, {-0.25, 0.25, 0.25}, {0.75, 0.75, 0.75}, 1000.0};

    std::array<double, 2> AuthoredUV(
        const Source& source, const Point& point, const uint64_t face, const bool upperChart = false)
    {
        const double chart = static_cast<double>(face);
        return {2.0 * point[0] + 3.0 * point[1] + source.m_uvOffset + 10.0 * chart + (upperChart ? 500.0 : 0.0),
            -point[0] + point[2] + source.m_uvOffset + 20.0 * chart + (upperChart ? 700.0 : 0.0)};
    }

    TriangleMesh AttributedBox(const Source& source)
    {
        TriangleMesh mesh = Box(source.m_minimum, source.m_maximum);
        for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
        {
            const uint64_t face = static_cast<uint64_t>(triangle / 2);
            mesh.m_surfaceKeys.push_back({source.m_namespace, SurfaceBase + face});
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const bool upperChart = source.m_namespace == SourceA.m_namespace && face == 1 && triangle % 2 == 1;
                const auto uv = AuthoredUV(source, Vertex(mesh, mesh.m_indices[triangle * 3 + corner]), face, upperChart);
                mesh.m_cornerUVs.insert(mesh.m_cornerUVs.end(), uv.begin(), uv.end());
            }
        }
        return mesh;
    }

    bool SameMesh(const TriangleMesh& a, const TriangleMesh& b)
    {
        return a.m_positions == b.m_positions && a.m_indices == b.m_indices &&
            a.m_cornerUVs == b.m_cornerUVs && a.m_surfaceKeys == b.m_surfaceKeys;
    }

    bool HasCompleteAttributes(const TriangleMesh& mesh)
    {
        return !mesh.m_indices.empty() && mesh.m_cornerUVs.size() == 2 * mesh.m_indices.size() &&
            mesh.m_surfaceKeys.size() == mesh.TriangleCount();
    }

    // Independent oracle: each triangle must belong to an authored box face,
    // and each corner must retain that face's analytic affine UV chart.
    bool HasAuthoredAttributes(const TriangleMesh& mesh, const std::vector<Source>& sources)
    {
        if (!HasCompleteAttributes(mesh))
        {
            return false;
        }
        constexpr std::array<size_t, 6> faceAxis{2, 2, 1, 1, 0, 0};
        for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
        {
            const SurfaceKey key = mesh.m_surfaceKeys[triangle];
            const auto source = std::find_if(sources.begin(), sources.end(), [key](const Source& input)
                { return input.m_namespace == key.m_namespace; });
            if (source == sources.end() || key.m_surface < SurfaceBase || key.m_surface >= SurfaceBase + 6)
            {
                return false;
            }
            const uint64_t face = key.m_surface - SurfaceBase;
            const size_t axis = faceAxis[static_cast<size_t>(face)];
            const double plane = face % 2 == 0 ? source->m_minimum[axis] : source->m_maximum[axis];
            // The top face of A has a second chart above y=x, even though both
            // triangles share the same surface key. Classify by the triangle
            // centroid so corners on the diagonal retain distinct chart UVs.
            double chartSide = 0.0;
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const uint32_t index = mesh.m_indices[triangle * 3 + corner];
                if (index >= mesh.VertexCount())
                {
                    return false;
                }
                const Point point = Vertex(mesh, index);
                chartSide += point[1] - point[0];
            }
            const bool upperChart = source->m_namespace == SourceA.m_namespace && face == 1 && chartSide > 0.0;
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const size_t index = triangle * 3 + corner;
                if (mesh.m_indices[index] >= mesh.VertexCount())
                {
                    return false;
                }
                const Point point = Vertex(mesh, mesh.m_indices[index]);
                const auto expected = AuthoredUV(*source, point, face, upperChart);
                if (std::abs(point[axis] - plane) > 1e-9)
                {
                    return false;
                }
                for (size_t coordinate = 0; coordinate < 3; ++coordinate)
                {
                    if (point[coordinate] < source->m_minimum[coordinate] - 1e-9 ||
                        point[coordinate] > source->m_maximum[coordinate] + 1e-9)
                    {
                        return false;
                    }
                }
                for (size_t channel = 0; channel < 2; ++channel)
                {
                    const double value = mesh.m_cornerUVs[index * 2 + channel];
                    if (!std::isfinite(value) || std::abs(value - expected[channel]) > 1e-9)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool HasNamespace(const TriangleMesh& mesh, const uint64_t callerNamespace)
    {
        return std::any_of(mesh.m_surfaceKeys.begin(), mesh.m_surfaceKeys.end(), [callerNamespace](const SurfaceKey key)
            { return key.m_namespace == callerNamespace; });
    }

    bool HasUVSeam(const TriangleMesh& mesh, const bool sameSurface = false)
    {
        if (!HasCompleteAttributes(mesh))
        {
            return false;
        }
        for (size_t a = 0; a < mesh.m_indices.size(); ++a)
        {
            for (size_t b = a + 1; b < mesh.m_indices.size(); ++b)
            {
                if (mesh.m_indices[a] == mesh.m_indices[b] &&
                    (!sameSurface || mesh.m_surfaceKeys[a / 3] == mesh.m_surfaceKeys[b / 3]) &&
                    (std::abs(mesh.m_cornerUVs[a * 2] - mesh.m_cornerUVs[b * 2]) > 1e-9 ||
                     std::abs(mesh.m_cornerUVs[a * 2 + 1] - mesh.m_cornerUVs[b * 2 + 1]) > 1e-9))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool HasNewVertex(const TriangleMesh& result, const TriangleMesh& a, const TriangleMesh& b)
    {
        for (size_t vertex = 0; vertex < result.VertexCount(); ++vertex)
        {
            const Point point = Vertex(result, static_cast<uint32_t>(vertex));
            bool original = false;
            for (const TriangleMesh* operand : {&a, &b})
            {
                for (size_t inputVertex = 0; inputVertex < operand->VertexCount(); ++inputVertex)
                {
                    const Point input = Vertex(*operand, static_cast<uint32_t>(inputVertex));
                    original = original || (std::abs(point[0] - input[0]) < 1e-9 &&
                        std::abs(point[1] - input[1]) < 1e-9 && std::abs(point[2] - input[2]) < 1e-9);
                }
            }
            if (!original)
            {
                return true;
            }
        }
        return false;
    }

    void AttributeFixture()
    {
        const TriangleMesh a = AttributedBox(SourceA);
        const TriangleMesh b = AttributedBox(SourceB);
        Check(HasAuthoredAttributes(a, {SourceA}) && HasUVSeam(a, true),
            "authored input has UV seams within one surface key");
        const std::array<BooleanOperation, 3> operations{
            BooleanOperation::Union, BooleanOperation::Subtraction, BooleanOperation::Intersection};
        const std::array<double, 3> volumes{14.125, 6.125, 1.875};
        const std::array<const char*, 3> names{"union", "subtraction", "intersection"};
        for (size_t operation = 0; operation < operations.size(); ++operation)
        {
            TriangleMesh result;
            const std::string label = std::string("attributed ") + names[operation];
            Check(MeshBoolean(a, b, operations[operation], result), (label + " succeeds").c_str());
            Check(std::abs(Volume(result) - volumes[operation]) < 1e-8 && HasClosedOrientedEdges(result),
                (label + " retains authored volume and topology").c_str());
            Check(HasAuthoredAttributes(result, {SourceA, SourceB}),
                (label + " retains discrete source faces and affine corner UVs").c_str());
            Check(HasNamespace(result, SourceA.m_namespace) && HasNamespace(result, SourceB.m_namespace),
                (label + " retains both caller namespaces without collisions").c_str());
            Check(HasNewVertex(result, a, b), (label + " creates intersection vertices").c_str());
            Check(HasUVSeam(result), (label + " retains UV seams on welded geometry").c_str());
            Check(HasUVSeam(result, true), (label + " retains the seam within one surface key").c_str());
        }

        TriangleMesh chain;
        Check(MeshBoolean(a, b, BooleanOperation::Union, chain), "attributed chain union succeeds");
        Check(MeshBoolean(chain, AttributedBox(SourceC), BooleanOperation::Subtraction, chain),
            "attributed chain subtraction supports input/output aliasing");
        Check(std::abs(Volume(chain) - 13.875) < 1e-8 && HasClosedOrientedEdges(chain),
            "attributed chain retains authored volume and topology");
        Check(HasAuthoredAttributes(chain, {SourceA, SourceB, SourceC}) && HasUVSeam(chain),
            "attributed chain retains original UV charts and source faces");
        Check(HasNamespace(chain, SourceA.m_namespace) && HasNamespace(chain, SourceB.m_namespace) &&
            HasNamespace(chain, SourceC.m_namespace), "attributed chain retains all three caller namespaces");

        TriangleMesh result = AttributedBox(SourceC);
        Check(MeshBoolean(a, a, BooleanOperation::Subtraction, result), "attributed A-A succeeds");
        Check(result.m_positions.empty() && result.m_indices.empty() && result.m_cornerUVs.empty() &&
            result.m_surfaceKeys.empty(), "attributed empty result clears geometry and attributes");
        Source distant = SourceB;
        distant.m_minimum[0] += 10.0;
        distant.m_maximum[0] += 10.0;
        result = AttributedBox(SourceC);
        Check(MeshBoolean(a, AttributedBox(distant), BooleanOperation::Intersection, result),
            "attributed disjoint intersection succeeds");
        Check(result.m_positions.empty() && result.m_indices.empty() && result.m_cornerUVs.empty() &&
            result.m_surfaceKeys.empty(), "attributed disjoint intersection clears all buffers");

        result = AttributedBox(SourceC);
        const TriangleMesh previous = result;
        TriangleMesh tiny = a;
        for (double& coordinate : tiny.m_positions)
        {
            coordinate *= 1e-8;
        }
        const bool tinySucceeded = MeshBoolean(tiny, tiny, BooleanOperation::Union, result);
        Check(tinySucceeded || SameMesh(result, previous), "attributed cleanup failure preserves all output buffers");
        Check(!tinySucceeded || HasCompleteAttributes(result), "attributed cleanup success retains complete attributes");
    }

    void CheckInvalidAttributes(const TriangleMesh& invalid, const char* name)
    {
        const TriangleMesh valid = AttributedBox(SourceB);
        const TriangleMesh previous = AttributedBox(SourceC);
        for (const bool firstOperand : {true, false})
        {
            TriangleMesh result = previous;
            const std::string label = std::string(name) + (firstOperand ? " in first operand" : " in second operand");
            Check(!MeshBoolean(firstOperand ? invalid : valid, firstOperand ? valid : invalid,
                BooleanOperation::Union, result), (label + " is rejected").c_str());
            Check(SameMesh(result, previous), (label + " preserves all output buffers").c_str());
        }
    }

    void InvalidAttributeFixture()
    {
        const TriangleMesh a = AttributedBox(SourceA);
        TriangleMesh invalid = a;
        invalid.m_cornerUVs.clear();
        CheckInvalidAttributes(invalid, "missing UV channel");
        invalid = a;
        invalid.m_surfaceKeys.clear();
        CheckInvalidAttributes(invalid, "missing surface channel");
        invalid = a;
        invalid.m_cornerUVs.pop_back();
        CheckInvalidAttributes(invalid, "incomplete corner UV");
        invalid = a;
        invalid.m_cornerUVs.push_back(0.0);
        CheckInvalidAttributes(invalid, "extra corner UV");
        invalid = a;
        invalid.m_surfaceKeys.pop_back();
        CheckInvalidAttributes(invalid, "missing triangle surface");
        invalid = a;
        invalid.m_surfaceKeys.push_back(a.m_surfaceKeys.front());
        CheckInvalidAttributes(invalid, "extra triangle surface");
        CheckInvalidAttributes(Box(SourceA.m_minimum, SourceA.m_maximum), "mixed legacy and attributed operands");
        invalid = a;
        invalid.m_cornerUVs[0] = std::numeric_limits<double>::quiet_NaN();
        CheckInvalidAttributes(invalid, "NaN UV");
        invalid = a;
        invalid.m_cornerUVs[1] = std::numeric_limits<double>::infinity();
        CheckInvalidAttributes(invalid, "infinite UV");
        invalid = a;
        invalid.m_positions[0] = std::numeric_limits<double>::quiet_NaN();
        CheckInvalidAttributes(invalid, "NaN position");
        invalid = a;
        invalid.m_positions[1] = std::numeric_limits<double>::infinity();
        CheckInvalidAttributes(invalid, "infinite position");
    }

    void InvalidIndexFixture()
    {
        TriangleMesh invalid = AttributedBox(SourceA);
        invalid.m_indices[0] = std::numeric_limits<uint32_t>::max();
        CheckInvalidAttributes(invalid, "out of range index");
    }

    void AttributeCleanupFixture()
    {
        TriangleMesh mesh{
            {0.0, 0.0, 0.0, 1e-8, 0.0, 0.0, 0.0, 1e-8, 0.0,
             2.0, 0.0, 0.0, 3.0, 0.0, 0.0, 2.0, 1.0, 0.0},
            {0, 1, 2, 3, 4, 5},
            {0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 10.0, 10.0, 11.0, 10.0, 10.0, 11.0},
            {{SourceA.m_namespace, SurfaceBase}, {SourceB.m_namespace, SurfaceBase}}};
        WeldVertices(mesh, 1e-6);
        Check(mesh.TriangleCount() == 1, "cleanup drops only the geometrically collapsed triangle");
        Check(mesh.m_cornerUVs == std::vector<double>{10.0, 10.0, 11.0, 10.0, 10.0, 11.0},
            "cleanup compacts corner UVs with the surviving triangle");
        Check(mesh.m_surfaceKeys == std::vector<SurfaceKey>{{SourceB.m_namespace, SurfaceBase}},
            "cleanup compacts surface lineage with the surviving triangle");
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

int main(const int argc, char** argv)
{
    // Isolate the historical unchecked access in MeshGL::Merge so an invalid
    // index cannot prevent the other behavioral regressions from running.
    if (argc == 2 && std::string(argv[1]) == "--invalid-index")
    {
        InvalidIndexFixture();
        return failures == 0 ? 0 : 1;
    }
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
    AttributeFixture();
    InvalidAttributeFixture();
    AttributeCleanupFixture();
    return failures == 0 ? 0 : 1;
}
