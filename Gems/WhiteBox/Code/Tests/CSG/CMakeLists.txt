# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

cmake_minimum_required(VERSION 3.24)
project(JE_BspCsgTests LANGUAGES CXX)

# This test compiles the production kernel. Source acquisition is deliberately
# external: only an explicitly supplied, audited local Manifold tree is used.
set(JE_MANIFOLD_SOURCE_DIR "" CACHE PATH "Local audited Manifold v3.5.1 source tree")
if(NOT EXISTS "${JE_MANIFOLD_SOURCE_DIR}/include/manifold/manifold.h"
    OR NOT EXISTS "${JE_MANIFOLD_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JE BSP CSG tests require JE_MANIFOLD_SOURCE_DIR; no dependency will be downloaded")
endif()

set(MANIFOLD_DOWNLOADS OFF CACHE BOOL "" FORCE)
set(MANIFOLD_CROSS_SECTION OFF CACHE BOOL "" FORCE)
set(MANIFOLD_PAR OFF CACHE BOOL "" FORCE)
set(MANIFOLD_TEST OFF CACHE BOOL "" FORCE)
set(MANIFOLD_CBIND OFF CACHE BOOL "" FORCE)
set(MANIFOLD_PYBIND OFF CACHE BOOL "" FORCE)
set(MANIFOLD_DEBUG OFF CACHE BOOL "" FORCE)
set(MANIFOLD_FUZZ OFF CACHE BOOL "" FORCE)
set(TRACY_ENABLE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory("${JE_MANIFOLD_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/manifold" EXCLUDE_FROM_ALL)

add_executable(JE_BspCsgTests
    WhiteBoxCsgCoreTest.cpp
    ../../Source/Core/WhiteBoxCsgCore.cpp)
target_include_directories(JE_BspCsgTests PRIVATE ../../Source/Core)
# Manifold's public headers retain their upstream warning policy. The production
# JE kernel and test code still compile with /W4 /WX below.
target_include_directories(JE_BspCsgTests SYSTEM PRIVATE
    "${JE_MANIFOLD_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/manifold/include")
target_compile_features(JE_BspCsgTests PRIVATE cxx_std_17)
target_link_libraries(JE_BspCsgTests PRIVATE manifold)
if(MSVC)
    target_compile_options(JE_BspCsgTests PRIVATE /W4 /WX)
endif()

enable_testing()
add_test(NAME JE.BSP.CsgCore COMMAND JE_BspCsgTests)
