# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Keep inactive backend/platform sources while deploying only permitted desktop headers.
# Null is retained solely for asset tools and headless tests.
set(je_shader_header_platforms ${PAL_PLATFORM_NAME} ${LY_PAL_TOOLS_ENABLED})
list(REMOVE_DUPLICATES je_shader_header_platforms)
foreach(je_shader_header_platform IN LISTS je_shader_header_platforms)
    if(je_shader_header_platform STREQUAL "Windows" OR je_shader_header_platform STREQUAL "Linux")
        ly_add_target_files(
            TARGETS RenderAssetBuilders
            FILES
                ${CMAKE_CURRENT_LIST_DIR}/ShaderHeaders/Platform/${je_shader_header_platform}/Vulkan/AzslcHeader.azsli
                ${CMAKE_CURRENT_LIST_DIR}/ShaderHeaders/Platform/${je_shader_header_platform}/Vulkan/PlatformHeader.hlsli
            OUTPUT_SUBDIRECTORY Builders/ShaderHeaders/Platform/${je_shader_header_platform}/Vulkan
        )
        ly_add_target_files(
            TARGETS RenderAssetBuilders
            FILES ${CMAKE_CURRENT_LIST_DIR}/ShaderHeaders/Platform/${je_shader_header_platform}/Null/AzslcHeader.azsli
            OUTPUT_SUBDIRECTORY Builders/ShaderHeaders/Platform/${je_shader_header_platform}/Null
        )
    endif()
endforeach()
unset(je_shader_header_platform)
unset(je_shader_header_platforms)