# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Local build of the parent-audited commit archive. No acquisition or global install.
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE SOURCE_SHA256 SOURCE_URL OUTPUT)
    if(NOT DEFINED JE_ZSTD_${JE_ARGUMENT} OR "${JE_ZSTD_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local zstd: JE_ZSTD_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_ZSTD_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_ZSTD_OUTPUT)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_ZSTD_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_ZSTD_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local zstd: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local zstd requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_ZSTD_OUTPUT}")
    message(FATAL_ERROR "JE local zstd: output must be new; existing outputs are preserved")
endif()
set(JE_SOURCE_COMMIT "90ae50224d15e8dbcb9fa26b9be096366733db8e")
set(JE_EXPECTED_HASH "dfb594dc41e79c1304ef2c9232746c048e4837578c1ca88cf0e9b83c8cc36457")
set(JE_EXPECTED_URL "https://codeload.github.com/facebook/zstd/tar.gz/${JE_SOURCE_COMMIT}")
string(TOLOWER "${JE_ZSTD_SOURCE_SHA256}" JE_SOURCE_HASH)
if(NOT JE_SOURCE_HASH STREQUAL JE_EXPECTED_HASH OR NOT JE_ZSTD_SOURCE_URL STREQUAL JE_EXPECTED_URL)
    message(FATAL_ERROR "JE local zstd: source URL/SHA256 must match the audited 1.3.5 commit archive")
endif()
JE_python_verify_file("${JE_ZSTD_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "zstd source archive")

set(JE_WORK "${JE_ZSTD_OUTPUT}/source-work/zstd-${JE_SOURCE_COMMIT}")
set(JE_BUILD "${JE_ZSTD_OUTPUT}/build")
set(JE_STAGE "${JE_ZSTD_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_ZSTD_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_ZSTD_SOURCE_ARCHIVE}" DESTINATION "${JE_ZSTD_OUTPUT}/source-work")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143 "-DJE_ZSTD_WORK:PATH=${JE_WORK}"
    RESULT_VARIABLE JE_CONFIGURE_RESULT)
if(NOT JE_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local zstd configure failed (${JE_CONFIGURE_RESULT}); outputs retained")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_BUILD}" --target libzstd_static --config Release
    RESULT_VARIABLE JE_BUILD_RESULT)
if(NOT JE_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local zstd build failed (${JE_BUILD_RESULT}); outputs retained")
endif()
set(JE_LIBRARY "${JE_BUILD}/lib/release/zstd_static.lib")
if(NOT EXISTS "${JE_LIBRARY}")
    message(FATAL_ERROR "JE local zstd: expected Release archive is missing")
endif()

file(MAKE_DIRECTORY "${JE_STAGE}/zstd/include" "${JE_STAGE}/zstd/lib/release" "${JE_STAGE}/LICENSES/Zstd")
file(COPY "${JE_LIBRARY}" DESTINATION "${JE_STAGE}/zstd/lib/release")
file(COPY "${JE_WORK}/lib/zstd.h" "${JE_WORK}/lib/common/zstd_errors.h"
    "${JE_WORK}/lib/deprecated/zbuff.h" "${JE_WORK}/lib/dictBuilder/zdict.h"
    DESTINATION "${JE_STAGE}/zstd/include")
file(COPY "${JE_WORK}/LICENSE" "${JE_WORK}/COPYING" DESTINATION "${JE_STAGE}/LICENSES/Zstd")
# Complete library source/header notices preserve attribution and embedded licences,
# including BSD2 entropy/xxHash, MIT divsufsort, and the legacy decoder notices.
file(GLOB_RECURSE JE_LIBRARY_NOTICES LIST_DIRECTORIES FALSE RELATIVE "${JE_WORK}/lib"
    "${JE_WORK}/lib/*.c" "${JE_WORK}/lib/*.h")
foreach(JE_NOTICE IN LISTS JE_LIBRARY_NOTICES)
    configure_file("${JE_WORK}/lib/${JE_NOTICE}" "${JE_STAGE}/LICENSES/Zstd/library/${JE_NOTICE}" COPYONLY)
endforeach()
configure_file("${CMAKE_CURRENT_LIST_DIR}/Findzstd.cmake" "${JE_STAGE}/Findzstd.cmake" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake"
    "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" "${CMAKE_CURRENT_LIST_DIR}/README.md" DESTINATION "${JE_STAGE}")
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(WRITE "${JE_STAGE}/zstd/JERecipe.json"
    "{\n  \"sourceVersion\": \"1.3.5\", \"toolset\": \"v143\",\n"
    "  \"generator\": \"Visual Studio 17 2022\", \"architecture\": \"x64\",\n"
    "  \"configuration\": \"Release\", \"runtimeLibrary\": \"MultiThreadedDLL\",\n"
    "  \"legacySupport\": 4, \"multithread\": true, \"static\": true,\n"
    "  \"shared\": false, \"programs\": false, \"tests\": false, \"contrib\": false,\n"
    "  \"policyCompatibilityMinimum\": \"3.5\",\n"
    "  \"historicalBinaryCompatibility\": \"not asserted\"\n}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "zstd-1.3.5-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_ZSTD_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "1.3.5")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_LICENSE "BSD-3-Clause AND BSD-2-Clause AND MIT")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_EXPECTED_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
