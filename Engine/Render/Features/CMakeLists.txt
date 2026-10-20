# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_install_directory(DIRECTORIES Assets
    EXCLUDE_PATTERNS
        "DiffuseProbeGrid/Shaders/DiffuseGlobalIllumination/*_dx12_*.azshadervariant"
        "DiffuseProbeGrid/Shaders/DiffuseGlobalIllumination/*_null_*.azshadervariant"
)

set(features_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
set(features_dpg_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/DiffuseProbeGrid/Platform/${PAL_PLATFORM_NAME})
set(common_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/Common)

# One real shared provider in modular builds; preserve the existing PAL/monolithic selector.
ly_add_target(
    NAME RenderFeatures ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE features_public_files.cmake features_private_files.cmake features_module_files.cmake
        features_asset_files.cmake features_diffuse_probe_grid_files.cmake
        ${features_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${features_dpg_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES ${features_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        ${features_pal_dir}/runtime_dependencies_clients.cmake
    INCLUDE_DIRECTORIES PRIVATE Source ${features_pal_dir} Assets/ShaderLib
        Source/DiffuseProbeGrid ${features_dpg_pal_dir}
        PUBLIC Include External/ACES
    COMPILE_DEFINITIONS PRIVATE JE_RENDER_FEATURES_EXPORTS IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    BUILD_DEPENDENCIES PUBLIC AZ::AzCore AZ::AzFramework JE::RenderIntegration Gem::Atom_Utils.Static
        PRIVATE JE::DearImGui 3rdParty::TIFF Gem::LmbrCentral.API
    RUNTIME_DEPENDENCIES JE::DearImGui
)
set_target_properties(RenderFeatures PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    add_subdirectory(${LY_ROOT_FOLDER}/Editor/Render/Features ${CMAKE_CURRENT_BINARY_DIR}/Editor)
endif()

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # Original private-body suite only; never part of product hosts or installed SDK.
    ly_add_target(
        NAME RenderFeatures.TestPrivate STATIC
        NAMESPACE JE
        FILES_CMAKE features_test_private_files.cmake ${features_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
            ${features_dpg_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        PLATFORM_INCLUDE_FILES ${features_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
        INCLUDE_DIRECTORIES PRIVATE Source ${features_pal_dir} Assets/ShaderLib External/ACES
            Source/DiffuseProbeGrid ${features_dpg_pal_dir}
        COMPILE_DEFINITIONS PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        BUILD_DEPENDENCIES PUBLIC JE::RenderFeatures JE::DearImGui 3rdParty::TIFF
            PRIVATE Gem::LmbrCentral.API
    )
    set_target_properties(RenderFeatures.TestPrivate PROPERTIES UNITY_BUILD OFF JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_target(
        NAME RenderFeatures.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE features_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source Tests Source/DiffuseProbeGrid ${features_dpg_pal_dir} PUBLIC Tests/Mocks
        BUILD_DEPENDENCIES PRIVATE AZ::AzFramework AZ::AzTest JE::RenderFeatures.TestPrivate JE::RenderFeatures JE::RenderRpi.TestUtils Gem::LmbrCentral.API
    )
    set_target_properties(RenderFeatures.Tests PROPERTIES UNITY_BUILD OFF JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_googletest(NAME JE::RenderFeatures.Tests LABELS REQUIRES_tiaf)
endif()
