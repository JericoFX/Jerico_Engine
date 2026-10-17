# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

add_subdirectory(Render/Materials)
add_subdirectory(Geometry)
add_subdirectory(World/Camera)
add_subdirectory(Assets/Primitives)
add_subdirectory(Debug/ImGui)
add_subdirectory(Debug/Profiler)
add_subdirectory(Render/Debug)
add_subdirectory(Render/Integration)
add_subdirectory(Render/Scene)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    add_subdirectory(Assets/PrefabBuilder)
    add_subdirectory(Assets/RenderBuilders)
    add_subdirectory(${LY_ROOT_FOLDER}/Editor/Materials ${CMAKE_CURRENT_BINARY_DIR}/Editor/Materials)
    add_subdirectory(${LY_ROOT_FOLDER}/Editor/Camera ${CMAKE_CURRENT_BINARY_DIR}/Editor/Camera)
endif()
