# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_add_target(
    NAME EditorRenderIntegration ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE render_integration_files.cmake rpi_edit_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source ${LY_ROOT_FOLDER}/Engine/Render/Integration/Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_EDITOR_RENDER_INTEGRATION_EXPORTS EDITOR JE_RENDER_RPI_EDITOR
    BUILD_DEPENDENCIES
        PUBLIC JE::RenderIntegration
        PRIVATE AZ::AzToolsFramework AZ::AssetBuilderSDK Gem::Atom_Utils.Static Gem::Atom_RHI.Edit
)
set_target_properties(EditorRenderIntegration PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME EditorRenderIntegration.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE render_integration_tests_files.cmake
        BUILD_DEPENDENCIES PRIVATE JE::EditorRenderIntegration AZ::AzToolsFramework AZ::AzTest AZ::AzTestShared
    )
    set_target_properties(EditorRenderIntegration.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::EditorRenderIntegration.Tests)
endif()
