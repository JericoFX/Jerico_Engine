// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <poly2tri.h>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace JE::Tools
{
    void Require(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    std::vector<p2t::Point*> References(std::vector<p2t::Point>& points)
    {
        std::vector<p2t::Point*> result;
        for (p2t::Point& point : points)
        {
            result.push_back(&point);
        }
        return result;
    }

    double Area(const std::vector<p2t::Triangle*>& triangles, const std::vector<p2t::Point*>& inputs)
    {
        double total = 0.0;
        for (p2t::Triangle* triangle : triangles)
        {
            Require(triangle != nullptr, "triangle is present");
            p2t::Point* vertices[3] = {triangle->GetPoint(0), triangle->GetPoint(1), triangle->GetPoint(2)};
            for (p2t::Point* vertex : vertices)
            {
                bool found = false;
                for (p2t::Point* input : inputs)
                {
                    found |= vertex == input;
                }
                Require(found, "triangle preserves caller-owned input vertex identity");
            }
            const double cross = (vertices[1]->x - vertices[0]->x) * (vertices[2]->y - vertices[0]->y)
                - (vertices[1]->y - vertices[0]->y) * (vertices[2]->x - vertices[0]->x);
            Require(std::isfinite(cross) && std::abs(cross) > 0.0, "finite nondegenerate triangle");
            total += std::abs(cross) * 0.5;
        }
        return total;
    }

    void Concave()
    {
        // PhysX's polygon-prism authoring uses this CDT/Triangulate/GetTriangles path.
        std::vector<p2t::Point> points{{0, 0}, {3, 0}, {3, 1}, {1, 1}, {1, 3}, {0, 3}};
        const auto references = References(points);
        p2t::CDT triangulation(references);
        triangulation.Triangulate();
        const auto triangles = triangulation.GetTriangles();
        Require(triangles.size() == 4, "six-vertex concave polygon has four triangles");
        Require(std::abs(Area(triangles, references) - 5.0) <= 1e-12, "concave polygon area is five");
        for (p2t::Triangle* triangle : triangles)
        {
            const double x = (triangle->GetPoint(0)->x + triangle->GetPoint(1)->x + triangle->GetPoint(2)->x) / 3.0;
            const double y = (triangle->GetPoint(0)->y + triangle->GetPoint(1)->y + triangle->GetPoint(2)->y) / 3.0;
            Require(!(x > 1.0 && y > 1.0), "no triangle centroid fills the concave opening");
        }
    }

    void Hole()
    {
        std::vector<p2t::Point> outer{{0, 0}, {6, 0}, {6, 6}, {0, 6}};
        std::vector<p2t::Point> inner{{2, 2}, {2, 4}, {4, 4}, {4, 2}};
        const auto outerReferences = References(outer);
        const auto innerReferences = References(inner);
        auto all = outerReferences;
        all.insert(all.end(), innerReferences.begin(), innerReferences.end());
        p2t::CDT triangulation(outerReferences);
        triangulation.AddHole(innerReferences);
        triangulation.Triangulate();
        const auto triangles = triangulation.GetTriangles();
        Require(triangles.size() == 8, "square with four-vertex hole has eight triangles");
        Require(std::abs(Area(triangles, all) - 32.0) <= 1e-12, "hole area remains excluded");
        for (p2t::Triangle* triangle : triangles)
        {
            const double x = (triangle->GetPoint(0)->x + triangle->GetPoint(1)->x + triangle->GetPoint(2)->x) / 3.0;
            const double y = (triangle->GetPoint(0)->y + triangle->GetPoint(1)->y + triangle->GetPoint(2)->y) / 3.0;
            Require(!(x > 2.0 && x < 4.0 && y > 2.0 && y < 4.0), "no triangle centroid fills the hole");
        }
    }

    void Steiner()
    {
        std::vector<p2t::Point> points{{0, 0}, {6, 0}, {6, 6}, {0, 6}};
        auto references = References(points);
        p2t::Point center(3, 3);
        p2t::CDT triangulation(references);
        triangulation.AddPoint(&center);
        triangulation.Triangulate();
        references.push_back(&center);
        const auto triangles = triangulation.GetTriangles();
        Require(triangles.size() == 4, "one interior Steiner point produces four triangles");
        Require(std::abs(Area(triangles, references) - 36.0) <= 1e-12, "Steiner insertion preserves area");
        size_t centerUses = 0;
        for (p2t::Triangle* triangle : triangles)
        {
            for (int index = 0; index < 3; ++index)
            {
                centerUses += triangle->GetPoint(index) == &center ? 1 : 0;
            }
        }
        Require(centerUses == 4, "Steiner point participates in all four triangles");
    }

    void Repeated()
    {
        // The documented primitive rejects coincident edge endpoints. The README
        // does not promise general validation of arbitrary malformed CDT polygons.
        p2t::Point first(7, 9), repeated(7, 9);
        bool rejected = false;
        try
        {
            p2t::Edge edge(first, repeated);
            (void)edge;
        }
        catch (const std::runtime_error& error)
        {
            rejected = std::string(error.what()) == "Edge::Edge: p1 == p2";
        }
        Require(rejected, "coincident edge is rejected with the upstream diagnostic");
        Require(first.x == 7 && first.y == 9 && repeated.x == 7 && repeated.y == 9
            && first.edge_list.empty() && repeated.edge_list.empty(), "rejection does not modify caller points/edges");
    }
}

int main(int argc, char** argv)
{
    try
    {
        JE::Tools::Require(argc == 2, "one consumer case is required");
        const std::string mode(argv[1]);
        if (mode == "concave") { JE::Tools::Concave(); }
        else if (mode == "hole") { JE::Tools::Hole(); }
        else if (mode == "steiner") { JE::Tools::Steiner(); }
        else if (mode == "repeated") { JE::Tools::Repeated(); }
        else { throw std::runtime_error("unknown consumer case"); }
        std::printf("CASE %s PASS\n", mode.c_str());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
