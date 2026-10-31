/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/numeric.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <cstdint>
#include <cmath>
#include <JE/Render/RPI/Reflect/Model/ModelKdTree.h>
#include <AzCore/Math/IntersectSegment.h>

namespace JE::Render
{
    namespace RPI
    {
        AZStd::tuple<ModelKdTree::ESplitAxis, float> ModelKdTree::SearchForBestSplitAxis(const AZ::Aabb& aabb)
        {
            const float xsize = aabb.GetXExtent();
            const float ysize = aabb.GetYExtent();
            const float zsize = aabb.GetZExtent();

            if (xsize >= ysize && xsize >= zsize)
            {
                return {ModelKdTree::eSA_X, aabb.GetMin().GetX() + xsize * 0.5f};
            }
            if (ysize >= zsize && ysize >= xsize)
            {
                return {ModelKdTree::eSA_Y, aabb.GetMin().GetY() + ysize * 0.5f};
            }
            return {ModelKdTree::eSA_Z, aabb.GetMin().GetZ() + zsize * 0.5f};
        }

        bool ModelKdTree::SplitNode(const AZ::Aabb& boundbox, const AZStd::vector<ObjectIdTriangleIndices>& indices, ModelKdTree::ESplitAxis splitAxis, float splitPos, SSplitInfo& outInfo)
        {
            if (splitAxis != ModelKdTree::eSA_X && splitAxis != ModelKdTree::eSA_Y && splitAxis != ModelKdTree::eSA_Z)
            {
                return false;
            }

            outInfo.m_aboveBoundbox = boundbox;
            outInfo.m_belowBoundbox = boundbox;

            {
                AZ::Vector3 maxBound = outInfo.m_aboveBoundbox.GetMax();
                maxBound.SetElement(splitAxis, splitPos);
                outInfo.m_aboveBoundbox.SetMax(maxBound);
            }
            {
                AZ::Vector3 minBound = outInfo.m_belowBoundbox.GetMin();
                minBound.SetElement(splitAxis, splitPos);
                outInfo.m_belowBoundbox.SetMin(minBound);
            }

            const AZ::u32 iIndexSize = aznumeric_cast<AZ::u32>(indices.size());
            outInfo.m_aboveIndices.reserve(iIndexSize);
            outInfo.m_belowIndices.reserve(iIndexSize);

            for (const auto& [nObjIndex, triangleIndices, primitiveIndex] : indices)
            {
                const auto& [first, second, third] = triangleIndices;

                const AZStd::span<const float>& positionBuffer = m_meshes[nObjIndex].m_vertexData;

                if (positionBuffer.empty())
                {
                    continue;
                }

                // If the split axis is Y, this uses a Vector3 to store the Y positions of each vertex in the triangle.
                const AZStd::array<const float, 3> triangleVerticesValuesForThisSplitAxis {
                    positionBuffer[size_t{first} * 3 + splitAxis], positionBuffer[size_t{second} * 3 + splitAxis], positionBuffer[size_t{third} * 3 + splitAxis]
                };

                if (AZStd::any_of(begin(triangleVerticesValuesForThisSplitAxis), end(triangleVerticesValuesForThisSplitAxis), [splitPos](const float value) { return value < splitPos; }))
                {
                    outInfo.m_aboveIndices.emplace_back(nObjIndex, triangleIndices, primitiveIndex);
                }
                if (AZStd::any_of(begin(triangleVerticesValuesForThisSplitAxis), end(triangleVerticesValuesForThisSplitAxis), [splitPos](const float value) { return value >= splitPos; }))
                {
                    outInfo.m_belowIndices.emplace_back(nObjIndex, triangleIndices, primitiveIndex);
                }
            }

            // If either the top or bottom contain all the input indices, the triangles are too close to cut any
            // further and the split failed
            // Additionally, if too many triangles straddle the split-axis,
            // the triangles are too close and the split failed
            // [ATOM-15944] - Use a more sophisticated method to terminate KdTree generation
            return indices.size() != outInfo.m_aboveIndices.size() && indices.size() != outInfo.m_belowIndices.size()
                && aznumeric_cast<float>(outInfo.m_aboveIndices.size() + outInfo.m_belowIndices.size()) / aznumeric_cast<float>(indices.size()) < s_MaximumSplitAxisStraddlingTriangles;
        }

        ModelKdTree::ModelKdTree() = default;

        ModelKdTree::ModelKdTree(ModelKdTree&&) = default;

        ModelKdTree::~ModelKdTree() = default;

        bool ModelKdTree::Build(const ModelAsset* model)
        {
            m_pRootNode.reset();
            m_meshes.clear();
            m_primitiveCount = 0;
            m_queryStatus = ReadRayMeshes(model, m_meshes, m_primitiveCount);
            if (m_queryStatus != ModelRayQueryStatus::Complete)
            {
                return false;
            }
            AZ::Aabb entireBoundBox = AZ::Aabb::CreateNull();
            AZStd::vector<ObjectIdTriangleIndices> indices;
            indices.reserve(m_primitiveCount);
            for (AZ::u32 meshIndex = 0; meshIndex < m_meshes.size(); ++meshIndex)
            {
                const auto& mesh = m_meshes[meshIndex];
                for (size_t positionIndex = 0; positionIndex < mesh.m_vertexData.size(); positionIndex += 3)
                {
                    entireBoundBox.AddPoint({mesh.m_vertexData[positionIndex], mesh.m_vertexData[positionIndex + 1],
                        mesh.m_vertexData[positionIndex + 2]});
                }
                for (AZ::u32 primitiveIndex = 0; primitiveIndex < mesh.m_triangles.size(); ++primitiveIndex)
                {
                    indices.emplace_back(meshIndex, mesh.m_triangles[primitiveIndex], primitiveIndex);
                }
            }
            m_pRootNode = AZStd::make_unique<ModelKdTreeNode>();
            BuildRecursively(m_pRootNode.get(), entireBoundBox, indices);
            return true;
        }

        AZStd::span<const float> ModelKdTree::GetPositionsBuffer(const ModelLodAsset::Mesh& mesh)
        {
            AZStd::span<const float> positionBuffer = mesh.GetSemanticBufferTyped<float>(AZ::Name{"POSITION"});
            AZ_Warning("ModelKdTree", !positionBuffer.empty(), "Could not find position buffers in a mesh");
            return positionBuffer;
        }

        AZStd::span<const ModelKdTree::TriangleIndices> ModelKdTree::GetIndexBuffer(const ModelLodAsset::Mesh& mesh)
        {
            return mesh.GetIndexBufferTyped<ModelKdTree::TriangleIndices>();
        }

        void ModelKdTree::BuildRecursively(ModelKdTreeNode* pNode, const AZ::Aabb& boundbox, AZStd::vector<ObjectIdTriangleIndices>& indices)
        {
            pNode->SetBoundBox(boundbox);

            if (indices.size() <= s_MinimumVertexSizeInLeafNode)
            {
                pNode->SetVertexIndexBuffer(AZStd::move(indices));
                return;
            }

            const auto [splitAxis, splitPos] = SearchForBestSplitAxis(boundbox);
            pNode->SetSplitAxis(splitAxis);
            pNode->SetSplitPos(splitPos);

            SSplitInfo splitInfo;
            if (!SplitNode(boundbox, indices, splitAxis, splitPos, splitInfo))
            {
                pNode->SetVertexIndexBuffer(AZStd::move(indices));
                return;
            }

            if (splitInfo.m_aboveIndices.empty() || splitInfo.m_belowIndices.empty())
            {
                pNode->SetVertexIndexBuffer(AZStd::move(indices));
                return;
            }

            pNode->SetChild(0, AZStd::make_unique<ModelKdTreeNode>());
            pNode->SetChild(1, AZStd::make_unique<ModelKdTreeNode>());

            BuildRecursively(pNode->GetChild(0), splitInfo.m_aboveBoundbox, splitInfo.m_aboveIndices);
            BuildRecursively(pNode->GetChild(1), splitInfo.m_belowBoundbox, splitInfo.m_belowIndices);
        }

        namespace
        {
            // Check offsets by division before multiplication, including on narrower size_t platforms.
            ModelRayQueryStatus ReadRayBuffer(const BufferAssetView& view, size_t elementSize,
                AZStd::span<const uint8_t>& bytes)
            {
                const auto* asset = view.GetBufferAsset().Get();
                if (!asset)
                {
                    return ModelRayQueryStatus::Unavailable;
                }
                const auto raw = asset->GetBuffer();
                const auto& descriptor = view.GetBufferViewDescriptor();
                if (descriptor.m_elementSize != elementSize)
                {
                    return ModelRayQueryStatus::InvalidGeometry;
                }
                // A valid non-empty view may have no CPU payload yet, even when its offset is nonzero.
                if (raw.empty() && descriptor.m_elementCount != 0)
                {
                    return ModelRayQueryStatus::Unavailable;
                }
                if (descriptor.m_elementOffset > raw.size() / elementSize)
                {
                    return ModelRayQueryStatus::InvalidGeometry;
                }
                const size_t offset = size_t{descriptor.m_elementOffset} * elementSize;
                if (descriptor.m_elementCount > (raw.size() - offset) / elementSize)
                {
                    return ModelRayQueryStatus::InvalidGeometry;
                }
                const size_t count = size_t{descriptor.m_elementCount} * elementSize;
                bytes = raw.subspan(offset, count);
                if (!bytes.empty() && reinterpret_cast<std::uintptr_t>(bytes.data()) % alignof(float) != 0)
                {
                    return ModelRayQueryStatus::InvalidGeometry;
                }
                return ModelRayQueryStatus::Complete;
            }

            AZ::Vector3 RayVertex(AZStd::span<const float> positions, AZ::u32 index)
            {
                const size_t offset = size_t{index} * 3;
                return {positions[offset], positions[offset + 1], positions[offset + 2]};
            }

            struct RayHitCollector
            {
                AZStd::span<ModelRayHit> m_hits;
                ModelRayQueryResult m_result{ModelRayQueryStatus::Complete};

                static bool Before(const ModelRayHit& a, const ModelRayHit& b)
                {
                    if (a.m_distanceNormalized != b.m_distanceNormalized) { return a.m_distanceNormalized < b.m_distanceNormalized; }
                    if (a.m_meshIndex != b.m_meshIndex) { return a.m_meshIndex < b.m_meshIndex; }
                    return a.m_primitiveIndex < b.m_primitiveIndex;
                }

                void Add(const ModelRayHit& hit)
                {
                    size_t insertion = 0;
                    while (insertion < m_result.m_hitCount && Before(m_hits[insertion], hit)) { ++insertion; }
                    if (m_result.m_hitCount < m_hits.size())
                    {
                        ++m_result.m_hitCount;
                    }
                    else
                    {
                        ++m_result.m_discardedHitCount;
                    }
                    if (insertion < m_result.m_hitCount)
                    {
                        for (size_t i = m_result.m_hitCount - 1; i > insertion; --i) { m_hits[i] = m_hits[i - 1]; }
                        m_hits[insertion] = hit;
                    }
                }

                ModelRayQueryResult Commit(AZStd::span<ModelRayHit> output)
                {
                    if (m_result.m_hitCount != 0) { AZStd::copy(m_hits.begin(), m_hits.begin() + m_result.m_hitCount, output.begin()); }
                    m_result.m_status = m_result.m_discardedHitCount ? ModelRayQueryStatus::Truncated : ModelRayQueryStatus::Complete;
                    return m_result;
                }
            };

            // False means an invalid numerical intersection, not an ordinary miss. Scratch is committed only after success.
            bool CollectRayTriangle(const AZ::Intersect::SegmentTriangleHitTester& tester,
                AZStd::span<const float> positions, const ModelKdTree::TriangleIndices& triangle,
                AZ::u32 meshIndex, AZ::u32 primitiveIndex, ModelMaterialSlot::StableId materialSlot,
                ModelRaySidedness sidedness, RayHitCollector& collector)
            {
                const AZ::Vector3 a = RayVertex(positions, triangle.index1);
                const AZ::Vector3 b = RayVertex(positions, triangle.index2);
                const AZ::Vector3 c = RayVertex(positions, triangle.index3);
                const AZ::Vector3 winding = (b - a).Cross(c - a);
                const AZ::Vector3 absolute = winding.GetAbs();
                const float scale = AZStd::max(AZStd::max(absolute.GetX(), absolute.GetY()), absolute.GetZ());
                if (scale == 0.0f) { return true; } // Finite, exactly degenerate primitives are not hits or discarded hits.
                float distance = 0.0f;
                AZ::Vector3 ignoredNormal = AZ::Vector3::CreateZero();
                const bool intersects = sidedness == ModelRaySidedness::FrontFace
                    ? tester.IntersectSegmentTriangleCCW(a, b, c, ignoredNormal, distance)
                    : sidedness == ModelRaySidedness::BackFace
                        ? tester.IntersectSegmentTriangleCCW(a, c, b, ignoredNormal, distance)
                        : tester.IntersectSegmentTriangle(a, b, c, ignoredNormal, distance);
                if (intersects)
                {
                    const AZ::Vector3 normal = (winding / scale).GetNormalized();
                    if (!AZ::IsFiniteFloat(distance) || distance < 0.0f || distance > 1.0f || !normal.IsFinite()) { return false; }
                    ModelRayHit hit;
                    hit.m_distanceNormalized = distance;
                    hit.m_normal = normal;
                    hit.m_meshIndex = meshIndex;
                    hit.m_primitiveIndex = primitiveIndex;
                    if (materialSlot != ModelMaterialSlot::InvalidStableId) { hit.m_materialSlotId = materialSlot; }
                    collector.Add(hit);
                }
                return true;
            }
        }

        bool ModelKdTree::ValidateRayRequest(const AZ::Vector3& start, const AZ::Vector3& direction,
            ModelRaySidedness sidedness, AZStd::span<ModelRayHit> output)
        {
            const auto end = start + direction;
            return start.IsFinite() && direction.IsFinite() && end.IsFinite() && end != start
                && (sidedness == ModelRaySidedness::FrontFace || sidedness == ModelRaySidedness::DoubleSided || sidedness == ModelRaySidedness::BackFace)
                && (output.empty() || output.data() != nullptr)
                && output.size() <= AZStd::numeric_limits<size_t>::max() / sizeof(ModelRayHit);
        }

        bool ModelKdTree::ValidateRayMeshes(AZStd::span<const MeshData> meshes)
        {
            for (const auto& mesh : meshes)
            {
                if (mesh.m_vertexData.size() % 3 != 0) { return false; }
                for (size_t i = 0; i < mesh.m_vertexData.size(); i += 3)
                {
                    if (!AZ::Vector3(mesh.m_vertexData[i], mesh.m_vertexData[i + 1], mesh.m_vertexData[i + 2]).IsFinite()) { return false; }
                }
                for (const auto& triangle : mesh.m_triangles)
                {
                    const size_t vertexCount = mesh.m_vertexData.size() / 3;
                    if (triangle.index1 >= vertexCount || triangle.index2 >= vertexCount || triangle.index3 >= vertexCount) { return false; }
                    const auto a = RayVertex(mesh.m_vertexData, triangle.index1);
                    const auto b = RayVertex(mesh.m_vertexData, triangle.index2);
                    const auto c = RayVertex(mesh.m_vertexData, triangle.index3);
                    if (!(b - a).IsFinite() || !(c - a).IsFinite() || !(b - a).Cross(c - a).IsFinite()) { return false; }
                }
            }
            return true;
        }

        ModelRayQueryStatus ModelKdTree::ReadRayMeshes(const ModelAsset* model, AZStd::vector<MeshData>& meshes, size_t& primitiveCount)
        {
            meshes.clear();
            primitiveCount = 0;
            if (!model || model->GetLodAssets().empty() || !model->GetLodAssets()[0].Get()) { return ModelRayQueryStatus::Unavailable; }
            const auto sourceMeshes = model->GetLodAssets()[0]->GetMeshes();
            if (sourceMeshes.size() > AZStd::numeric_limits<AZ::u32>::max()) { return ModelRayQueryStatus::InvalidGeometry; }
            meshes.reserve(sourceMeshes.size());
            for (const auto& source : sourceMeshes)
            {
                const auto* positions = source.GetSemanticBufferAssetView(AZ::Name("POSITION"));
                if (!positions) { return ModelRayQueryStatus::InvalidGeometry; }
                AZStd::span<const uint8_t> positionBytes;
                AZStd::span<const uint8_t> indexBytes;
                auto status = ReadRayBuffer(*positions, sizeof(float) * 3, positionBytes);
                if (status != ModelRayQueryStatus::Complete) { return status; }
                status = ReadRayBuffer(source.GetIndexBufferAssetView(), sizeof(AZ::u32), indexBytes);
                if (status != ModelRayQueryStatus::Complete) { return status; }
                if (indexBytes.size() % sizeof(TriangleIndices) != 0) { return ModelRayQueryStatus::InvalidGeometry; }
                const size_t triangleCount = indexBytes.size() / sizeof(TriangleIndices);
                // Existing tree node counts are u32. Reject overflow, never wrap/truncate the model or a node.
                if (triangleCount > AZStd::numeric_limits<AZ::u32>::max() - primitiveCount) { return ModelRayQueryStatus::InvalidGeometry; }
                meshes.push_back({&source,
                    {reinterpret_cast<const float*>(positionBytes.data()), positionBytes.size() / sizeof(float)},
                    {reinterpret_cast<const TriangleIndices*>(indexBytes.data()), triangleCount}, primitiveCount});
                primitiveCount += triangleCount;
            }
            return ValidateRayMeshes({meshes.data(), meshes.size()}) ? ModelRayQueryStatus::Complete : ModelRayQueryStatus::InvalidGeometry;
        }

        ModelRayQueryResult ModelKdTree::BruteForceRayIntersections(AZStd::span<const MeshData> meshes,
            const AZ::Vector3& start, const AZ::Vector3& direction,
            ModelRaySidedness sidedness, AZStd::span<ModelRayHit> output)
        {
            AZStd::vector<ModelRayHit> scratch(output.size());
            RayHitCollector collector{{scratch.data(), scratch.size()}};
            const AZ::Intersect::SegmentTriangleHitTester tester(start, start + direction);
            for (AZ::u32 meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
            {
                const auto& mesh = meshes[meshIndex];
                for (AZ::u32 primitive = 0; primitive < mesh.m_triangles.size(); ++primitive)
                {
                    if (!CollectRayTriangle(tester, mesh.m_vertexData, mesh.m_triangles[primitive], meshIndex, primitive,
                        mesh.m_mesh->GetMaterialSlotId(), sidedness, collector)) { return {ModelRayQueryStatus::InvalidGeometry}; }
                }
            }
            return collector.Commit(output);
        }

        ModelRayQueryResult ModelKdTree::RayIntersections(const AZ::Vector3& start, const AZ::Vector3& direction,
            ModelRaySidedness sidedness, AZStd::span<ModelRayHit> output) const
        {
            if (!ValidateRayRequest(start, direction, sidedness, output)) { return {ModelRayQueryStatus::InvalidInput}; }
            if (m_queryStatus != ModelRayQueryStatus::Complete) { return {m_queryStatus}; }
            if (!m_pRootNode) { return {ModelRayQueryStatus::Unavailable}; }
            // No geometry copy per query. Validate all resident spans before publishing any partial output.
            if (!ValidateRayMeshes({m_meshes.data(), m_meshes.size()})) { return {ModelRayQueryStatus::InvalidGeometry}; }
            if (m_primitiveCount == 0) { return {ModelRayQueryStatus::Complete}; }
            AZStd::vector<ModelRayHit> scratch(output.size());
            AZStd::vector<AZ::u8> visited(m_primitiveCount, 0);
            RayHitCollector collector{{scratch.data(), scratch.size()}};
            const AZ::Intersect::SegmentTriangleHitTester tester(start, start + direction);
            const auto reciprocal = direction.GetReciprocal();
            const auto visit = [&](auto&& self, ModelKdTreeNode* node) -> bool
            {
                if (!node) { return true; }
                float entry = 0.0f, exit = 0.0f;
                if (AZ::Intersect::IntersectRayAABB2(start, reciprocal, node->GetBoundBox(), entry, exit) == AZ::Intersect::ISECT_RAY_AABB_NONE
                    || entry > 1.0f || exit < 0.0f) { return true; }
                if (!node->IsLeaf()) { return self(self, node->GetChild(0)) && self(self, node->GetChild(1)); }
                for (AZ::u32 i = 0; i < node->GetVertexBufferSize(); ++i)
                {
                    const AZ::u32 meshIndex = node->GetObjIndex(i);
                    const AZ::u32 primitive = node->GetPrimitiveIndex(i);
                    const auto& mesh = m_meshes[meshIndex];
                    const size_t original = mesh.m_primitiveOffset + primitive;
                    if (visited[original]) { continue; }
                    visited[original] = 1;
                    if (!CollectRayTriangle(tester, mesh.m_vertexData, node->GetVertexIndex(i), meshIndex, primitive,
                        mesh.m_mesh->GetMaterialSlotId(), sidedness, collector)) { return false; }
                }
                return true;
            };
            if (!visit(visit, m_pRootNode.get())) { return {ModelRayQueryStatus::InvalidGeometry}; }
            return collector.Commit(output);
        }

        namespace
        {
            struct VolumeIntersectionContext
            {
                AZ::Aabb m_bounds = AZ::Aabb::CreateNull();
                AZ::Aabb m_localBounds = AZ::Aabb::CreateNull();
                AZStd::array<AZ::Vector3, 8> m_worldCorners, m_localCorners;
                AZStd::array<AZStd::array<size_t, 2>, 12> m_edgeCorners;
                AZStd::fixed_vector<AZ::Intersect::SegmentTriangleHitTester, 12> m_edgeTesters;

                bool Initialize(const AZ::Aabb& bounds, const AZ::Transform& pose, const AZ::Vector3& scale)
                {
                    m_bounds = bounds;
                    const auto inverse = pose.GetInverse();
                    if (!inverse.IsFinite()) { return false; }
                    for (size_t i = 0; i < 8; ++i)
                    {
                        m_worldCorners[i] = {(i & 1) ? bounds.GetMax().GetX() : bounds.GetMin().GetX(),
                            (i & 2) ? bounds.GetMax().GetY() : bounds.GetMin().GetY(),
                            (i & 4) ? bounds.GetMax().GetZ() : bounds.GetMin().GetZ()};
                        m_localCorners[i] = inverse.TransformPoint(m_worldCorners[i]) / scale;
                        if (!m_localCorners[i].IsFinite()) { return false; }
                        m_localBounds.AddPoint(m_localCorners[i]);
                    }
                    for (size_t corner = 0; corner < 8; ++corner)
                    {
                        for (size_t axis = 0; axis < 3; ++axis)
                        {
                            const size_t bit = size_t{1} << axis;
                            if (corner & bit) { continue; }
                            const size_t other = corner | bit;
                            const float lengthSq = (m_localCorners[other] - m_localCorners[corner]).GetLengthSq();
                            if (!std::isfinite(lengthSq) || lengthSq <= 0) { return false; }
                            m_edgeCorners[m_edgeTesters.size()] = {corner, other};
                            m_edgeTesters.emplace_back(m_localCorners[corner], m_localCorners[other]);
                        }
                    }
                    return true;
                }

                bool ClipEdge(const AZ::Vector3& start, const AZ::Vector3& end, AZ::Vector3& point) const
                {
                    // Native clipper has an absolute parallel epsilon. Condition each nonzero axis to +/-1,
                    // preserving t without broadening the box or changing the renderer's geometry tolerance.
                    AZ::Vector3 lower, upper, direction;
                    for (int32_t axis = 0; axis < 3; ++axis)
                    {
                        const double origin = start.GetElement(axis);
                        const double delta = static_cast<double>(end.GetElement(axis)) - origin;
                        if (delta == 0)
                        {
                            if (origin < m_bounds.GetMin().GetElement(axis) || origin > m_bounds.GetMax().GetElement(axis)) { return false; }
                            lower.SetElement(axis, -1); upper.SetElement(axis, 1); direction.SetElement(axis, 0);
                        }
                        else
                        {
                            const auto normalized = [&](float value)
                            { return static_cast<float>(AZStd::max(-2.0, AZStd::min(2.0, (static_cast<double>(value) - origin) / std::fabs(delta)))); };
                            lower.SetElement(axis, normalized(m_bounds.GetMin().GetElement(axis)));
                            upper.SetElement(axis, normalized(m_bounds.GetMax().GetElement(axis)));
                            direction.SetElement(axis, delta > 0 ? 1.0f : -1.0f);
                        }
                    }
                    auto clipped = AZ::Vector3::CreateZero();
                    float enter, exit;
                    if (!AZ::Intersect::ClipRayWithAabb(AZ::Aabb::CreateFromMinMax(lower, upper), clipped, direction, enter, exit) ||
                        !std::isfinite(enter) || !std::isfinite(exit) || enter < 0 || enter > 1 || exit < enter) { return false; }
                    const double fraction = (static_cast<double>(enter) + AZStd::min(1.0, static_cast<double>(exit))) * 0.5;
                    for (int32_t axis = 0; axis < 3; ++axis)
                    {
                        const double origin = start.GetElement(axis);
                        point.SetElement(axis, static_cast<float>(origin + (static_cast<double>(end.GetElement(axis)) - origin) * fraction));
                    }
                    return m_bounds.Contains(point);
                }

                bool Intersect(const AZStd::array<AZ::Vector3, 3>& local,
                    const AZStd::array<AZ::Vector3, 3>& world, AZ::Vector3& point) const
                {
                    // Exhaustive convex surface intersection: 3 vertices, 3 triangle edges, ALL 12 box edges.
                    for (const auto& vertex : world) { if (m_bounds.Contains(vertex)) { point = vertex; return true; } }
                    for (size_t edge = 0; edge < 3; ++edge)
                    { if (ClipEdge(world[edge], world[(edge + 1) % 3], point)) { return true; } }
                    for (size_t edge = 0; edge < m_edgeTesters.size(); ++edge)
                    {
                        AZ::Vector3 ignoredNormal; float fraction;
                        if (!m_edgeTesters[edge].IntersectSegmentTriangle(local[0], local[1], local[2], ignoredNormal, fraction) ||
                            !std::isfinite(fraction) || fraction < 0 || fraction > 1) { continue; }
                        const auto& indices = m_edgeCorners[edge];
                        point = m_worldCorners[indices[0]] + (m_worldCorners[indices[1]] - m_worldCorners[indices[0]]) * fraction;
                        if (m_bounds.Contains(point)) { return true; }
                    }
                    return false;
                }
            };

            bool VolumeWindingNormal(const AZStd::array<AZ::Vector3, 3>& local,
                const AZ::Transform& pose, const AZ::Vector3& scale, AZ::Vector3& normal)
            {
                normal = (local[1] - local[0]).Cross(local[2] - local[0]);
                const auto largest = [](const AZ::Vector3& value)
                { const auto v = value.GetAbs(); return AZStd::max(v.GetX(), AZStd::max(v.GetY(), v.GetZ())); };
                const float localMax = largest(normal);
                if (!normal.IsFinite()) { return false; }
                if (localMax == 0) { return true; } // Exactly degenerate finite local primitive: not a hit.
                normal = pose.TransformVector((normal / localMax) / scale);
                const bool reflected = ((pose.GetUniformScale() < 0) != (scale.GetX() < 0)) != ((scale.GetY() < 0) != (scale.GetZ() < 0));
                if (reflected) { normal = -normal; }
                const float worldMax = largest(normal);
                if (!normal.IsFinite() || worldMax <= 0) { return false; }
                normal = (normal / worldMax).GetNormalized();
                return normal.IsFinite() && normal.GetLengthSq() > 0;
            }
        }

        bool ModelKdTree::ValidateVolumeRequest(const AZ::Aabb& bounds, const AZ::Transform& pose,
            const AZ::Vector3& scale, AZStd::span<ModelVolumeHit> output)
        {
            if (!bounds.IsValid() || !bounds.GetMin().IsFinite() || !bounds.GetMax().IsFinite() || !bounds.GetCenter().IsFinite() ||
                !pose.IsFinite() || pose.GetUniformScale() == 0 || !scale.IsFinite() ||
                scale.GetX() == 0 || scale.GetY() == 0 || scale.GetZ() == 0 ||
                (!output.empty() && output.data() == nullptr) || output.size() > AZStd::numeric_limits<size_t>::max() / sizeof(ModelVolumeHit)) { return false; }
            const auto extents = bounds.GetExtents();
            return extents.IsFinite() && std::isfinite(extents.GetLengthSq()) && extents.GetLengthSq() > 0 &&
                extents.GetX() > 0 && extents.GetY() > 0 && extents.GetZ() > 0;
        }

        ModelRayQueryResult ModelKdTree::CollectVolumeIntersections(AZStd::span<const MeshData> meshes,
            ModelKdTreeNode* root, size_t primitiveCount, const AZ::Aabb& bounds,
            const AZ::Transform& pose, const AZ::Vector3& scale, AZStd::span<ModelVolumeHit> output)
        {
            VolumeIntersectionContext context;
            if (!context.Initialize(bounds, pose, scale)) { return {ModelRayQueryStatus::InvalidInput}; }
            // Validate the complete resident source AND transformed geometry before publishing any output.
            if (!ValidateRayMeshes(meshes)) { return {ModelRayQueryStatus::InvalidGeometry}; }
            for (const auto& mesh : meshes)
            {
                for (size_t i = 0; i < mesh.m_vertexData.size(); i += 3)
                {
                    const AZ::Vector3 p(mesh.m_vertexData[i], mesh.m_vertexData[i + 1], mesh.m_vertexData[i + 2]);
                    if (!pose.TransformPoint(p * scale).IsFinite()) { return {ModelRayQueryStatus::InvalidGeometry}; }
                }
                for (const auto& triangle : mesh.m_triangles)
                {
                    AZ::Vector3 normal;
                    if (!VolumeWindingNormal({RayVertex(mesh.m_vertexData, triangle.index1), RayVertex(mesh.m_vertexData, triangle.index2),
                        RayVertex(mesh.m_vertexData, triangle.index3)}, pose, scale, normal)) { return {ModelRayQueryStatus::InvalidGeometry}; }
                }
            }
            if (primitiveCount == 0) { return {ModelRayQueryStatus::Complete}; }
            AZStd::vector<ModelVolumeHit> scratch(output.size());
            AZStd::vector<AZ::u8> visited(root ? primitiveCount : 0, 0);
            ModelRayQueryResult result{ModelRayQueryStatus::Complete};
            const auto collect = [&](AZ::u32 meshIndex, AZ::u32 primitive) -> bool
            {
                const auto& mesh = meshes[meshIndex];
                const auto& triangle = mesh.m_triangles[primitive];
                const AZStd::array<AZ::Vector3, 3> local{RayVertex(mesh.m_vertexData, triangle.index1),
                    RayVertex(mesh.m_vertexData, triangle.index2), RayVertex(mesh.m_vertexData, triangle.index3)};
                AZ::Vector3 normal;
                if (!VolumeWindingNormal(local, pose, scale, normal)) { return false; }
                if (normal.IsZero()) { return true; }
                const AZStd::array<AZ::Vector3, 3> world{pose.TransformPoint(local[0] * scale),
                    pose.TransformPoint(local[1] * scale), pose.TransformPoint(local[2] * scale)};
                AZ::Vector3 point;
                if (!context.Intersect(local, world, point)) { return true; }
                ModelVolumeHit hit;
                hit.m_worldPosition = point; hit.m_worldNormal = normal;
                hit.m_distance = (point - bounds.GetCenter()).GetLength();
                if (!point.IsFinite() || !std::isfinite(hit.m_distance)) { return false; }
                hit.m_meshIndex = meshIndex; hit.m_primitiveIndex = primitive;
                const auto slot = mesh.m_mesh->GetMaterialSlotId();
                if (slot != ModelMaterialSlot::InvalidStableId) { hit.m_materialSlotId = slot; }
                size_t insertion = 0;
                while (insertion < result.m_hitCount && (scratch[insertion].m_meshIndex < meshIndex ||
                    (scratch[insertion].m_meshIndex == meshIndex && scratch[insertion].m_primitiveIndex < primitive))) { ++insertion; }
                if (result.m_hitCount < scratch.size()) { ++result.m_hitCount; }
                else { ++result.m_discardedHitCount; }
                if (insertion < result.m_hitCount)
                {
                    for (size_t i = result.m_hitCount - 1; i > insertion; --i) { scratch[i] = scratch[i - 1]; }
                    scratch[insertion] = hit;
                }
                return true;
            };
            const auto visit = [&](auto&& self, ModelKdTreeNode* node) -> bool
            {
                if (!node || !node->GetBoundBox().Overlaps(context.m_localBounds)) { return true; }
                if (!node->IsLeaf()) { return self(self, node->GetChild(0)) && self(self, node->GetChild(1)); }
                for (AZ::u32 i = 0; i < node->GetVertexBufferSize(); ++i)
                {
                    const auto mesh = node->GetObjIndex(i), primitive = node->GetPrimitiveIndex(i);
                    const size_t original = meshes[mesh].m_primitiveOffset + primitive;
                    if (visited[original]) { continue; }
                    visited[original] = 1;
                    if (!collect(mesh, primitive)) { return false; }
                }
                return true;
            };
            if (root)
            {
                if (!visit(visit, root)) { return {ModelRayQueryStatus::InvalidGeometry}; }
            }
            else
            {
                for (AZ::u32 mesh = 0; mesh < meshes.size(); ++mesh)
                {
                    for (AZ::u32 primitive = 0; primitive < meshes[mesh].m_triangles.size(); ++primitive)
                    { if (!collect(mesh, primitive)) { return {ModelRayQueryStatus::InvalidGeometry}; } }
                }
            }
            if (result.m_hitCount) { AZStd::copy(scratch.begin(), scratch.begin() + result.m_hitCount, output.begin()); }
            result.m_status = result.m_discardedHitCount ? ModelRayQueryStatus::Truncated : ModelRayQueryStatus::Complete;
            return result;
        }

        ModelRayQueryResult ModelKdTree::VolumeIntersections(const AZ::Aabb& bounds, const AZ::Transform& pose,
            const AZ::Vector3& scale, AZStd::span<ModelVolumeHit> output) const
        {
            if (!ValidateVolumeRequest(bounds, pose, scale, output)) { return {ModelRayQueryStatus::InvalidInput}; }
            if (m_queryStatus != ModelRayQueryStatus::Complete) { return {m_queryStatus}; }
            if (!m_pRootNode) { return {ModelRayQueryStatus::Unavailable}; }
            return CollectVolumeIntersections({m_meshes.data(), m_meshes.size()}, m_pRootNode.get(),
                m_primitiveCount, bounds, pose, scale, output);
        }

        bool ModelKdTree::RayIntersection(
            const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal) const
        {
            AZStd::optional<ModelMaterialSlot::StableId> materialSlotId;
            return RayIntersection(raySrc, rayDir, distanceNormalized, normal, materialSlotId);
        }

        bool ModelKdTree::RayIntersection(
            const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal,
            AZStd::optional<ModelMaterialSlot::StableId>& materialSlotId) const
        {
            materialSlotId.reset();
            float shortestDistanceNormalized = AZStd::numeric_limits<float>::max();
            if (RayIntersectionRecursively(m_pRootNode.get(), raySrc, rayDir, shortestDistanceNormalized, normal, materialSlotId))
            {
                distanceNormalized = shortestDistanceNormalized;
                return true;
            }

            return false;
        }

        bool ModelKdTree::RayIntersectionRecursively(
            ModelKdTreeNode* pNode,
            const AZ::Vector3& raySrc,
            const AZ::Vector3& rayDir,
            float& distanceNormalized,
            AZ::Vector3& normal) const
        {
            AZStd::optional<ModelMaterialSlot::StableId> materialSlotId;
            return RayIntersectionRecursively(pNode, raySrc, rayDir, distanceNormalized, normal, materialSlotId);
        }

        bool ModelKdTree::RayIntersectionRecursively(
            ModelKdTreeNode* pNode,
            const AZ::Vector3& raySrc,
            const AZ::Vector3& rayDir,
            float& distanceNormalized,
            AZ::Vector3& normal,
            AZStd::optional<ModelMaterialSlot::StableId>& materialSlotId) const
        {
            using AZ::Intersect::IntersectRayAABB2;
            using AZ::Intersect::ISECT_RAY_AABB_NONE;

            if (!pNode)
            {
                return false;
            }

            float start, end;
            if (IntersectRayAABB2(raySrc, rayDir.GetReciprocal(), pNode->GetBoundBox(), start, end) == ISECT_RAY_AABB_NONE)
            {
                return false;
            }

            if (start > distanceNormalized)
            {
                return false;
            }

            if (pNode->IsLeaf())
            {
                if (m_meshes.empty())
                {
                    return false;
                }

                const AZ::u32 nVBuffSize = pNode->GetVertexBufferSize();
                if (nVBuffSize == 0)
                {
                    return false;
                }

                const AZ::Vector3 rayEnd = raySrc + rayDir;
                AZ::Intersect::SegmentTriangleHitTester hitTester(raySrc, rayEnd);

                float nearestDistanceNormalized = distanceNormalized;
                for (AZ::u32 i = 0; i < nVBuffSize; ++i)
                {
                    const auto& [first, second, third] = pNode->GetVertexIndex(i);
                    const AZ::u32 nObjIndex = pNode->GetObjIndex(i);

                    const AZStd::span<const float> positionBuffer = m_meshes[nObjIndex].m_vertexData;

                    if (positionBuffer.empty())
                    {
                        continue;
                    }

                    const AZStd::array trianglePoints {
                        AZ::Vector3{positionBuffer[size_t{first} * 3 + 0], positionBuffer[size_t{first} * 3 + 1], positionBuffer[size_t{first} * 3 + 2]},
                        AZ::Vector3{positionBuffer[size_t{second} * 3 + 0], positionBuffer[size_t{second} * 3 + 1], positionBuffer[size_t{second} * 3 + 2]},
                        AZ::Vector3{positionBuffer[size_t{third} * 3 + 0], positionBuffer[size_t{third} * 3 + 1], positionBuffer[size_t{third} * 3 + 2]},
                    };

                    float hitDistanceNormalized;
                    AZ::Vector3 intersectionNormal;
                    if (hitTester.IntersectSegmentTriangleCCW(trianglePoints[0], trianglePoints[1], trianglePoints[2],
                        intersectionNormal, hitDistanceNormalized))
                    {
                        if (nearestDistanceNormalized > hitDistanceNormalized)
                        {
                            normal = intersectionNormal;
                            nearestDistanceNormalized = hitDistanceNormalized;
                            materialSlotId.reset();
                            const auto slotId = m_meshes[nObjIndex].m_mesh->GetMaterialSlotId();
                            if (slotId != ModelMaterialSlot::InvalidStableId) { materialSlotId = slotId; }
                        }
                    }
                }

                if (nearestDistanceNormalized < distanceNormalized)
                {
                    distanceNormalized = nearestDistanceNormalized;
                    return true;
                }

                return false;
            }

            // running both sides to find the closest intersection
            const bool bFoundChild0 = RayIntersectionRecursively(pNode->GetChild(0), raySrc, rayDir, distanceNormalized, normal, materialSlotId);
            const bool bFoundChild1 = RayIntersectionRecursively(pNode->GetChild(1), raySrc, rayDir, distanceNormalized, normal, materialSlotId);

            return bFoundChild0 || bFoundChild1;
        }

        void ModelKdTree::GetPenetratedBoxes(const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, AZStd::vector<AZ::Aabb>& outBoxes)
        {
            GetPenetratedBoxesRecursively(m_pRootNode.get(), raySrc, rayDir, outBoxes);
        }

        void ModelKdTree::GetPenetratedBoxesRecursively(ModelKdTreeNode* pNode, const AZ::Vector3& raySrc, const AZ::Vector3& rayDir, AZStd::vector<AZ::Aabb>& outBoxes)
        {
            AZ::Vector3 ignoreNormal;
            float ignore;
            if (!pNode || (!pNode->GetBoundBox().Contains(raySrc) &&
                (AZ::Intersect::IntersectRayAABB(raySrc, rayDir, rayDir.GetReciprocal(), pNode->GetBoundBox(),
                    ignore, ignore, ignoreNormal)) == AZ::Intersect::ISECT_RAY_AABB_NONE))
            {
                return;
            }

            outBoxes.push_back(pNode->GetBoundBox());

            GetPenetratedBoxesRecursively(pNode->GetChild(0), raySrc, rayDir, outBoxes);
            GetPenetratedBoxesRecursively(pNode->GetChild(1), raySrc, rayDir, outBoxes);
        }
    } // namespace RPI
} // namespace AZ
