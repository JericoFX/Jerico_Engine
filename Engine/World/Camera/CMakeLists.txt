# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_install_directory(DIRECTORIES Assets)
ly_add_target(
    NAME WorldCamera ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE camera_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_WORLD_CAMERA_EXPORTS
    BUILD_DEPENDENCIES PUBLIC AZ::AzCore AZ::AzFramework JE::RenderIntegration
)
set_target_properties(WorldCamera PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME WorldCamera.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE camera_tests_files.cmake
        BUILD_DEPENDENCIES PRIVATE AZ::AzTestShared AZ::AzTest JE::WorldCamera
    )
    set_target_properties(WorldCamera.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::WorldCamera.Tests)
endif()
