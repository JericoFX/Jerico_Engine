/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <JE/Geometry/GeometryExports.h>
#include <Rendering/WhiteBoxRenderData.h>
#include <JE/Geometry/WhiteBoxToolApi.h>
#include <JE/Render/Materials/MaterialAssignment.h>
#include <AzCore/std/containers/unordered_set.h>

namespace JE::BSP
{
    struct SurfaceMaterialMerge
    {
        SurfaceMaterialBindings m_bindings;
        JE::Render::MaterialAssignmentMap m_materials;
    };

    //! Validate/merge scene associations before CSG. Failure leaves every output unchanged.
    //! Native MaterialComponent remains the owner of the returned references and overrides.
    JE_GEOMETRY_AUTHORING_API bool MergeSurfaceMaterials(
        const JE::Geometry::WhiteBoxMesh& target, const SurfaceMaterialBindings& targetBindings,
        const JE::Render::MaterialAssignmentMap& targetMaterials, const JE::Geometry::WhiteBoxMaterial& targetLegacy,
        const JE::Geometry::WhiteBoxMesh& source, const SurfaceMaterialBindings& sourceBindings,
        const JE::Render::MaterialAssignmentMap& sourceMaterials, const JE::Geometry::WhiteBoxMaterial& sourceLegacy,
        bool targetHasMaterialOwner, SurfaceMaterialMerge& output, AZStd::string& error);

    JE_GEOMETRY_AUTHORING_API bool MaterialAssignmentsEquivalent(const JE::Render::MaterialAssignmentMap& lhs,
        const JE::Render::MaterialAssignmentMap& rhs);

    //! Bind the complete inherited surface, not a truncated identifier or triangulation face index.
    JE_GEOMETRY_AUTHORING_API bool AssignPolygonMaterial(const JE::Geometry::WhiteBoxMesh& mesh, const JE::Geometry::Api::PolygonHandles& polygons,
        uint32_t slot, SurfaceMaterialBindings& bindings);
    //! Plan a native material selection without modifying mesh, bindings or MaterialComponent.
    //! Preserve compatible overrides; reuse effective associations or allocate isolated local slots.
    JE_GEOMETRY_AUTHORING_API bool PreparePolygonMaterialAssignment(const JE::Geometry::WhiteBoxMesh& mesh,
        const JE::Geometry::Api::PolygonHandles& polygons, const SurfaceMaterialBindings& bindings,
        const JE::Render::MaterialAssignmentMap& materials, const AZ::Data::Asset<JE::Render::RPI::MaterialAsset>& material,
        const AZStd::unordered_set<AZ::Name>& modelUvNames, bool hasMaterialOwner,
        SurfaceMaterialMerge& output, AZStd::string& error);

    //! Apply one validated affine transform to selected corners, preserving all other UVs and keys.
    JE_GEOMETRY_AUTHORING_API bool TransformPolygonUVs(JE::Geometry::WhiteBoxMesh& mesh, const JE::Geometry::Api::PolygonHandles& polygons,
        const AZ::Vector2& scale, const AZ::Vector2& offset, float rotationDegrees);
}
