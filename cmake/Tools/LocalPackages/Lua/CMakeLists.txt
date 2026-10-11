# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Adapted from o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18,
# package-system/Lua/CMakeLists.txt and build_config.json.

cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
JE_python_require_local_path(JE_LUA_WORK)
JE_python_require_local_path(CMAKE_BINARY_DIR)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
foreach(JE_PATH IN ITEMS JE_LUA_WORK CMAKE_BINARY_DIR)
    cmake_path(IS_PREFIX JE_BUILD_ROOT "${${JE_PATH}}" NORMALIZE JE_INSIDE_BUILD)
    if(NOT JE_INSIDE_BUILD OR "${${JE_PATH}}" STREQUAL JE_BUILD_ROOT)
        message(FATAL_ERROR "JE local Lua: ${JE_PATH} must be beneath this checkout/build")
    endif()
endforeach()
if(NOT EXISTS "${JE_LUA_WORK}/onelua.c")
    message(FATAL_ERROR "JE local Lua: prepared patched work copy is missing")
endif()
project(JELocalLua LANGUAGES C)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022"
    OR NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64" OR NOT CMAKE_VS_PLATFORM_TOOLSET STREQUAL "v143")
    message(FATAL_ERROR "JE local Lua requires Visual Studio 2022 v143 Windows x64")
endif()
set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Original Lua package configuration" FORCE)
add_library(lualib STATIC "${JE_LUA_WORK}/onelua.c")
target_compile_definitions(lualib PRIVATE MAKE_LIB _CRT_SECURE_NO_WARNINGS)
set_target_properties(lualib PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
