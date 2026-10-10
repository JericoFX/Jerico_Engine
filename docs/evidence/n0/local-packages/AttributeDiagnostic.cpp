// Temporary diagnostic output: include the frozen regressions without changing their oracles.
#define main FrozenRegressionMain
#include "F:/JericoEngine/JericoEngine/Gems/WhiteBox/Code/Tests/CSG/WhiteBoxCsgCoreTest.cpp"
#undef main
#include <iomanip>

namespace
{
    void DescribeAttributes(const char* name, const TriangleMesh& mesh, const std::vector<Source>& sources)
    {
        if (!HasCompleteAttributes(mesh))
        {
            std::cout << name << " INCOMPLETE\n";
            return;
        }
        size_t keyErrors = 0;
        size_t planeErrors = 0;
        size_t boundErrors = 0;
        size_t uvErrors = 0;
        size_t chartCrossings = 0;
        constexpr std::array<size_t, 6> axes{2, 2, 1, 1, 0, 0};
        for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
        {
            const SurfaceKey key = mesh.m_surfaceKeys[triangle];
            const auto source = std::find_if(sources.begin(), sources.end(), [key](const Source& candidate)
                { return candidate.m_namespace == key.m_namespace; });
            if (source == sources.end() || key.m_surface < SurfaceBase || key.m_surface >= SurfaceBase + 6)
            {
                ++keyErrors;
                continue;
            }
            const uint64_t face = key.m_surface - SurfaceBase;
            const size_t axis = axes[static_cast<size_t>(face)];
            const double plane = face % 2 == 0 ? source->m_minimum[axis] : source->m_maximum[axis];
            double chartSide = 0.0;
            double minimumSide = std::numeric_limits<double>::infinity();
            double maximumSide = -std::numeric_limits<double>::infinity();
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const Point point = Vertex(mesh, mesh.m_indices[triangle * 3 + corner]);
                const double side = point[1] - point[0];
                chartSide += side;
                minimumSide = std::min(minimumSide, side);
                maximumSide = std::max(maximumSide, side);
            }
            const bool upperChart = key.m_namespace == SourceA.m_namespace && face == 1 && chartSide > 0.0;
            if (key.m_namespace == SourceA.m_namespace && face == 1 && minimumSide < -1e-9 && maximumSide > 1e-9)
            {
                ++chartCrossings;
            }
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const size_t index = triangle * 3 + corner;
                const Point point = Vertex(mesh, mesh.m_indices[index]);
                planeErrors += std::abs(point[axis] - plane) > 1e-9 ? 1 : 0;
                for (size_t coordinate = 0; coordinate < 3; ++coordinate)
                {
                    boundErrors += point[coordinate] < source->m_minimum[coordinate] - 1e-9 ||
                        point[coordinate] > source->m_maximum[coordinate] + 1e-9 ? 1 : 0;
                }
                const auto expected = AuthoredUV(*source, point, face, upperChart);
                for (size_t channel = 0; channel < 2; ++channel)
                {
                    const double actual = mesh.m_cornerUVs[index * 2 + channel];
                    if (!std::isfinite(actual) || std::abs(actual - expected[channel]) > 1e-9)
                    {
                        if (uvErrors < 8)
                        {
                            std::cout << name << " UV tri=" << triangle << " corner=" << corner << " channel=" << channel
                                << " key=" << key.m_namespace << ':' << key.m_surface
                                << " xyz=" << point[0] << ',' << point[1] << ',' << point[2]
                                << " actual=" << actual << " expected=" << expected[channel]
                                << " delta=" << actual - expected[channel] << " chartSide=" << chartSide << '\n';
                        }
                        ++uvErrors;
                    }
                }
            }
        }
        std::cout << name << " SUMMARY triangles=" << mesh.TriangleCount() << " KEY=" << keyErrors
            << " PLANE=" << planeErrors << " BOUNDS=" << boundErrors << " UV=" << uvErrors
            << " CHART_CROSSINGS=" << chartCrossings << '\n';
    }
}

int main()
{
    std::cout << std::setprecision(17);
    const TriangleMesh a = AttributedBox(SourceA);
    const TriangleMesh b = AttributedBox(SourceB);
    DescribeAttributes("inputA", a, {SourceA});
    DescribeAttributes("inputB", b, {SourceB});
    const std::array<BooleanOperation, 3> operations{
        BooleanOperation::Union, BooleanOperation::Subtraction, BooleanOperation::Intersection};
    const std::array<const char*, 3> names{"union", "subtraction", "intersection"};
    for (size_t operation = 0; operation < operations.size(); ++operation)
    {
        TriangleMesh result;
        if (!MeshBoolean(a, b, operations[operation], result))
        {
            return 2;
        }
        DescribeAttributes(names[operation], result, {SourceA, SourceB});
    }
    TriangleMesh chain;
    if (!MeshBoolean(a, b, BooleanOperation::Union, chain) ||
        !MeshBoolean(chain, AttributedBox(SourceC), BooleanOperation::Subtraction, chain))
    {
        return 3;
    }
    DescribeAttributes("chain", chain, {SourceA, SourceB, SourceC});
    return 0;
}
