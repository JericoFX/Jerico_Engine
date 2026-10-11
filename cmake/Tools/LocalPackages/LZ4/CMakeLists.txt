# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
JE_python_require_local_path(JE_LZ4_WORK)
JE_python_require_local_path(CMAKE_BINARY_DIR)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
foreach(JE_PATH IN ITEMS JE_LZ4_WORK CMAKE_BINARY_DIR)
    cmake_path(IS_PREFIX JE_BUILD_ROOT "${${JE_PATH}}" NORMALIZE JE_PATH_IN_BUILD)
    if(NOT JE_PATH_IN_BUILD OR "${${JE_PATH}}" STREQUAL JE_BUILD_ROOT)
        message(FATAL_ERROR "JE local lz4: ${JE_PATH} must be beneath this checkout/build")
    endif()
endforeach()
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Release package producer" FORCE)
# Only this standalone producer and its audited upstream child need old CMake compatibility.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
project(JELocalLz4 LANGUAGES C)
if(NOT MSVC OR NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022"
    OR NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64" OR NOT CMAKE_VS_PLATFORM_TOOLSET STREQUAL "v143")
    message(FATAL_ERROR "JE local lz4 requires Visual Studio 2022 Windows x64 v143")
endif()
foreach(JE_REQUIRED IN ITEMS build/cmake/CMakeLists.txt lib/lz4.c lib/lz4hc.c lib/lz4frame.c
    lib/xxhash.c lib/lz4.h lib/lz4hc.h lib/lz4frame.h lib/xxhash.h lib/liblz4.pc.in LICENSE lib/LICENSE)
    if(NOT EXISTS "${JE_LZ4_WORK}/${JE_REQUIRED}")
        message(FATAL_ERROR "JE local lz4: missing pinned source file ${JE_REQUIRED}")
    endif()
endforeach()
file(READ "${JE_LZ4_WORK}/lib/lz4.h" JE_HEADER)
if(NOT JE_HEADER MATCHES "#define[ \t]+LZ4_VERSION_MAJOR[ \t]+1([ \t\r\n]|$)"
    OR NOT JE_HEADER MATCHES "#define[ \t]+LZ4_VERSION_MINOR[ \t]+9([ \t\r\n]|$)"
    OR NOT JE_HEADER MATCHES "#define[ \t]+LZ4_VERSION_RELEASE[ \t]+4([ \t\r\n]|$)")
    message(FATAL_ERROR "JE local lz4: explicit work source must be version 1.9.4")
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static engine dependency" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "Core, HC, frame and xxHash library" FORCE)
set(LZ4_BUNDLED_MODE ON CACHE BOOL "No upstream installation or CPack" FORCE)
set(LZ4_BUILD_CLI OFF CACHE BOOL "No engine consumer of GPL CLI" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "No engine consumer of legacy GPL CLI" FORCE)
set(LZ4_POSITION_INDEPENDENT_LIB ON CACHE BOOL "Preserve upstream static-library default" FORCE)
add_subdirectory("${JE_LZ4_WORK}/build/cmake" "${CMAKE_CURRENT_BINARY_DIR}/upstream")
set_target_properties(lz4_static PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreadedDLL
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/lib")
