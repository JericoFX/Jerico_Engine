/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxCsgCore.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <tuple>
#include <utility>

// manifold (Apache-2.0) - see 3rdParty/FindManifold.cmake
#include <manifold/manifold.h>

namespace JE::BSP
{
    namespace Csg
    {
        void WeldVertices(TriangleMesh& mesh, const double tolerance)
        {
            const size_t vertexCount = mesh.VertexCount();
            const double invTolerance = 1.0 / tolerance;

            // map quantized position -> first (compacted) vertex index found at that position
            std::map<std::tuple<int64_t, int64_t, int64_t>, uint32_t> grid;
            std::vector<uint32_t> remap(vertexCount, 0);
            std::vector<double> weldedPositions;
            weldedPositions.reserve(mesh.m_positions.size());

            const auto quantize = [invTolerance](const double value)
            {
                return static_cast<int64_t>(std::llround(value * invTolerance));
            };

            for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                const double x = mesh.m_positions[vertexIndex * 3 + 0];
                const double y = mesh.m_positions[vertexIndex * 3 + 1];
                const double z = mesh.m_positions[vertexIndex * 3 + 2];

                const auto key = std::make_tuple(quantize(x), quantize(y), quantize(z));
                const auto gridIt = grid.find(key);
                if (gridIt != grid.end())
                {
                    remap[vertexIndex] = gridIt->second;
                }
                else
                {
                    const auto weldedIndex = static_cast<uint32_t>(weldedPositions.size() / 3);
                    weldedPositions.push_back(x);
                    weldedPositions.push_back(y);
                    weldedPositions.push_back(z);
                    grid.emplace(key, weldedIndex);
                    remap[vertexIndex] = weldedIndex;
                }
            }

            // remap the index buffer, dropping triangles that have collapsed to a line/point
            std::vector<uint32_t> weldedIndices;
            weldedIndices.reserve(mesh.m_indices.size());
            std::vector<double> weldedUVs;
            weldedUVs.reserve(mesh.m_cornerUVs.size());
            std::vector<SurfaceKey> weldedSurfaceKeys;
            weldedSurfaceKeys.reserve(mesh.m_surfaceKeys.size());
            for (size_t triangleIndex = 0; triangleIndex < mesh.TriangleCount(); ++triangleIndex)
            {
                const uint32_t i0 = remap[mesh.m_indices[triangleIndex * 3 + 0]];
                const uint32_t i1 = remap[mesh.m_indices[triangleIndex * 3 + 1]];
                const uint32_t i2 = remap[mesh.m_indices[triangleIndex * 3 + 2]];
                if (i0 != i1 && i1 != i2 && i2 != i0)
                {
                    weldedIndices.push_back(i0);
                    weldedIndices.push_back(i1);
                    weldedIndices.push_back(i2);
                    if (!mesh.m_cornerUVs.empty())
                    {
                        for (size_t channel = 0; channel < 6; ++channel)
                        {
                            weldedUVs.push_back(mesh.m_cornerUVs[triangleIndex * 6 + channel]);
                        }
                    }
                    if (!mesh.m_surfaceKeys.empty())
                    {
                        weldedSurfaceKeys.push_back(mesh.m_surfaceKeys[triangleIndex]);
                    }
                }
            }

            mesh.m_positions = std::move(weldedPositions);
            mesh.m_indices = std::move(weldedIndices);
            mesh.m_cornerUVs = std::move(weldedUVs);
            mesh.m_surfaceKeys = std::move(weldedSurfaceKeys);
        }

        namespace
        {
            bool HasAttributes(const TriangleMesh& mesh)
            {
                return !mesh.m_cornerUVs.empty() || !mesh.m_surfaceKeys.empty();
            }

            bool IsValidMesh(const TriangleMesh& mesh)
            {
                // Manifold's internal halfedge and property-vertex indices are signed ints,
                // even when its MeshGL64 transport uses uint64_t indices.
                const size_t indexLimit = static_cast<size_t>(std::numeric_limits<int>::max());
                if (mesh.TriangleCount() == 0 || mesh.m_positions.size() % 3 != 0 ||
                    mesh.m_indices.size() % 3 != 0 || mesh.VertexCount() > indexLimit ||
                    mesh.m_indices.size() > indexLimit)
                {
                    return false;
                }
                const auto finite = [](const double value) { return std::isfinite(value); };
                if (!std::all_of(mesh.m_positions.begin(), mesh.m_positions.end(), finite) ||
                    !std::all_of(mesh.m_indices.begin(), mesh.m_indices.end(), [&mesh](const uint32_t index)
                        { return index < mesh.VertexCount(); }))
                {
                    return false;
                }
                return !HasAttributes(mesh) ||
                    (mesh.m_cornerUVs.size() == 2 * mesh.m_indices.size() &&
                     mesh.m_surfaceKeys.size() == mesh.TriangleCount() &&
                     std::all_of(mesh.m_cornerUVs.begin(), mesh.m_cornerUVs.end(), finite));
            }

            // This remap exists only during one boolean operation. Neither OriginalID
            // nor faceID escapes into the caller-owned SurfaceKey namespace.
            struct SourceMapping
            {
                uint32_t m_originalId{};
                std::vector<SurfaceKey> m_faceKeys;
            };

            // Convert a TriangleMesh into a manifold::Manifold (double precision).
            manifold::Manifold ToManifold(const TriangleMesh& mesh, SourceMapping& source)
            {
                manifold::MeshGL64 meshGl;
                if (!HasAttributes(mesh))
                {
                    meshGl.numProp = 3;
                    meshGl.vertProperties = mesh.m_positions;
                    meshGl.triVerts.assign(mesh.m_indices.begin(), mesh.m_indices.end());
                }
                else
                {
                    meshGl.numProp = 5; // xyz + interpolated UV; discrete IDs never enter properties.
                    meshGl.vertProperties.reserve(mesh.m_indices.size() * 5);
                    meshGl.triVerts.reserve(mesh.m_indices.size());
                    const uint64_t unused = std::numeric_limits<uint64_t>::max();
                    std::vector<uint64_t> firstCorner(mesh.VertexCount(), unused);
                    for (size_t corner = 0; corner < mesh.m_indices.size(); ++corner)
                    {
                        const uint32_t vertex = mesh.m_indices[corner];
                        for (size_t axis = 0; axis < 3; ++axis)
                        {
                            meshGl.vertProperties.push_back(mesh.m_positions[static_cast<size_t>(vertex) * 3 + axis]);
                        }
                        meshGl.vertProperties.push_back(mesh.m_cornerUVs[corner * 2]);
                        meshGl.vertProperties.push_back(mesh.m_cornerUVs[corner * 2 + 1]);
                        meshGl.triVerts.push_back(static_cast<uint64_t>(corner));
                        if (firstCorner[vertex] == unused)
                        {
                            firstCorner[vertex] = static_cast<uint64_t>(corner);
                        }
                        else
                        {
                            // Duplicate property vertices carry the seam. Only their
                            // geometry is joined through the caller's original index.
                            meshGl.mergeFromVert.push_back(static_cast<uint64_t>(corner));
                            meshGl.mergeToVert.push_back(firstCorner[vertex]);
                        }
                    }

                    // A caller surface can contain several UV charts. Distinct
                    // temporary faceIDs protect every input triangle boundary
                    // from Manifold's coplanar simplification, including seams
                    // within one SurfaceKey. Many faceIDs map to the same key.
                    source.m_faceKeys = mesh.m_surfaceKeys;
                    meshGl.faceID.reserve(mesh.TriangleCount());
                    for (size_t triangle = 0; triangle < mesh.TriangleCount(); ++triangle)
                    {
                        meshGl.faceID.push_back(static_cast<uint64_t>(triangle));
                    }
                    meshGl.runOriginalID = {source.m_originalId};
                    meshGl.runIndex = {0, static_cast<uint64_t>(mesh.m_indices.size())};
                }
                // fill the merge vectors in case the input is not perfectly indexed
                meshGl.Merge();
                return manifold::Manifold(meshGl);
            }

            bool ReadSurfaceKeys(
                const manifold::MeshGL64& mesh, const SourceMapping& sourceA, const SourceMapping& sourceB,
                TriangleMesh& result)
            {
                if (mesh.faceID.size() != result.TriangleCount() ||
                    mesh.runIndex.size() != mesh.runOriginalID.size() + 1 || mesh.runIndex.front() != 0 ||
                    mesh.runIndex.back() != mesh.triVerts.size())
                {
                    return false;
                }
                result.m_surfaceKeys.resize(result.TriangleCount());
                for (size_t run = 0; run < mesh.runOriginalID.size(); ++run)
                {
                    const SourceMapping* source = nullptr;
                    if (mesh.runOriginalID[run] == sourceA.m_originalId)
                    {
                        source = &sourceA;
                    }
                    else if (mesh.runOriginalID[run] == sourceB.m_originalId)
                    {
                        source = &sourceB;
                    }
                    if (!source || mesh.runIndex[run] > mesh.runIndex[run + 1] ||
                        mesh.runIndex[run + 1] > mesh.triVerts.size() ||
                        mesh.runIndex[run] % 3 != 0 || mesh.runIndex[run + 1] % 3 != 0)
                    {
                        return false;
                    }
                    for (uint64_t triangle = mesh.runIndex[run] / 3; triangle < mesh.runIndex[run + 1] / 3; ++triangle)
                    {
                        const uint64_t face = mesh.faceID[static_cast<size_t>(triangle)];
                        if (face >= source->m_faceKeys.size())
                        {
                            return false;
                        }
                        result.m_surfaceKeys[static_cast<size_t>(triangle)] = source->m_faceKeys[static_cast<size_t>(face)];
                    }
                }
                return true;
            }
        } // anonymous namespace

        bool MeshBoolean(
            const TriangleMesh& meshA, const TriangleMesh& meshB, const BooleanOperation operation,
            TriangleMesh& result)
        {
            if (!IsValidMesh(meshA) || !IsValidMesh(meshB) || HasAttributes(meshA) != HasAttributes(meshB))
            {
                return false;
            }

            const bool attributed = HasAttributes(meshA);
            SourceMapping sourceA;
            SourceMapping sourceB;
            if (attributed)
            {
                sourceA.m_originalId = manifold::Manifold::ReserveIDs(2);
                sourceB.m_originalId = sourceA.m_originalId + 1;
            }

            const manifold::Manifold manifoldA = ToManifold(meshA, sourceA);
            if (manifoldA.Status() != manifold::Manifold::Error::NoError)
            {
                return false;
            }

            const manifold::Manifold manifoldB = ToManifold(meshB, sourceB);
            if (manifoldB.Status() != manifold::Manifold::Error::NoError)
            {
                return false;
            }

            manifold::OpType opType;
            switch (operation)
            {
            case BooleanOperation::Union:
                opType = manifold::OpType::Add;
                break;
            case BooleanOperation::Intersection:
                opType = manifold::OpType::Intersect;
                break;
            case BooleanOperation::Subtraction:
                opType = manifold::OpType::Subtract;
                break;
            default:
                return false;
            }

            const manifold::Manifold booleanResult = manifoldA.Boolean(manifoldB, opType);
            if (booleanResult.Status() != manifold::Manifold::Error::NoError)
            {
                return false;
            }

            if (booleanResult.IsEmpty())
            {
                result = {};
                return true;
            }

            const manifold::MeshGL64 outputMesh = booleanResult.GetMeshGL64();
            TriangleMesh candidate;

            // copy vertex positions (vertProperties is interleaved with stride numProp,
            // the first three properties are always the position)
            const size_t outputVertexCount = outputMesh.NumVert();
            const auto numProp = static_cast<size_t>(outputMesh.numProp);
            const size_t expectedNumProp = attributed ? size_t{5} : size_t{3};
            if (numProp != expectedNumProp || outputVertexCount > std::numeric_limits<uint32_t>::max())
            {
                return false;
            }
            candidate.m_positions.reserve(outputVertexCount * 3);
            for (size_t vertexIndex = 0; vertexIndex < outputVertexCount; ++vertexIndex)
            {
                candidate.m_positions.push_back(outputMesh.vertProperties[vertexIndex * numProp + 0]);
                candidate.m_positions.push_back(outputMesh.vertProperties[vertexIndex * numProp + 1]);
                candidate.m_positions.push_back(outputMesh.vertProperties[vertexIndex * numProp + 2]);
            }

            // copy triangle indices (narrowing from MeshGL64's uint64_t)
            candidate.m_indices.reserve(outputMesh.triVerts.size());
            candidate.m_cornerUVs.reserve(attributed ? outputMesh.triVerts.size() * 2 : 0);
            for (const uint64_t index : outputMesh.triVerts)
            {
                if (index >= outputVertexCount)
                {
                    return false;
                }
                candidate.m_indices.push_back(static_cast<uint32_t>(index));
                if (attributed)
                {
                    candidate.m_cornerUVs.push_back(outputMesh.vertProperties[static_cast<size_t>(index) * numProp + 3]);
                    candidate.m_cornerUVs.push_back(outputMesh.vertProperties[static_cast<size_t>(index) * numProp + 4]);
                }
            }
            if ((attributed && !ReadSurfaceKeys(outputMesh, sourceA, sourceB, candidate)) || !IsValidMesh(candidate))
            {
                return false;
            }

            // compact any duplicate vertices so the rebuilt mesh is a closed two-manifold
            WeldVertices(candidate, 1e-6);
            if (candidate.TriangleCount() == 0)
            {
                return false;
            }

            result = std::move(candidate);
            return true;
        }
    } // namespace Csg
} // namespace JE::BSP
