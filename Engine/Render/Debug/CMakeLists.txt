# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_install_directory(DIRECTORIES Assets)

# The bus contract carries no implementation dependency back to the Atom adapters.
ly_add_target(
    NAME RenderDebug.Public HEADERONLY
    NAMESPACE JE
    FILES_CMAKE render_debug_public_files.cmake
    INCLUDE_DIRECTORIES INTERFACE Include
    BUILD_DEPENDENCIES INTERFACE AZ::AzCore JE::RenderIntegration
)

ly_add_target(
    NAME RenderDebug ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE render_debug_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_RENDER_DEBUG_EXPORTS
    BUILD_DEPENDENCIES
        PUBLIC JE::RenderDebug.Public
        PRIVATE AZ::AzFramework Legacy::CryCommon JE::DebugImGui
            JE::RenderFeatures Gem::Atom_Utils.Static
)
set_target_properties(RenderDebug PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME RenderDebug.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE render_debug_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source
        BUILD_DEPENDENCIES PRIVATE JE::RenderDebug JE::DebugImGui Gem::Atom_Utils.Static AZ::AzTest AZ::AzTestShared
    )
    set_target_properties(RenderDebug.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::RenderDebug.Tests)
endif()
