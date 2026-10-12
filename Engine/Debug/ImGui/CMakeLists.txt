# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Keep the existing cache key/default as compatibility for configured builds.
set(O3DE_IMGUI_RELEASE_ENABLED ON CACHE BOOL "Allow IMGUI in release builds")
if($<CONFIG:release> AND NOT O3DE_IMGUI_RELEASE_ENABLED)
    set(config_base_defines IMGUI_DISABLED)
else()
    set(config_base_defines IMGUI_ENABLED)
endif()
set(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME DearImGui ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE dear_imgui_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES ${pal_source_dir}/imgui_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES PUBLIC External/ImGui/v1.82
    COMPILE_DEFINITIONS PUBLIC ${config_base_defines}
        INTERFACE $<$<NOT:$<BOOL:${LY_MONOLITHIC_GAME}>>:IMGUI_API_IMPORT>
    BUILD_DEPENDENCIES PUBLIC AZ::AzCore
)
set_target_properties(DearImGui PROPERTIES UNITY_BUILD OFF)

ly_add_target(
    NAME DebugImGui ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE debug_imgui_files.cmake
    PLATFORM_INCLUDE_FILES ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES PRIVATE Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_DEBUG_IMGUI_EXPORTS
    BUILD_DEPENDENCIES PUBLIC JE::DearImGui AZ::AzCore AZ::AzFramework Legacy::CryCommon
)
set_target_properties(DebugImGui PROPERTIES UNITY_BUILD OFF)

ly_install_directory(DIRECTORIES External)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME DebugImGui.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE debug_imgui_tests_files.cmake
        BUILD_DEPENDENCIES PRIVATE JE::DebugImGui AZ::AzTest AZ::AzTestShared Legacy::CryCommonMocks
    )
    set_target_properties(DebugImGui.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::DebugImGui.Tests)
endif()
