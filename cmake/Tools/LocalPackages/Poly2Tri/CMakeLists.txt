# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
JE_python_require_local_path(JE_POLY2TRI_WORK)
JE_python_require_local_path(CMAKE_BINARY_DIR)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
foreach(JE_PATH IN ITEMS JE_POLY2TRI_WORK CMAKE_BINARY_DIR)
    cmake_path(IS_PREFIX JE_BUILD_ROOT "${${JE_PATH}}" NORMALIZE JE_IN_BUILD)
    if(NOT JE_IN_BUILD OR "${${JE_PATH}}" STREQUAL JE_BUILD_ROOT)
        message(FATAL_ERROR "JE local poly2tri: ${JE_PATH} must be beneath this checkout/build")
    endif()
endforeach()
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Original package configurations" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
project(JELocalPoly2Tri LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022"
    OR NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64" OR NOT CMAKE_VS_PLATFORM_TOOLSET STREQUAL "v143")
    message(FATAL_ERROR "JE local poly2tri requires Visual Studio 2022 Windows x64 v143")
endif()
# The historical vcpkg port compiled these five files directly. Do not evaluate
# upstream testbed/tests or bootstrap vcpkg merely to build the STL-only library.
add_library(JEPoly2Tri STATIC
    "${JE_POLY2TRI_WORK}/poly2tri/common/shapes.cc"
    "${JE_POLY2TRI_WORK}/poly2tri/sweep/advancing_front.cc"
    "${JE_POLY2TRI_WORK}/poly2tri/sweep/cdt.cc"
    "${JE_POLY2TRI_WORK}/poly2tri/sweep/sweep.cc"
    "${JE_POLY2TRI_WORK}/poly2tri/sweep/sweep_context.cc")
target_compile_features(JEPoly2Tri PRIVATE cxx_std_14)
target_compile_options(JEPoly2Tri PRIVATE /EHsc)
set_target_properties(JEPoly2Tri PROPERTIES OUTPUT_NAME poly2tri
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/lib/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/lib/Release")
