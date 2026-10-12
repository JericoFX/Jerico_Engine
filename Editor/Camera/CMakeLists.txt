# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
ly_add_target(
    NAME EditorCamera ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE camera_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_EDITOR_CAMERA_EXPORTS
    BUILD_DEPENDENCIES PUBLIC JE::WorldCamera AZ::AzToolsFramework
        Gem::AtomToolsFramework.Static
)
set_target_properties(EditorCamera PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME EditorCamera.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE camera_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source
        BUILD_DEPENDENCIES PRIVATE AZ::AzTestShared AZ::AzTest AZ::AzToolsFrameworkTestCommon JE::EditorCamera
    )
    ly_add_googletest(NAME JE::EditorCamera.Tests)
endif()
