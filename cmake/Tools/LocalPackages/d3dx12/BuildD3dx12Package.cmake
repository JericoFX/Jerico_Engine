# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Stage the original single-header provider; no compiler, acquisition or upstream CMake.
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE SOURCE_SHA256 SOURCE_URL OUTPUT)
    if(NOT DEFINED JE_D3DX12_${JE_ARGUMENT} OR "${JE_D3DX12_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local d3dx12: JE_D3DX12_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_D3DX12_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_D3DX12_OUTPUT)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_D3DX12_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_D3DX12_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local d3dx12: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local d3dx12 package requires Windows")
endif()
if(EXISTS "${JE_D3DX12_OUTPUT}")
    message(FATAL_ERROR "JE local d3dx12: output already exists; retained outputs are never overwritten")
endif()
set(JE_SOURCE_COMMIT "70f0b7b5257bf7cf935bf1cfd270e1b53ccead18")
set(JE_EXPECTED_HASH "f69edb8bdf9d3dcd83cf296db31130f4d16c82efb9b4cdebd76c8035ccaa6042")
set(JE_EXPECTED_URL "https://codeload.github.com/o3de/3p-package-source/tar.gz/${JE_SOURCE_COMMIT}")
string(TOLOWER "${JE_D3DX12_SOURCE_SHA256}" JE_SOURCE_HASH)
if(NOT JE_SOURCE_HASH STREQUAL JE_EXPECTED_HASH OR NOT JE_D3DX12_SOURCE_URL STREQUAL JE_EXPECTED_URL)
    message(FATAL_ERROR "JE local d3dx12: source URL/SHA256 must match the audited O3DE source archive")
endif()
JE_python_verify_file("${JE_D3DX12_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "d3dx12 O3DE source archive")

set(JE_ARCHIVE_ROOT "3p-package-source-${JE_SOURCE_COMMIT}")
set(JE_WORK "${JE_D3DX12_OUTPUT}/source-work/${JE_ARCHIVE_ROOT}/package-system/d3dx12-windows")
set(JE_STAGE "${JE_D3DX12_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_D3DX12_OUTPUT}/source-work")
# Entire archive audited; extract only four byte-pinned regular files, zero links.
file(ARCHIVE_EXTRACT INPUT "${JE_D3DX12_SOURCE_ARCHIVE}" DESTINATION "${JE_D3DX12_OUTPUT}/source-work"
    PATTERNS "${JE_ARCHIVE_ROOT}/package-system/d3dx12-windows/*")
JE_python_verify_file("${JE_WORK}/d3dx12/include/d3dx12.h"
    "632dfbbb8c4683dc28569c9eaf5805c1caa059c730246bcd6235100062ff0273" "original d3dx12 header")
JE_python_verify_file("${JE_WORK}/d3dx12/license.txt"
    "fd532481d828e13a0b13ccb598e02338a3617740675a862ee6bdc1541b68e93d" "Microsoft MIT license")
JE_python_verify_file("${JE_WORK}/Findd3dx12.cmake"
    "a8c0cdb1770c4470570c601fb1f817f79a23f0dafecebe6b4fae946e37580ad0" "original O3DE Findd3dx12")
JE_python_verify_file("${JE_WORK}/PackageInfo.json"
    "cdaf233596a2423c47a009a7fb53de31252b9b4b39163532a2a2e333132dc24d" "original O3DE metadata")
file(MAKE_DIRECTORY "${JE_STAGE}/d3dx12/include" "${JE_STAGE}/LICENSES/O3DE")
configure_file("${JE_WORK}/d3dx12/include/d3dx12.h" "${JE_STAGE}/d3dx12/include/d3dx12.h" COPYONLY)
configure_file("${JE_WORK}/d3dx12/license.txt" "${JE_STAGE}/d3dx12/license.txt" COPYONLY)
configure_file("${JE_WORK}/Findd3dx12.cmake" "${JE_STAGE}/LICENSES/O3DE/OriginalFindd3dx12.cmake" COPYONLY)
configure_file("${JE_WORK}/PackageInfo.json" "${JE_STAGE}/LICENSES/O3DE/OriginalPackageInfo.json" COPYONLY)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/Findd3dx12.cmake" "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt"
    "${CMAKE_CURRENT_LIST_DIR}/README.md" DESTINATION "${JE_STAGE}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake"
    "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(WRITE "${JE_STAGE}/d3dx12/JERecipe.json"
    "{\n  \"sourceRepository\": \"o3de/3p-package-source\",\n"
    "  \"sourceCommit\": \"${JE_SOURCE_COMMIT}\", \"upstreamMicrosoftRevision\": null,\n"
    "  \"originalPackageName\": \"d3dx12-headers-rev1-windows\",\n"
    "  \"headerSHA256\": \"632dfbbb8c4683dc28569c9eaf5805c1caa059c730246bcd6235100062ff0273\",\n"
    "  \"licenseSHA256\": \"fd532481d828e13a0b13ccb598e02338a3617740675a862ee6bdc1541b68e93d\",\n"
    "  \"headerModified\": false, \"libraryType\": \"INTERFACE\",\n"
    "  \"sdkHeaders\": \"Installed Windows SDK d3d12.h; not redistributed\",\n"
    "  \"historicalPackageHashMatch\": \"not asserted\"\n}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "d3dx12-headers-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_D3DX12_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "headers-je-rev1")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_LICENSE "MIT")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_EXPECTED_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
