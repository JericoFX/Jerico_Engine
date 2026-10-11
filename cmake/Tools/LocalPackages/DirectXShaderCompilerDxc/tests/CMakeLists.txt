# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../../Python/JERequireLocalPath.cmake")
foreach(JE_PATH IN ITEMS JE_DXC_PACKAGE_ROOT JE_DXC_SOURCE_ROOT CMAKE_BINARY_DIR)
    JE_python_require_local_path(${JE_PATH})
endforeach()
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${CMAKE_BINARY_DIR}" NORMALIZE JE_LOCAL_OUTPUT)
if(NOT JE_LOCAL_OUTPUT OR CMAKE_BINARY_DIR STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE DXC consumer output must be below checkout/build")
endif()
JE_python_verify_file("${JE_DXC_SOURCE_ROOT}/include/dxc/dxcapi.h"
    "1855295d3e9ed604a8f2b9dbd415a9d8f39f9e78b386f2f46fd00615da2cdfee" "pinned DXC API headers")
JE_python_verify_file("${JE_DXC_SOURCE_ROOT}/lib/DxilHash/DxilHash.cpp"
    "d66b631ea28d4c90a8217fac004f9290eaac07f79115451e0c4e954c0b0c94af" "pinned upstream retail hash oracle")
JE_python_verify_file("${JE_DXC_SOURCE_ROOT}/include/dxc/DxilHash/DxilHash.h"
    "4cff245da88b69526290495b2e439745a938aa60e48fd565832c9d8d2dfe6918" "pinned hash API header")
project(JELocalDirectXShaderCompilerDxcConsumer LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "JE DXC consumer requires Windows MSVC x64")
endif()
list(PREPEND CMAKE_MODULE_PATH "${JE_DXC_PACKAGE_ROOT}")
find_package(DirectXShaderCompilerDxc 1.8.2505.1 EXACT REQUIRED MODULE)
add_executable(JEDirectXShaderCompilerDxcConsumer DirectXShaderCompilerDxcConsumer.cpp
    "${JE_DXC_SOURCE_ROOT}/lib/DxilHash/DxilHash.cpp")
target_compile_features(JEDirectXShaderCompilerDxcConsumer PRIVATE cxx_std_17)
target_compile_options(JEDirectXShaderCompilerDxcConsumer PRIVATE /W4)
target_include_directories(JEDirectXShaderCompilerDxcConsumer SYSTEM PRIVATE "${JE_DXC_SOURCE_ROOT}/include")
target_link_libraries(JEDirectXShaderCompilerDxcConsumer PRIVATE 3rdParty::DirectXShaderCompilerDxc ole32 version)
enable_testing()
foreach(JE_CASE IN ITEMS dxil spirv invalid)
    add_test(NAME JE.LocalPackages.DXC.${JE_CASE} COMMAND JEDirectXShaderCompilerDxcConsumer
        ${JE_CASE} "${DirectXShaderCompilerDxc_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/${JE_CASE}" "${CMAKE_CURRENT_LIST_DIR}")
    set_tests_properties(JE.LocalPackages.DXC.${JE_CASE} PROPERTIES TIMEOUT 90)
endforeach()
add_test(NAME JE.LocalPackages.DXC.dxsc COMMAND "${CMAKE_COMMAND}"
    "-DJE_DXSC=${DirectXShaderCompilerDxc_BINARY_DIR}/dxsc.exe"
    "-DJE_CONSUMER=$<TARGET_FILE:JEDirectXShaderCompilerDxcConsumer>"
    "-DJE_BIN=${DirectXShaderCompilerDxc_BINARY_DIR}"
    "-DJE_INPUT=${CMAKE_CURRENT_BINARY_DIR}/dxil/fixture.dxil"
    "-DJE_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/dxsc"
    "-DJE_INCLUDES=${CMAKE_CURRENT_LIST_DIR}" -P "${CMAKE_CURRENT_LIST_DIR}/RunDxsc.cmake")
set_tests_properties(JE.LocalPackages.DXC.dxsc PROPERTIES DEPENDS JE.LocalPackages.DXC.dxil TIMEOUT 90)
