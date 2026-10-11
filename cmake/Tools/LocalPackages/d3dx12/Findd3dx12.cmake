# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Adapted from o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18,
# package-system/d3dx12-windows/Findd3dx12.cmake. Header API is unmodified.

include("${CMAKE_CURRENT_LIST_DIR}/JERequireLocalPath.cmake")
set(JE_D3DX12_ROOT "${CMAKE_CURRENT_LIST_DIR}")
JE_python_require_local_path(JE_D3DX12_ROOT)
if(NOT WIN32 OR (DEFINED CMAKE_SIZEOF_VOID_P AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8))
    message(FATAL_ERROR "JE local d3dx12 package requires Windows x64")
endif()
set(d3dx12_INCLUDE_DIR "${JE_D3DX12_ROOT}/d3dx12/include")
set(JE_D3DX12_HEADER "${d3dx12_INCLUDE_DIR}/d3dx12.h")
JE_python_verify_file("${JE_D3DX12_HEADER}"
    "632dfbbb8c4683dc28569c9eaf5805c1caa059c730246bcd6235100062ff0273" "original d3dx12 header")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(d3dx12 REQUIRED_VARS d3dx12_INCLUDE_DIR)
if(NOT TARGET 3rdParty::d3dx12)
    add_library(3rdParty::d3dx12 INTERFACE IMPORTED GLOBAL)
    if(COMMAND ly_target_include_system_directories)
        ly_target_include_system_directories(TARGET 3rdParty::d3dx12 INTERFACE "${d3dx12_INCLUDE_DIR}")
    else()
        target_include_directories(3rdParty::d3dx12 SYSTEM INTERFACE "${d3dx12_INCLUDE_DIR}")
    endif()
endif()
message(STATUS "Using JE local original d3dx12 header from ${JE_D3DX12_ROOT}")
