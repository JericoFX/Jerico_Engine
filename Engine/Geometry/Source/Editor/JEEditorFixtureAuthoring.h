/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <JE/Geometry/GeometryExports.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <JE/Geometry/WhiteBoxToolApi.h>
#include <Rendering/WhiteBoxRenderData.h>
#include <JE/Render/Materials/MaterialAssignment.h>

namespace JE::BSP::Editor
{
    inline constexpr const char* FixtureAuthorActionId = "je.action.editor.bsp.authorRoomFixture";
    inline constexpr const char* FixtureCompleteEntityName = "JEFixture_F63AAB68_RoomCorridor_complete";
    inline constexpr uint32_t FixtureGridSlot = 1;
    inline constexpr uint32_t FixtureMetalSlot = 2;
    inline constexpr const char* FixtureGridMaterial = "materials/presets/pbr/default_grid.azmaterial";
    inline constexpr const char* FixtureMetalMaterial = "materials/presets/pbr/metal_iron.azmaterial";
    //! Requires existing compiled products; never registers, compiles, or synchronously loads assets.
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<JE::Render::MaterialAssignmentMap, AZStd::string> PreflightFixtureMaterials();
    //! +Z faces are upper faces (not necessarily floors); all remaining faces use the second slot.
    //! Does not mutate geometry, UVs or caller-owned UUID128 identities.
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<SurfaceMaterialBindings, AZStd::string> BuildFixtureSurfaceBindings(const JE::Geometry::WhiteBoxMesh& mesh);
    //! Requires full face coverage and the two exact distinct product IDs, with no fallback slot.
    JE_GEOMETRY_AUTHORING_API bool ValidateFixtureSurfaceMaterials(const JE::Geometry::WhiteBoxMesh& mesh, const SurfaceMaterialBindings& bindings,
        const JE::Render::MaterialAssignmentMap& materials, const JE::Render::MaterialAssignmentMap& expected, AZStd::string& error);
    JE_GEOMETRY_AUTHORING_API bool IsFixtureProjectOpen();
    //! Dispatches the registered action and observes its authoring outcome without a modal dialog.
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<void, AZStd::string> InvokeFixtureActionForVerification();
    //! Uses existing native primitives/CSG; does not touch Editor entities or the filesystem.
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<JE::Geometry::Api::WhiteBoxMeshPtr, AZStd::string> BuildRoomFixture();
    //! Lexical guard, before filesystem canonicalization and Editor ownership checks.
    JE_GEOMETRY_AUTHORING_API bool IsFixtureLevelPath(AZStd::string_view project, AZStd::string_view level, AZStd::string_view name);
    //! Authors and reads back the existing reflected tint/texture properties.
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<void, AZStd::string> ConfigureFixtureMaterial(const AZ::EntityComponentIdPair& component);
    JE_GEOMETRY_AUTHORING_API AZ::Outcome<void, AZStd::string> AuthorRoomFixture();
    JE_GEOMETRY_AUTHORING_API void RegisterFixtureAction();
    JE_GEOMETRY_AUTHORING_API void BindFixtureActionToMenu();
}
