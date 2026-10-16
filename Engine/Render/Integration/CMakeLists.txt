# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_install_directory(DIRECTORIES Assets)

set(je_render_integration_bootstrap_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Bootstrap/Platform/${PAL_PLATFORM_NAME})

# Pure contracts let Font/Icons use the shared AZ::Interface without a runtime backedge.
ly_add_target(
    NAME RenderIntegration.Public HEADERONLY
    NAMESPACE JE
    FILES_CMAKE render_integration_public_files.cmake
    INCLUDE_DIRECTORIES INTERFACE Include
    BUILD_DEPENDENCIES INTERFACE AZ::AzCore AZ::AzFramework Gem::Atom_RPI.Public
)

ly_add_target(
    NAME RenderIntegration ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE
        render_integration_files.cmake
        ${je_render_integration_bootstrap_pal_dir}/bootstrap_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source ${je_render_integration_bootstrap_pal_dir} PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_RENDER_INTEGRATION_EXPORTS ENABLE_ATOM_DEBUG_DISPLAY=1
    BUILD_DEPENDENCIES
        PUBLIC JE::RenderIntegration.Public
        PRIVATE Legacy::CryCommon
)
set_target_properties(RenderIntegration PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME RenderIntegration.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE render_integration_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source
        BUILD_DEPENDENCIES PRIVATE JE::RenderIntegration AZ::AzTest AZ::AzTestShared
    )
    set_target_properties(RenderIntegration.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::RenderIntegration.Tests)
endif()

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    add_subdirectory(${LY_ROOT_FOLDER}/Editor/Render/Integration ${CMAKE_CURRENT_BINARY_DIR}/Editor)
endif()
