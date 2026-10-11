# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE SOURCE_SHA256 SOURCE_URL OUTPUT)
    if(NOT DEFINED JE_EXPAT_${JE_ARGUMENT} OR "${JE_EXPAT_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local Expat: JE_EXPAT_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_EXPAT_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_EXPAT_OUTPUT)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_EXPAT_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_EXPAT_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local Expat: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local Expat requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_EXPAT_OUTPUT}")
    message(FATAL_ERROR "JE local Expat: output already exists; retained outputs are never overwritten")
endif()
set(JE_SOURCE_COMMIT "e26a263aa9182d1e4552cd615857f0f61df3ba42")
set(JE_EXPECTED_HASH "a6130cab18d9df124dbfe4045f57f8c2180811c35d37bc357a42cff61afa4057")
set(JE_EXPECTED_URL "https://codeload.github.com/libexpat/libexpat/tar.gz/${JE_SOURCE_COMMIT}")
string(TOLOWER "${JE_EXPAT_SOURCE_SHA256}" JE_SOURCE_HASH)
if(NOT JE_SOURCE_HASH STREQUAL JE_EXPECTED_HASH OR NOT JE_EXPAT_SOURCE_URL STREQUAL JE_EXPECTED_URL)
    message(FATAL_ERROR "JE local Expat: source URL/SHA256 must match the audited R_2_4_2 commit archive")
endif()
JE_python_verify_file("${JE_EXPAT_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "Expat source archive")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/expat_2.4.2.patch"
    "685f13076bd02b2323ab756adfdb4ad57bf229888ec793c503efb378ca51d483" "original Expat recipe patch")

set(JE_ARCHIVE_ROOT "libexpat-${JE_SOURCE_COMMIT}")
set(JE_WORK "${JE_EXPAT_OUTPUT}/source-work/${JE_ARCHIVE_ROOT}/expat")
set(JE_BUILD "${JE_EXPAT_OUTPUT}/build")
set(JE_STAGE "${JE_EXPAT_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_EXPAT_OUTPUT}/source-work")
# The pinned archive's sole symlink is root/README.md -> expat/README.md.
# Extract only the audited expat subtree (127 regular files, zero links).
file(ARCHIVE_EXTRACT INPUT "${JE_EXPAT_SOURCE_ARCHIVE}" DESTINATION "${JE_EXPAT_OUTPUT}/source-work"
    PATTERNS "${JE_ARCHIVE_ROOT}/expat/*")
JE_python_verify_file("${JE_WORK}/CMakeLists.txt"
    "81c57f446bc10dd184c4817f107f3a15f86eea0f270eec7f8d164f484f6d655b" "pristine Expat CMake source")
file(READ "${JE_WORK}/CMakeLists.txt" JE_CMAKE_SOURCE)
string(REGEX MATCHALL "cmake_minimum_required\\(VERSION 3\\.1\\.3\\)" JE_HUNKS "${JE_CMAKE_SOURCE}")
list(LENGTH JE_HUNKS JE_HUNK_COUNT)
if(NOT JE_HUNK_COUNT EQUAL 1)
    message(FATAL_ERROR "JE local Expat: original minimum-version patch hunk must occur exactly once")
endif()
string(REPLACE "cmake_minimum_required(VERSION 3.1.3)" "cmake_minimum_required(VERSION 3.8.0)"
    JE_CMAKE_PATCHED "${JE_CMAKE_SOURCE}")
# Preserve upstream LF bytes; Windows file(WRITE) changes the raw patch hash.
file(CONFIGURE OUTPUT "${JE_WORK}/CMakeLists.txt" CONTENT "${JE_CMAKE_PATCHED}" @ONLY NEWLINE_STYLE UNIX)
JE_python_verify_file("${JE_WORK}/CMakeLists.txt"
    "d2f6e71b0209468048cec60c9981be7367b911d99517dd0b3b76da91dc489203" "patched Expat CMake source")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143 "-DJE_EXPAT_WORK:PATH=${JE_WORK}"
    RESULT_VARIABLE JE_CONFIGURE_RESULT)
if(NOT JE_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local Expat configure failed (${JE_CONFIGURE_RESULT}); outputs retained")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_BUILD}" --target expat --config Release
    RESULT_VARIABLE JE_BUILD_RESULT)
if(NOT JE_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local Expat build failed (${JE_BUILD_RESULT}); outputs retained")
endif()
if(NOT EXISTS "${JE_BUILD}/lib/libexpatMD.lib" OR NOT EXISTS "${JE_BUILD}/upstream/expat_config.h")
    message(FATAL_ERROR "JE local Expat: expected Release archive/generated header is missing")
endif()
file(MAKE_DIRECTORY "${JE_STAGE}/expat/include" "${JE_STAGE}/expat/lib" "${JE_STAGE}/LICENSES/Expat")
file(COPY "${JE_WORK}/lib/expat.h" "${JE_WORK}/lib/expat_external.h" "${JE_BUILD}/upstream/expat_config.h"
    DESTINATION "${JE_STAGE}/expat/include")
file(COPY "${JE_BUILD}/lib/libexpatMD.lib" DESTINATION "${JE_STAGE}/expat/lib")
configure_file("${JE_WORK}/COPYING" "${JE_STAGE}/expat/COPYING" COPYONLY)
configure_file("${JE_WORK}/COPYING" "${JE_STAGE}/LICENSES/Expat/COPYING" COPYONLY)
# Preserve all compiled/include copyright notices, including SipHash's CC0 notice.
file(GLOB JE_SOURCE_NOTICES "${JE_WORK}/lib/*.c" "${JE_WORK}/lib/*.h")
file(COPY ${JE_SOURCE_NOTICES} DESTINATION "${JE_STAGE}/LICENSES/Expat/library")
configure_file("${JE_WORK}/CMakeLists.txt" "${JE_STAGE}/LICENSES/Expat/patched-CMakeLists.txt" COPYONLY)
configure_file("${JE_WORK}/ConfigureChecks.cmake" "${JE_STAGE}/LICENSES/Expat/ConfigureChecks.cmake" COPYONLY)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/Findexpat.cmake" "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt"
    "${CMAKE_CURRENT_LIST_DIR}/README.md" DESTINATION "${JE_STAGE}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake"
    "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" "${CMAKE_CURRENT_LIST_DIR}/expat_2.4.2.patch"
    DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(WRITE "${JE_STAGE}/expat/JERecipe.json"
    "{\n  \"recipeCommit\": \"70f0b7b5257bf7cf935bf1cfd270e1b53ccead18\",\n"
    "  \"originalPackageVersion\": \"2.4.2-rev2\", \"sourceTag\": \"R_2_4_2\",\n"
    "  \"patchSHA256\": \"685f13076bd02b2323ab756adfdb4ad57bf229888ec793c503efb378ca51d483\",\n"
    "  \"patchedCMakeSHA256\": \"d2f6e71b0209468048cec60c9981be7367b911d99517dd0b3b76da91dc489203\",\n"
    "  \"originalGenerator\": \"Visual Studio 16 2019\", \"toolset\": \"v143\",\n"
    "  \"configuration\": \"Release\", \"runtimeLibrary\": \"MultiThreadedDLL\",\n"
    "  \"static\": true, \"charType\": \"char\", \"dtd\": true, \"namespace\": true,\n"
    "  \"contextBytes\": 1024, \"largeSize\": false, \"tools\": false,\n"
    "  \"omittedArchiveLink\": \"README.md -> expat/README.md\",\n"
    "  \"historicalBinaryCompatibility\": \"not asserted\"\n}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "expat-2.4.2-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_EXPAT_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "2.4.2")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_LICENSE "MIT AND CC0-1.0")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_EXPECTED_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
