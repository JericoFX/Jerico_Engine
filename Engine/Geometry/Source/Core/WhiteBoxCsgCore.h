/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace JE::BSP
{
    namespace Csg
    {
        //! The supported CSG boolean operations.
        enum class BooleanOperation
        {
            Union, //!< A + B
            Subtraction, //!< A - B
            Intersection //!< A & B
        };

        //! Stable caller-owned identity, independent of Manifold's temporary OriginalIDs.
        //! The namespace distinguishes surfaces belonging to different authored meshes.
        struct SurfaceKey
        {
            uint64_t m_namespace{};
            uint64_t m_surface{};

            bool operator==(const SurfaceKey& other) const
            {
                return m_namespace == other.m_namespace && m_surface == other.m_surface;
            }

            bool operator!=(const SurfaceKey& other) const
            {
                return !(*this == other);
            }
        };

        //! An indexed triangle mesh using only std types.
        //! @note This has no engine dependencies so the production CSG core can be
        //! compiled and tested in isolation (see Tests/CSG).
        struct TriangleMesh
        {
            std::vector<double> m_positions; //!< Flattened vertex positions (x0, y0, z0, x1, y1, z1, ...).
            std::vector<uint32_t> m_indices; //!< Triangle list indices (three per triangle, CCW winding).
            std::vector<double> m_cornerUVs; //!< Optional flattened (u,v) per index, preserving UV seams.
            std::vector<SurfaceKey> m_surfaceKeys; //!< Optional caller-owned identity per triangle.

            size_t VertexCount() const
            {
                return m_positions.size() / 3;
            }

            size_t TriangleCount() const
            {
                return m_indices.size() / 3;
            }
        };

        //! Merge vertices that lie within tolerance of each other and remap/compact the
        //! index buffer, dropping any triangles that become degenerate.
        //! @note Use this to build a TriangleMesh from a raw (unindexed) triangle soup
        //! and to clean up the output of a boolean operation before rebuilding a mesh.
        //! @pre Indices are in range and any attribute buffers have complete cardinality.
        //! Corner UVs and surface keys are compacted with their surviving triangles.
        void WeldVertices(TriangleMesh& mesh, double tolerance);

        //! Perform a CSG boolean operation between two non-empty, closed (watertight) triangle meshes
        //! using the Manifold library (https://github.com/elalish/manifold).
        //! @param meshA The first operand (the mesh being modified).
        //! @param meshB The second operand (the 'tool' mesh).
        //! @param operation Which boolean operation to apply.
        //! @param result The resulting triangle mesh; unchanged when returning false.
        //! @return True on success, including a valid empty result (A-A or a disjoint
        //! intersection). False for malformed/non-manifold input, an unsupported operation,
        //! or a non-empty result collapsed by vertex cleanup. Successful empty results clear
        //! all buffers; a failure never publishes a partial result.
        //! Both attribute buffers may be empty for the legacy geometry-only path. Otherwise,
        //! both operands require exactly two finite UV values per index and one surface key
        //! per triangle. UVs interpolate within their source surface; discrete keys do not.
        //! The attributed path retains input triangle boundaries to protect UV charts
        //! within one surface key; it can simplify less than the geometry-only path.
        //! A caller must use distinct namespaces for unrelated authored surface identities.
        bool MeshBoolean(
            const TriangleMesh& meshA, const TriangleMesh& meshB, BooleanOperation operation, TriangleMesh& result);
    } // namespace Csg
} // namespace JE::BSP
