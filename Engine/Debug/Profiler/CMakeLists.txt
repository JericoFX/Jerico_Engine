# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_add_target(
    NAME ProfilerCapture ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE profiler_capture_files.cmake
    INCLUDE_DIRECTORIES PUBLIC Include PRIVATE Source
    COMPILE_DEFINITIONS PRIVATE JE_PROFILER_CAPTURE_EXPORTS
    BUILD_DEPENDENCIES PUBLIC AZ::AzCore AZ::AzFramework
)
set_target_properties(ProfilerCapture PROPERTIES UNITY_BUILD OFF)

ly_add_target(
    NAME ProfilerView ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE profiler_view_files.cmake
    INCLUDE_DIRECTORIES PUBLIC Include PRIVATE Source
    COMPILE_DEFINITIONS PRIVATE JE_PROFILER_VIEW_EXPORTS
    BUILD_DEPENDENCIES PUBLIC JE::ProfilerCapture JE::DebugImGui
)
set_target_properties(ProfilerView PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME Profiler.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE profiler_tests_files.cmake
        BUILD_DEPENDENCIES PRIVATE JE::ProfilerCapture JE::ProfilerView AZ::AzTest AZ::AzTestShared
    )
    set_target_properties(Profiler.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::Profiler.Tests)
endif()
