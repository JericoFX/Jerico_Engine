# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_install_directory(DIRECTORIES Assets)
ly_install_directory(DIRECTORIES RPI/Assets)

set(je_render_integration_bootstrap_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Bootstrap/Platform/${PAL_PLATFORM_NAME})
set(je_render_integration_rpi_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/RPI/Platform/${PAL_PLATFORM_NAME})
include(${je_render_integration_rpi_pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
set(je_render_integration_rpi_occlusion_files)
if(PAL_TRAIT_BUILD_ATOM_RPI_MASKED_OCCLUSION_CULLING_SUPPORTED)
    set(je_render_integration_rpi_occlusion_files rpi_masked_occlusion_files.cmake)
endif()

# Header contracts have no implementation backedge; function users link RenderIntegration.
ly_add_target(
    NAME RenderIntegration.Public HEADERONLY
    NAMESPACE JE
    FILES_CMAKE render_integration_public_files.cmake
    INCLUDE_DIRECTORIES INTERFACE Include
    BUILD_DEPENDENCIES INTERFACE AZ::AzCore AZ::AzFramework Gem::Atom_RHI.Public
)

ly_add_target(
    NAME RenderIntegration ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE
        render_integration_files.cmake
        rpi_public_files.cmake
        rpi_reflect_files.cmake
        RPI/Assets/atom_rpi_asset_files.cmake
        ${je_render_integration_bootstrap_pal_dir}/bootstrap_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${je_render_integration_rpi_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${je_render_integration_rpi_occlusion_files}
    PLATFORM_INCLUDE_FILES
        ${CMAKE_CURRENT_LIST_DIR}/Source/RPI/Platform/Common/${PAL_TRAIT_COMPILER_ID}/atom_rpi_public_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE Source ${je_render_integration_bootstrap_pal_dir} ${je_render_integration_rpi_pal_dir}
        PUBLIC Include External/RPI
    COMPILE_DEFINITIONS PRIVATE JE_RENDER_INTEGRATION_EXPORTS ENABLE_ATOM_DEBUG_DISPLAY=1
    BUILD_DEPENDENCIES
        PUBLIC JE::RenderIntegration.Public Gem::Atom_RHI.Public
        PRIVATE Legacy::CryCommon Gem::Atom_RHI.Reflect AZ::AzCore AZ::AzFramework 3rdParty::meshoptimizer
)
set_target_properties(RenderIntegration PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME RenderIntegration.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE render_integration_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source ${LY_ROOT_FOLDER}/Editor/Render/Integration/Include
        BUILD_DEPENDENCIES PRIVATE JE::RenderIntegration AZ::AzTest AZ::AzTestShared
    )
    set_target_properties(RenderIntegration.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::RenderIntegration.Tests)

    ly_add_target(
        NAME RenderRpi.TestUtils STATIC
        NAMESPACE JE
        FILES_CMAKE rpi_test_utils_files.cmake
        INCLUDE_DIRECTORIES PUBLIC Tests/RPI
        BUILD_DEPENDENCIES PRIVATE AZ::AzTest AZ::AzTestShared AZ::AzFramework JE::RenderIntegration Gem::Atom_RHI.Public
    )
    set_target_properties(RenderRpi.TestUtils PROPERTIES JE_EXCLUDE_FROM_SDK TRUE)
endif()

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    add_subdirectory(${LY_ROOT_FOLDER}/Editor/Render/Integration ${CMAKE_CURRENT_BINARY_DIR}/Editor)
endif()

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED AND PAL_TRAIT_BUILD_HOST_TOOLS)
    # Preserve the original RPI UnitTestMain/environment as its own test module.
    ly_add_target(
        NAME RenderRpi.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE rpi_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source Tests/RPI
            ${LY_ROOT_FOLDER}/Engine/Render/Features/Source/DiffuseProbeGrid
            ${LY_ROOT_FOLDER}/Engine/Render/Features/Source/DiffuseProbeGrid/Platform/${PAL_PLATFORM_NAME}
        BUILD_DEPENDENCIES PRIVATE
            AZ::AzTest AZ::AzTestShared AZ::AzFramework AZ::AzToolsFramework Legacy::CryCommon
            Gem::Atom_RHI.Public Gem::Atom_Utils.TestUtils.Static
            JE::EditorRenderIntegration JE::RenderIntegration JE::RenderRpi.TestUtils
            JE::RenderFeatures JE::RenderFeatures.TestPrivate Gem::LmbrCentral.API
    )
    set_target_properties(RenderRpi.Tests PROPERTIES JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_googletest(NAME JE::RenderRpi.Tests LABELS REQUIRES_tiaf)
endif()
