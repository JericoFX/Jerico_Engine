# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
JE_python_require_local_path(JE_EXPAT_WORK)
JE_python_require_local_path(CMAKE_BINARY_DIR)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
foreach(JE_PATH IN ITEMS JE_EXPAT_WORK CMAKE_BINARY_DIR)
    cmake_path(IS_PREFIX JE_BUILD_ROOT "${${JE_PATH}}" NORMALIZE JE_PATH_IN_BUILD)
    if(NOT JE_PATH_IN_BUILD OR "${${JE_PATH}}" STREQUAL JE_BUILD_ROOT)
        message(FATAL_ERROR "JE local Expat: ${JE_PATH} must be beneath this checkout/build")
    endif()
endforeach()
JE_python_verify_file("${JE_EXPAT_WORK}/CMakeLists.txt"
    "d2f6e71b0209468048cec60c9981be7367b911d99517dd0b3b76da91dc489203" "patched Expat CMake source")
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Release package producer" FORCE)
set(CMAKE_CXX_STANDARD 17)
project(JELocalExpat LANGUAGES C)
if(NOT MSVC OR NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022"
    OR NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64" OR NOT CMAKE_VS_PLATFORM_TOOLSET STREQUAL "v143")
    message(FATAL_ERROR "JE local Expat requires Visual Studio 2022 Windows x64 v143")
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Original static Expat recipe" FORCE)
foreach(JE_OPTION IN ITEMS BUILD_DOCS BUILD_EXAMPLES BUILD_FUZZERS BUILD_TESTS BUILD_TOOLS BUILD_PKGCONFIG
    SHARED_LIBS MSVC_STATIC_CRT ATTR_INFO LARGE_SIZE MIN_SIZE)
    set(EXPAT_${JE_OPTION} OFF CACHE BOOL "Original Windows Expat baseline" FORCE)
endforeach()
set(EXPAT_CHAR_TYPE char CACHE STRING "CrySystem char API" FORCE)
set(EXPAT_DTD ON CACHE BOOL "Original DTD support" FORCE)
set(EXPAT_NS ON CACHE BOOL "Original namespace support" FORCE)
set(EXPAT_CONTEXT_BYTES 1024 CACHE STRING "Original parse context" FORCE)
add_subdirectory("${JE_EXPAT_WORK}" "${CMAKE_CURRENT_BINARY_DIR}/upstream")
set_target_properties(expat PROPERTIES MSVC_RUNTIME_LIBRARY MultiThreadedDLL
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/lib")
