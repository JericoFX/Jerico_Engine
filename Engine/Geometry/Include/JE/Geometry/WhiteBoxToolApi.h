/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Geometry/GeometryExports.h>

#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class Transform;
}

namespace AZ::IO
{
    class GenericStream;
}

namespace JE::Geometry
{
    //! Opaque handle to a WhiteBoxMesh.
    //! @note All Api calls require a white box mesh pointer as the first argument.
    struct WhiteBoxMesh;

    //! Wrapper class to provide type safe index/handle.
    //! @note Make use of the template tag technique (phantom types)
    //! to provide type safe variants of all handles.
    template<typename Tag>
    class GenericHandle
    {
    public:
        GenericHandle() = default;
        explicit GenericHandle(const int index)
            : m_index(index)
        {
        }

        bool IsValid() const
        {
            return m_index >= 0;
        }

        int Index() const
        {
            return m_index;
        }

        bool operator==(const GenericHandle& handle) const
        {
            return m_index == handle.m_index;
        }

        bool operator!=(const GenericHandle& handle) const
        {
            return !operator==(handle);
        }

        bool operator<(const GenericHandle& handle) const
        {
            return m_index < handle.m_index;
        }

    private:
        int m_index = -1;
    };

    namespace Api
    {
        //! Unique identifier for a vertex in the mesh.
        using VertexHandle = GenericHandle<struct VertexHandleTag>;
        //! Unique identifier for a face (triangle) in the mesh.
        using FaceHandle = GenericHandle<struct FaceHandleTag>;
        //! Unique identifier for an edge in the mesh.
        using EdgeHandle = GenericHandle<struct EdgeHandleTag>;
        //! Unique identifier for a halfedge in the mesh.
        using HalfedgeHandle = GenericHandle<struct HalfedgeHandleTag>;

        //! Alias for a collection of vertex handles.
        using VertexHandles = AZStd::vector<VertexHandle>;
        //! Alias for a collection of multiple vertex handle lists.
        using VertexHandlesCollection = AZStd::vector<VertexHandles>;
        //! Alias for a collection of multiple vertex position lists.
        using VertexPositionsCollection = AZStd::vector<AZStd::vector<AZ::Vector3>>;
        //! Alias for a collection of face handles.
        using FaceHandles = AZStd::vector<FaceHandle>;
        //! Alias for a collection of edge handles.
        using EdgeHandles = AZStd::vector<EdgeHandle>;
        //! Alias for a collection of multiple edge handle lists.
        using EdgeHandlesCollection = AZStd::vector<EdgeHandles>;
        //! Alias for a collection of halfedge handles.
        using HalfedgeHandles = AZStd::vector<HalfedgeHandle>;
        //! Alias for a collection of multiple halfedge handle lists.
        using HalfedgeHandlesCollection = AZStd::vector<HalfedgeHandles>;
        //! Alias for position of face vertices.
        using Face = AZStd::array<AZ::Vector3, 3>;
        //! Alias for a collection of faces.
        using Faces = AZStd::vector<Face>;

        //! Underlying representation of the White Box mesh (serialized halfedge data).
        using WhiteBoxMeshStream = AZStd::vector<AZ::u8>;

        //! Represents the vertex handles to be used to form a new face.
        struct FaceVertHandles
        {
            AZStd::array<VertexHandle, 3> m_vertexHandles;
        };

        //! Alias for a collection of multiple face vert handles.
        using FaceVertHandlesList = AZStd::vector<FaceVertHandles>;

        //! Alias for a collection of multiple face vert handle lists.
        using FaceVertHandlesCollection = AZStd::vector<FaceVertHandlesList>;

        //! A type safe way to ask for either the first of second halfedge handle from an edge handle.
        enum class EdgeHalfedge
        {
            First,
            Second
        };

        //! A wrapper to group both 'user' edges and 'mesh' edges.
        //! A 'user' edge is associated with a polygon and the user can interact with it (a logical edge).
        //! A 'mesh' edge is an edge the user cannot currently interact with. It can turn into a user edge
        //! by clicking it in edge activation mode. A 'mesh' edge is an interior edge of a polygon.
        struct EdgeTypes
        {
            EdgeHandles m_user;
            EdgeHandles m_mesh;
        };

        //! A polygon handle is an internal grouping of face handles a user can select and interact with.
        //! @note A polygon handle can consist of 1-N face handles (commonly two).
        struct PolygonHandle
        {
            FaceHandles m_faceHandles;

            bool operator==(const PolygonHandle& handle) const
            {
                return m_faceHandles == handle.m_faceHandles;
            }

            bool operator!=(const PolygonHandle& handle) const
            {
                return !(operator==(handle));
            }
        };

        using PolygonHandles = AZStd::vector<PolygonHandle>; //!< Alias for a collection of polygon handles.

        //! Stores the before and after polygon handles potentially created during a polygon append (impression).
        struct RestoredPolygonHandlePair
        {
            PolygonHandle m_before;
            PolygonHandle m_after;
        };

        //!< Alias for a collection of restored polygon handle pairs.
        using RestoredPolygonHandlePairs = AZStd::vector<RestoredPolygonHandlePair>;

        //! Stores all relevant created/modified polygon handles from an append operation.
        struct AppendedPolygonHandles
        {
            PolygonHandle m_appendedPolygonHandle; //!< The primary new polygon handle that was created
                                                   //!< (usually the one being interacted with).
            RestoredPolygonHandlePairs m_restoredPolygonHandles; //!< A collection of the connected polygon handles to
                                                                 //!< the primary polygon handle that may have been
                                                                 //!< deleted and then re-added.
        };

        //! Custom deleter for WhiteBoxMesh opaque pointer.
        class WhiteBoxMeshDeleter
        {
        public:
            void operator()(WhiteBoxMesh* whiteBox)
            {
                DestroyWhiteBoxMesh(whiteBox);
            }

        private:
            static JE_GEOMETRY_AUTHORING_API void DestroyWhiteBoxMesh(WhiteBoxMesh* whiteBox);
        };

        //! Alias for an owning pointer to the opaque WhiteBoxMesh type.
        using WhiteBoxMeshPtr = AZStd::unique_ptr<WhiteBoxMesh, WhiteBoxMeshDeleter>;

        //! Return an owning pointer to a newly created WhiteBoxMesh.
        //! @note `delete` does not need to be called as the memory will automatically be reclaimed when
        //! the WhiteBoxMeshPtr goes out of scope (or reset() is explicitly called).
        JE_GEOMETRY_AUTHORING_API WhiteBoxMeshPtr CreateWhiteBoxMesh();

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Handles

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Mesh - handle operations/queries that relate to the entire mesh.
        //
        //! Return all vertex handles in the mesh.
        JE_GEOMETRY_AUTHORING_API VertexHandles MeshVertexHandles(const WhiteBoxMesh& whiteBox);

        //! Return all edge handles in the mesh.
        //! @note this includes interior edges of a polygon.
        JE_GEOMETRY_AUTHORING_API EdgeHandles MeshEdgeHandles(const WhiteBoxMesh& whiteBox);
        //! Return edge handles decomposed into 'user' edges and 'mesh' edges.
        //! @note: 'user' edges are those associated with a polygon and that can
        //! be interacted with, 'mesh' edges are interior edges. It is possible for
        //! edges to transition between 'user' and 'mesh' with show/hide operations.
        JE_GEOMETRY_AUTHORING_API EdgeTypes MeshUserEdgeHandles(const WhiteBoxMesh& whiteBox);

        //! Return all face handles in the mesh.
        JE_GEOMETRY_AUTHORING_API FaceHandles MeshFaceHandles(const WhiteBoxMesh& whiteBox);

        //! Return all polygon handles in the mesh.
        JE_GEOMETRY_AUTHORING_API PolygonHandles MeshPolygonHandles(const WhiteBoxMesh& whiteBox);

        //! Return all unique edges bordering the polygons in the mesh.
        JE_GEOMETRY_AUTHORING_API EdgeHandles MeshPolygonEdgeHandles(const WhiteBoxMesh& whiteBox);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Face - handle operations/queries that relate to a single face.
        //
        //! Return all halfedge handles corresponding to a given face.
        //! @note In all cases the number of halfedge handles returned for a face should be three.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandles FaceHalfedgeHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return all edge handles that belong to a face.
        JE_GEOMETRY_AUTHORING_API EdgeHandles FaceEdgeHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return all vertex handles corresponding to a given face.
        //! @note In all cases the number of vertex handles returned for a face should be three.
        JE_GEOMETRY_AUTHORING_API VertexHandles FaceVertexHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return the polygon handle containing the given face.
        //! @note Calling this function from outside of the Api should always return a non-empty
        //! PolygonHandle however internally there may be occasions were certain invariants are
        //! temporarily broken and a FaceHandle may not yet have been added to a PolygonHandle.
        //! e.g. While splitting an Edge.
        JE_GEOMETRY_AUTHORING_API PolygonHandle FacePolygonHandle(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Vertex - handle operations/queries that relate to a vertex
        //
        //! Return all outgoing halfedge handles from a given vertex.
        //! @note The outgoing halfedges may span multiple faces.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandles VertexOutgoingHalfedgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return all incoming halfedge handles to a given vertex.
        //! @note The incoming halfedges may span multiple faces.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandles VertexIncomingHalfedgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return all halfedge handles (incoming and outgoing) for a given vertex.
        //! @note The halfedges may span multiple faces.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandles VertexHalfedgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return all edge handles for a given vertex.
        //! @note The edge handles returned will include both 'user' and 'mesh' edges.
        JE_GEOMETRY_AUTHORING_API EdgeHandles VertexEdgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return user edge handles for a given vertex.
        //! @note The edge handles returned will only include 'user' edges.
        JE_GEOMETRY_AUTHORING_API EdgeHandles VertexUserEdgeHandles(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Side - handle operations/queries that relate to a side.
        // @note A 'side' is defined as a collection of faces that all share the same normal and are connected/adjacent.
        //
        //! Return all vertex handles that correspond with a given side.
        //! @note All vertex handles that lie on the same plane as the side.
        JE_GEOMETRY_AUTHORING_API VertexHandles SideVertexHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return all faces that correspond to a given side.
        JE_GEOMETRY_AUTHORING_API FaceHandles SideFaceHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return only the vertices that bound the edge of the side.
        //! @note No internal vertices will be returned.
        //! @note A vector of vectors is returned as there may be multiple vertex loops for a given side.
        JE_GEOMETRY_AUTHORING_API VertexHandlesCollection SideBorderVertexHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return only the halfedges that bound the edge of the side.
        //! @note No internal halfedges will be returned (that is any halfedges
        //! that have an opposite face handle with the same normal).
        //! @note A vector of vectors is returned as there may be multiple halfedge loops for a given side.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandlesCollection SideBorderHalfedgeHandles(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Halfedge - handle operations/queries that relate to a single halfedge.
        //
        //! Return the face handle that this halfedge belongs to.
        JE_GEOMETRY_AUTHORING_API FaceHandle HalfedgeFaceHandle(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the opposite halfedge handle of the passed in halfedge handle.
        //! @note Is it possible the returned halfedge handle may be invalid if the
        //! opposite halfedge does not belong to a face (this may happen with a 2d
        //! mesh when looking at a halfedge that bounds the mesh).
        JE_GEOMETRY_AUTHORING_API HalfedgeHandle HalfedgeOppositeHalfedgeHandle(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the opposite face handle of the passed in halfedge handle.
        //! @note It is possible the returned face handle may be invalid if the
        //! opposite halfedge does not belong to a face (this may happen with a 2d
        //! mesh when looking at a halfedge that bounds the mesh).
        JE_GEOMETRY_AUTHORING_API FaceHandle HalfedgeOppositeFaceHandle(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the vertex handle when following the direction/orientation of
        //! the halfedge handle and looking at what vertex it is pointing to.
        JE_GEOMETRY_AUTHORING_API VertexHandle HalfedgeVertexHandleAtTip(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the vertex handle when following the direction/orientation of
        //! the halfedge handle and looking at what vertex it is coming from.
        JE_GEOMETRY_AUTHORING_API VertexHandle HalfedgeVertexHandleAtTail(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the edge handle the halfedge handle is related to.
        JE_GEOMETRY_AUTHORING_API EdgeHandle HalfedgeEdgeHandle(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the next halfedge handle the halfedge is connected to.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandle HalfedgeHandleNext(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the previous halfedge handle the halfedge is connected to.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandle HalfedgeHandlePrevious(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Edge - handle operations/queries that related to a single edge.
        //
        //! Return the two adjacent faces for a given edge.
        //! If the edge is on a border, only one face handle is returned.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<FaceHandle> EdgeFaceHandles(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return the first or second halfedge handle of a given edge handle.
        //! @param edgeHalfedge Used to ask for either the first or second halfedge of an edge.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandle EdgeHalfedgeHandle(
            const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, EdgeHalfedge edgeHalfedge);

        //! Return either one or two halfedge handles for a given edge handle (depending on if
        //! the edge is a boundary or not).
        JE_GEOMETRY_AUTHORING_API HalfedgeHandles EdgeHalfedgeHandles(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Polygon - handle operations/queries that relate to polygons.
        //
        //! Return all vertex handles that are associated with the polygon.
        JE_GEOMETRY_AUTHORING_API VertexHandles PolygonVertexHandles(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all vertex handles that bound the polygon (vertices at the edge).
        //! @note These will be ordered CCW.
        //! @note A vector of vectors is returned as there may be multiple vertex loops for a given polygon.
        JE_GEOMETRY_AUTHORING_API VertexHandlesCollection PolygonBorderVertexHandles(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all vertex handles that bound the polygon (vertices at the edge).
        //! @note These will be ordered CCW.
        //! @note Return the vector of vectors as a flattened list. May contain multiple loops that are not connected.
        JE_GEOMETRY_AUTHORING_API VertexHandles PolygonBorderVertexHandlesFlattened(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all halfedge handles associated with the given polygon (this includes interior edges).
        JE_GEOMETRY_AUTHORING_API HalfedgeHandles PolygonHalfedgeHandles(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all halfedge handles that border the polygon (synonymous with 'logical' edges).
        //! @note This excludes interior halfedges.
        //! @note A vector of vectors is returned as there may be multiple vertex loops for a given polygon.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandlesCollection PolygonBorderHalfedgeHandles(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all halfedge handles that border the polygon (synonymous with 'logical' edges).
        //! @note This excludes interior halfedges.
        //! @note Return the vector of vectors as a flattened list. May contain multiple loops that are not connected.
        JE_GEOMETRY_AUTHORING_API HalfedgeHandles PolygonBorderHalfedgeHandlesFlattened(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all edge handles that border the polygon (synonymous with 'logical' edges).
        //! @note This excludes interior edges.
        //! @note A vector of vectors is returned as there may be multiple edge loops for a given polygon.
        JE_GEOMETRY_AUTHORING_API EdgeHandlesCollection PolygonBorderEdgeHandles(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all edge handles that border the polygon.
        //! @note This excludes interior edges.
        //! @note Return the vector of vectors as a flattened list. May contain multiple loops that are not connected.
        JE_GEOMETRY_AUTHORING_API EdgeHandles PolygonBorderEdgeHandlesFlattened(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Values

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Mesh - value operations/queries that relate to the entire mesh.
        //
        //! Return how many faces the white box mesh has.
        JE_GEOMETRY_AUTHORING_API AZ::u64 MeshFaceCount(const WhiteBoxMesh& whiteBox);

        //! Return how many vertices the white box mesh has.
        JE_GEOMETRY_AUTHORING_API AZ::u64 MeshVertexCount(const WhiteBoxMesh& whiteBox);

        //! Return how many halfedges the white box mesh has.
        JE_GEOMETRY_AUTHORING_API AZ::u64 MeshHalfedgeCount(const WhiteBoxMesh& whiteBox);

        //! Return all face value types in the mesh.
        //! @note Face is a collection for three vertices/positions.
        JE_GEOMETRY_AUTHORING_API Faces MeshFaces(const WhiteBoxMesh& whiteBox);

        //! Return the positions of all vertices in the mesh.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<AZ::Vector3> MeshVertexPositions(const WhiteBoxMesh& whiteBox);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Vertex - value operations/queries that relate to vertices.
        //
        //! Return the vertex position of the requested vertex handle.
        //! @note A valid vertex handle must be provided. This function will fail if a vertex
        //! is passed that does not exist in the mesh.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 VertexPosition(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return all vertex positions corresponding to the collection of vertex handles passed in.
        //! @note All vertex handles in the collection must exist in the mesh. This function will fail
        //! if a vertex is passed that does not exist in the mesh.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<AZ::Vector3> VertexPositions(const WhiteBoxMesh& whiteBox, const VertexHandles& vertexHandles);

        //! Return if a vertex is hidden or not.
        JE_GEOMETRY_AUTHORING_API bool VertexIsHidden(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return if a vertex is isolated or not.
        //! @note A vertex is isolated if it has no connecting 'user' edges.
        JE_GEOMETRY_AUTHORING_API bool VertexIsIsolated(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return user edge vectors for a given vertex.
        //! @note The edge vectors returned will only include 'user' edges and will not be normalized.
        //! @note Any invalid (zero) edge vectors will be filtered out and not returned.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<AZ::Vector3> VertexUserEdgeVectors(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Return user edge axes for a given vertex.
        //! @note The edge axes returned will only include 'user' edges and will be normalized.
        //! @note Any invalid (zero) edge axes will be filtered out and not returned.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<AZ::Vector3> VertexUserEdgeAxes(const WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Face - value operations/queries that relate to faces.
        //
        //! Return the normal associated with the given face handle.
        //! @note The face handle passed in must be known to exist in the mesh otherwise the call will fail.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 FaceNormal(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);
        //! Opaque caller-owned UUID128 surface lineage. Its complete bytes map to the two
        //! JE CSG key words; these are not an allocator namespace or OpenMesh face index.
        //! Snapshot clones preserve IDs. New authored polygons receive fresh IDs.
        JE_GEOMETRY_AUTHORING_API AZ::Uuid FaceSurfaceId(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);
        JE_GEOMETRY_AUTHORING_API void SetFaceSurfaceId(WhiteBoxMesh& whiteBox, FaceHandle faceHandle, const AZ::Uuid& surfaceId);

        //! Return all vertex positions for a given face handle.
        //! @note The face handle passed in must be known to exist in the mesh otherwise the call will fail.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<AZ::Vector3> FaceVertexPositions(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Return positions of each face in the collection.
        //! Vertex positions for each face will be returned (not unique vertex positions for all faces).
        //! @note FacesPositions effectively returns a triangle list of vertex positions.
        //! @note The face handles passed in must be known to exist in the mesh otherwise the call will fail.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<AZ::Vector3> FacesPositions(const WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Halfedge - value operations/queries that relate to halfedges.
        //
        //! Return the texture coordinate (uv) associated with the given halfedge handle.
        JE_GEOMETRY_AUTHORING_API AZ::Vector2 HalfedgeUV(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Set an authored corner UV; shared positions may have different UVs.
        JE_GEOMETRY_AUTHORING_API void SetHalfedgeUV(WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle, const AZ::Vector2& uv);

        //! Return the vertex position at the tip of the half edge.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 HalfedgeVertexPositionAtTip(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return the vertex position at the tail of the half edge.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 HalfedgeVertexPositionAtTail(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);

        //! Return if the halfedge is at a boundary (i.e. the halfedge only has one face associated with it).
        JE_GEOMETRY_AUTHORING_API bool HalfedgeIsBoundary(const WhiteBoxMesh& whiteBox, HalfedgeHandle halfedgeHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Edge - value operations/queries that relate to edges.
        //
        //! Return the vertex positions at the tip of each edge.
        JE_GEOMETRY_AUTHORING_API AZStd::array<AZ::Vector3, 2> EdgeVertexPositions(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return the vertex handles at each end of the edge.
        JE_GEOMETRY_AUTHORING_API AZStd::array<VertexHandle, 2> EdgeVertexHandles(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return the normalized axis of the edge.
        //! @note Internally uses the 'first' halfedge, direction will be from tail to tip.
        //! @note It is possible EdgeAxis will return a zero vector if the two edge vertices are at the same position.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 EdgeAxis(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return the edge for the corresponding edge handle.
        //! @note The edge vector will not be normalized. It will be the length of the distance between the two
        //! vertices.
        //! @note It is possible EdgeVector will return a zero vector if the two edge vertices are at the same position.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 EdgeVector(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return if the edge is at a boundary (i.e. the edge only has one halfedge associated with it).
        JE_GEOMETRY_AUTHORING_API bool EdgeIsBoundary(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return all connected edges that have been merged through vertex hiding.
        JE_GEOMETRY_AUTHORING_API EdgeHandles EdgeGrouping(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Return if an edge is hidden or not.
        JE_GEOMETRY_AUTHORING_API bool EdgeIsHidden(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Polygon - value operations/queries that relate to polygons.
        //
        //! Return the average normal of a polygon composed of several faces (average the normal across the faces).
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 PolygonNormal(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Will return a transform aligned to the orientation of the polygon.
        //! @param pivot Most often will be the midpoint of the polygon but can be customized for other
        //! use cases such as non-uniform scaling.
        JE_GEOMETRY_AUTHORING_API AZ::Transform PolygonSpace(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const AZ::Vector3& pivot);

        //! Will return a transform aligned to the direction of the edge.
        //! @param pivot Most often will be the midpoint of the edge but can be customized for other
        //! use cases such as non-uniform scaling.
        JE_GEOMETRY_AUTHORING_API AZ::Transform EdgeSpace(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, const AZ::Vector3& pivot);

        //! Return all vertex positions associated with the polygon handle.
        //! @note This is a helper which internally calls PolygonVertexHandles and then does a
        //! transformation from vertex handles to vertex positions.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<AZ::Vector3> PolygonVertexPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return all border vertex positions associated with the polygon handle.
        //! @note This is a helper which internally calls PolygonBorderVertexHandles and then does a
        //! transformation from vertex handles to vertex positions.
        //! @note A vector of vectors is returned as there may be multiple vertex loops for a given polygon.
        JE_GEOMETRY_AUTHORING_API VertexPositionsCollection PolygonBorderVertexPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Return positions of each face in the polygon.
        //! @note This call is a convenience wrapper for FacesPositions.
        //! @note PolygonFacesPositions effectively returns a triangle list of vertex positions.
        JE_GEOMETRY_AUTHORING_API AZStd::vector<AZ::Vector3> PolygonFacesPositions(
            const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Defaults

        //! Accepts a created but uninitialized white box and generates a cube for editing.
        //! @note A polygon will be created for each side of the cube.
        //! @return A vector of the polygon handles that were added.
        JE_GEOMETRY_AUTHORING_API PolygonHandles InitializeAsUnitCube(WhiteBoxMesh& whiteBox);
        //! Accepts a created but uninitialized whiteBox and generates a quad for editing.
        //! @note A polygon will be created for the quad (it is currently only 1 sided, CCW winding order).
        //! @return The polygon handle that was added.
        JE_GEOMETRY_AUTHORING_API PolygonHandle InitializeAsUnitQuad(WhiteBoxMesh& whiteBox);
        //! Accepts a created but uninitialized whiteBox and generates a triangle for editing.
        //! @note A polygon will be created for the triangle (it is currently only 1 sided, CCW winding order).
        //! @return The polygon handle that was added.
        JE_GEOMETRY_AUTHORING_API PolygonHandle InitializeAsUnitTriangle(WhiteBoxMesh& whiteBox);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Mutations
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        //! Remove faces from the mesh.
        //! @note Collect all face handles to remove and call once per operation --
        //! do not call in a loop, as handles may be invalidated during garbage_collect.
        JE_GEOMETRY_AUTHORING_API void RemoveFaces(WhiteBoxMesh& whiteBox, const FaceHandles& faceHandles);
        //! Set the position of the provided vertex handle to the new vertex position.
        JE_GEOMETRY_AUTHORING_API void SetVertexPosition(WhiteBoxMesh& whiteBox, VertexHandle vertexHandle, const AZ::Vector3& position);

        //! Set the position of the provided vertex handle to the new vertex position
        //! and immediately recalculate the UVs for the mesh.
        //! @note Internally calls SetVertexPosition and CalculatePlanarUVs.
        JE_GEOMETRY_AUTHORING_API void SetVertexPositionAndUpdateUVs(
            WhiteBoxMesh& whiteBox, VertexHandle vertexHandle, const AZ::Vector3& position);

        //! Add a vertex to the mesh.
        //! @return A handle to the newly added vertex.
        //! @note This is a low level call that should not generally be used as it usually
        //! requires calling AddFace immediately after adding three consecutive vertex handles.
        JE_GEOMETRY_AUTHORING_API VertexHandle AddVertex(WhiteBoxMesh& whiteBox, const AZ::Vector3& vertex);

        //! Add a face to the mesh.
        //! @return A handle to the newly added face.
        //! @note This is a low level call that should not generally be used as internally
        //! a Polygon should be created when one or more faces are added - prefer using the Append<> operations.
        JE_GEOMETRY_AUTHORING_API FaceHandle AddFace(WhiteBoxMesh& whiteBox, VertexHandle v0, VertexHandle v1, VertexHandle v2);

        //! Create a Polygon from a list of FaceVertHandles.
        //! Each FaceVertHandle represents an individual Face in the Polygon (3 vertex handles forming a Face/Triangle).
        //! @return The newly added PolygonHandle.
        //! @note faceVertHandles should have at least one faceVertHandle.
        JE_GEOMETRY_AUTHORING_API PolygonHandle AddPolygon(WhiteBoxMesh& whiteBox, const FaceVertHandlesList& faceVertHandles);

        //! Rebuild/restore triangles with inherited surface lineage without minting a new ID.
        JE_GEOMETRY_AUTHORING_API PolygonHandle AddPolygon(WhiteBoxMesh& whiteBox, const FaceVertHandlesList& faceVertHandles, const AZ::Uuid& surfaceId);

        //! Create a three sided (triangle) polygon from three VertexHandles.
        //! Each VertexHandle represents an individual Vertex in the Polygon (3 vertex handles forming a Face/Triangle).
        //! @return The newly added PolygonHandle.
        JE_GEOMETRY_AUTHORING_API PolygonHandle AddTriPolygon(WhiteBoxMesh& whiteBox, VertexHandle vh0, VertexHandle vh1, VertexHandle vh2);

        //! Create four sided quad polygon (two triangles sharing an edge) from four VertexHandles.
        //! Each VertexHandle represents an individual Vertex in the Polygon (4 vertex handles forming
        //! two Faces in one Quad).
        //! @return The newly added PolygonHandle.
        JE_GEOMETRY_AUTHORING_API PolygonHandle AddQuadPolygon(
            WhiteBoxMesh& whiteBox, VertexHandle vh0, VertexHandle vh1, VertexHandle vh2, VertexHandle vh3);

        //! Extrude a single polygon in the mesh.
        //! A lateral face will be created for each edge of the polygon that is extruded.
        //! TranslatePolygonAppend can be thought of as an extrusion along the normal of the polygon.
        //! @note distance must not be zero.
        JE_GEOMETRY_AUTHORING_API PolygonHandle TranslatePolygonAppend(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, float distance);

        //! Extrude a single polygon in the mesh.
        //! A lateral face will be created for each edge of the polygon that is extruded.
        //! TranslatePolygonAppend can be thought of as an extrusion along the normal of the polygon.
        //! @note distance must not be zero.
        //! @note TranslatePolygonAppendAdvanced returns additional information including connected/neighboring
        //! polygons that were removed and then re-added if shared vertices were changed. This information is important
        //! for types that hold references to polygon handles that may change (e.g. Modifiers).
        JE_GEOMETRY_AUTHORING_API AppendedPolygonHandles TranslatePolygonAppendAdvanced(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, float distance);

        //! Translate an edge in the mesh.
        //! Moves an edge specified by the displacement.
        JE_GEOMETRY_AUTHORING_API void TranslateEdge(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, const AZ::Vector3& displacement);

        //! Translate an edge while at the same time duplicating it to create a total of three new polygons.
        //! One new polygon will be created aligned to the edge that was translated (most likely a quad)
        //! and two polygons at the top and bottom of the new edge will be inserted (most likely triangles).
        //! @note displacement must not be zero.
        JE_GEOMETRY_AUTHORING_API EdgeHandle TranslateEdgeAppend(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, const AZ::Vector3& displacement);

        //! Duplicate and then scale a polygon in the mesh.
        //! @note A new polygon will be inserted into the mesh but will be scaled uniformly along
        //! the tangent of the face (orthogonal to the normal of the polygon/face).
        JE_GEOMETRY_AUTHORING_API PolygonHandle ScalePolygonAppendRelative(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, float scale);

        //! Translate a polygon along its normal axis.
        //! @note distance can be positive or negative to move forward or backward respectively.
        JE_GEOMETRY_AUTHORING_API void TranslatePolygon(WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, float distance);

        //! Scale a polygon in-place.
        //! @note Scale will be applied relative to the existing scale of the polygon.
        //! @note All edges/vertices of a polygon will be scaled uniformly along the
        //! tangent of the face about the pivot.
        JE_GEOMETRY_AUTHORING_API void ScalePolygonRelative(
            WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle, const AZ::Vector3& pivot, float scaleDelta);

        //! The supported CSG (constructive solid geometry) boolean operations.
        enum class BooleanOperation
        {
            Union, //!< The combined volume of both meshes (A + B).
            Subtraction, //!< The volume of the first mesh with the second mesh cut away (A - B).
            Intersection //!< Only the volume shared by both meshes (A & B).
        };

        //! Perform a CSG boolean operation between two white box meshes.
        //! The result replaces the contents of whiteBox; the operand mesh is unmodified.
        //! @param whiteBox The mesh to apply the boolean operation to (modified in place).
        //! @param operand The second operand (the 'tool' mesh), e.g. the volume to cut away in a subtraction.
        //! @param operandTransform Transform positioning operand in the local space of whiteBox
        //! (for two entities, worldFromLocalA.GetInverse() * worldFromLocalB).
        //! @param operation Which boolean operation to apply.
        //! @return True if a non-empty result was applied. False for invalid input or an
        //! unsupported operation, or when a valid empty result cannot yet be applied by the
        //! editor's render/collider consumers. whiteBox is unmodified when returning false.
        //! @note Both meshes must be closed (watertight). Coplanar faces in the result are merged
        //! into white box polygons, but UVs are recalculated so custom UVs are not preserved.
        JE_GEOMETRY_AUTHORING_API bool ApplyMeshBoolean(
            WhiteBoxMesh& whiteBox, const WhiteBoxMesh& operand, const AZ::Transform& operandTransform,
            BooleanOperation operation);

        //! Recalculate all normals of each face in the mesh.
        JE_GEOMETRY_AUTHORING_API void CalculateNormals(WhiteBoxMesh& whiteBox);

        //! Zero/clear all uvs.
        JE_GEOMETRY_AUTHORING_API void ZeroUVs(WhiteBoxMesh& whiteBox);

        //! Calculate planar uvs for all halfedges (these are where the uvs are stored) in the mesh.
        //! @note This will produce a tiling effect across each side of the mesh.
        JE_GEOMETRY_AUTHORING_API void CalculatePlanarUVs(WhiteBoxMesh& whiteBox);

        //! Hide an edge to merge two polygons that share the same edge.
        //! Return the handle to the merged polygon.
        JE_GEOMETRY_AUTHORING_API PolygonHandle HideEdge(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Flip an edge within a quad to subdivide the quad across another diagonal.
        //! Return whether the edge was able to be flipped (operation may fail).
        JE_GEOMETRY_AUTHORING_API bool FlipEdge(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Split an edge and subdivide any connected faces in two, the position
        //! is where to insert the new vertex.
        //! Return the vertex handle where the edge was split.
        //! @note If the edge is at a boundary only one face will be split.
        JE_GEOMETRY_AUTHORING_API VertexHandle SplitEdge(WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, const AZ::Vector3& position);

        //! Split a face by subdividing it into three new faces, the position
        //! is where to insert the new vertex.
        //! Return the vertex handle where the face was split.
        JE_GEOMETRY_AUTHORING_API VertexHandle SplitFace(WhiteBoxMesh& whiteBox, FaceHandle faceHandle, const AZ::Vector3& position);

        //! Hide a vertex to merge the edges that share the same vertex.
        JE_GEOMETRY_AUTHORING_API void HideVertex(WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Move an edge from a 'mesh' edge to a 'user' edge (a 'logical' edge).
        //! As a new polygon may not be immediately created, it is the caller's responsibility to
        //! store the edges that might form new polygons. When a new polygon is formed, the caller
        //! must remove the edges from the intermediate restoringEdgeHandles parameter.
        //! @return If successful, return the two newly formed polygons, otherwise an empty optional.
        //! @param restoringEdgeHandles Temporary buffer for edges that are being restored,
        //! must be maintained by the caller.
        JE_GEOMETRY_AUTHORING_API AZStd::optional<AZStd::array<PolygonHandle, 2>> RestoreEdge(
            WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle, EdgeHandles& restoringEdgeHandles);

        //! Restore a vertex from its 'hidden' state and split any connected edges.
        JE_GEOMETRY_AUTHORING_API void RestoreVertex(WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Attempt to restore a vertex from its 'hidden' state and split any connected edges.
        //! @note TryRestoreVertex will first check if there are any connected edges. If there are none, then the
        //! vertex will not be restored as it has no valid edges to connect with so cannot be interacted with.
        //! @return Returns if the vertex was restored or not.
        JE_GEOMETRY_AUTHORING_API bool TryRestoreVertex(WhiteBoxMesh& whiteBox, VertexHandle vertexHandle);

        //! Removes all mesh data by clearing the mesh.
        JE_GEOMETRY_AUTHORING_API void Clear(WhiteBoxMesh& whiteBox);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Helpers

        //! Calculate the midpoint of all vertices composing a polygon.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 PolygonMidpoint(const WhiteBoxMesh& whiteBox, const PolygonHandle& polygonHandle);

        //! Calculate the midpoint of the two vertices composing an edge.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 EdgeMidpoint(const WhiteBoxMesh& whiteBox, EdgeHandle edgeHandle);

        //! Calculate the midpoint of a face (three vertices).
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 FaceMidpoint(const WhiteBoxMesh& whiteBox, FaceHandle faceHandle);

        //! Calculate the midpoint of an arbitrary collection of vertices.
        JE_GEOMETRY_AUTHORING_API AZ::Vector3 VerticesMidpoint(const WhiteBoxMesh& whiteBox, const VertexHandles& vertexHandles);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Serialization

        //! The result of attempting to deserialize a white box mesh from a white box mesh stream.
        enum class ReadResult
        {
            Full, //!< The white box mesh stream was full and was read into white box mesh (it is now initialized).
            Empty, //!< The white box mesh stream was empty so no white box mesh was loaded.
            Error //!< An error occurred while trying to deserialize white box mesh stream.
        };

        //! Take an input stream of bytes and create a white box mesh from the deserialized data.
        //! @return Will return ReadResult::Full if the white box mesh stream was filled with data and
        //! the white box mesh was initialized, ReadResult::Empty if white box mesh stream did not contain
        //! any data (the existing white box mesh is preserved) or ReadResult::Error if any error was encountered
        //! during deserialization.
        //! @note A white box mesh must have been created first.
        JE_GEOMETRY_AUTHORING_API ReadResult ReadMesh(WhiteBoxMesh& whiteBox, const WhiteBoxMeshStream& input);

        //! Take an input stream and create a white box mesh from the deserialized data.
        //! @return Will return ReadResult::Full if the white box mesh stream was filled with data and
        //! the white box mesh was initialized, ReadResult::Empty if white box mesh stream did not contain
        //! any data (the existing white box mesh is preserved) or ReadResult::Error if any error was encountered
        //! during deserialization.
        //! @note The input stream must not skip white space characters (std::noskipws must be set on the stream).
        //! Old .om files without surface properties derive stable UUIDs from the original
        //! byte content and polygon ordinal; the next WriteMesh persists those full IDs.
        //! Malformed/partial surface properties return Error without replacing the mesh.
        JE_GEOMETRY_AUTHORING_API ReadResult ReadMesh(WhiteBoxMesh& whiteBox, std::istream& input);

        //! Take a white box mesh and write it out to a stream of bytes.
        //! @return Will return false if any error was encountered during serialization, true otherwise.
        JE_GEOMETRY_AUTHORING_API bool WriteMesh(const WhiteBoxMesh& whiteBox, WhiteBoxMeshStream& output);

        //! Clones the white box mesh object into a new mesh.
        //! @return Will return null if any error was encountered during serialization, otherwise the cloned mesh.
        JE_GEOMETRY_AUTHORING_API WhiteBoxMeshPtr CloneMesh(const WhiteBoxMesh& whiteBox);

        //! Commit a fully constructed candidate without serialization or regenerating attributes.
        JE_GEOMETRY_AUTHORING_API void ReplaceMesh(WhiteBoxMesh& destination, WhiteBoxMeshPtr candidate);

        //! Writes the white box mesh to an obj file at the specified path.
        //! @return Will return false if any error was encountered during serialization, true otherwise.
        JE_GEOMETRY_AUTHORING_API bool SaveToObj(const WhiteBoxMesh& whiteBox, const AZStd::string& filePath);

        //! Writes the white box mesh to the io stream.
        //! @return Will return false if any error was encountered during serialization, true otherwise.
        JE_GEOMETRY_AUTHORING_API bool SaveToWbm(const WhiteBoxMesh& whiteBox, AZ::IO::GenericStream& stream);

        //! Writes the white box mesh to a wbm file at the specified path.
        //! @return Will return false if any error was encountered during serialization, true otherwise.
        JE_GEOMETRY_AUTHORING_API bool SaveToWbm(const WhiteBoxMesh& whiteBox, const AZStd::string& filePath);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Logging

        JE_GEOMETRY_AUTHORING_API AZStd::string ToString(VertexHandle vertexHandle);
        JE_GEOMETRY_AUTHORING_API AZStd::string ToString(FaceHandle faceHandle);
        JE_GEOMETRY_AUTHORING_API AZStd::string ToString(EdgeHandle edgeHandle);
        JE_GEOMETRY_AUTHORING_API AZStd::string ToString(HalfedgeHandle halfedgeHandle);
        JE_GEOMETRY_AUTHORING_API AZStd::string ToString(const PolygonHandle& polygonHandle);
        JE_GEOMETRY_AUTHORING_API AZStd::string ToString(const FaceVertHandles& faceVertHandles);
        JE_GEOMETRY_AUTHORING_API AZStd::string ToString(const FaceVertHandlesList& faceVertHandlesList);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    } // namespace Api
} // namespace JE::Geometry
