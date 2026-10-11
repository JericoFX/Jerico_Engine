# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
JE_python_require_local_path(JE_ZSTD_WORK)
JE_python_require_local_path(CMAKE_BINARY_DIR)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
foreach(JE_PATH IN ITEMS JE_ZSTD_WORK CMAKE_BINARY_DIR)
    cmake_path(IS_PREFIX JE_BUILD_ROOT "${${JE_PATH}}" NORMALIZE JE_PATH_IN_BUILD)
    if(NOT JE_PATH_IN_BUILD OR "${${JE_PATH}}" STREQUAL JE_BUILD_ROOT)
        message(FATAL_ERROR "JE local zstd: ${JE_PATH} must be beneath this checkout/build")
    endif()
endforeach()
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Release package producer" FORCE)
# Compatibility applies only to this standalone producer and the upstream child.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
project(JELocalZstd LANGUAGES C CXX)
if(NOT MSVC OR NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022"
    OR NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64" OR NOT CMAKE_VS_PLATFORM_TOOLSET STREQUAL "v143")
    message(FATAL_ERROR "JE local zstd requires Visual Studio 2022 Windows x64 v143")
endif()
foreach(JE_REQUIRED IN ITEMS build/cmake/CMakeLists.txt lib/zstd.h LICENSE COPYING)
    if(NOT EXISTS "${JE_ZSTD_WORK}/${JE_REQUIRED}")
        message(FATAL_ERROR "JE local zstd: missing pinned source file ${JE_REQUIRED}")
    endif()
endforeach()
file(READ "${JE_ZSTD_WORK}/lib/zstd.h" JE_HEADER)
if(NOT JE_HEADER MATCHES "#define[ \t]+ZSTD_VERSION_MAJOR[ \t]+1([\r\n]|$)"
    OR NOT JE_HEADER MATCHES "#define[ \t]+ZSTD_VERSION_MINOR[ \t]+3([\r\n]|$)"
    OR NOT JE_HEADER MATCHES "#define[ \t]+ZSTD_VERSION_RELEASE[ \t]+5([\r\n]|$)")
    message(FATAL_ERROR "JE local zstd: explicit work source must be version 1.3.5")
endif()
set(ZSTD_BUILD_STATIC ON CACHE BOOL "Full static library" FORCE)
foreach(JE_OPTION IN ITEMS SHARED PROGRAMS TESTS CONTRIB)
    set(ZSTD_BUILD_${JE_OPTION} OFF CACHE BOOL "Library-only package" FORCE)
endforeach()
set(ZSTD_LEGACY_SUPPORT ON CACHE BOOL "Preserve upstream Windows legacy level4" FORCE)
set(ZSTD_MULTITHREAD_SUPPORT ON CACHE BOOL "Preserve upstream Windows multithread support" FORCE)
set(ZSTD_USE_STATIC_RUNTIME OFF CACHE BOOL "Use engine Release DLL runtime" FORCE)
add_subdirectory("${JE_ZSTD_WORK}/build/cmake" "${CMAKE_CURRENT_BINARY_DIR}/upstream")
set_target_properties(libzstd_static PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreadedDLL
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/lib/release")
