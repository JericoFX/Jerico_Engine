# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Offline build from the exact audited source archive; no global install.
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_PATH IN ITEMS JE_SAMPLERATE_SOURCE_ARCHIVE JE_SAMPLERATE_SOURCE_MANIFEST JE_SAMPLERATE_OUTPUT)
    JE_python_require_local_path(${JE_PATH})
endforeach()
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_SAMPLERATE_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_SAMPLERATE_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local libsamplerate: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local libsamplerate requires Windows MSVC x64")
endif()
if(EXISTS "${JE_SAMPLERATE_OUTPUT}")
    message(FATAL_ERROR "JE local libsamplerate: output must be new; existing outputs are preserved")
endif()
set(JE_SOURCE_COMMIT "a147b6dc4046deda22b2aa772b348717de772e2b")
set(JE_SOURCE_SHA256 "f90e46bbf9bae05139e297ceebd5394ef77ce5e294e6b46beffcc15b9446a041")
set(JE_SOURCE_URL "https://codeload.github.com/libsndfile/libsamplerate/tar.gz/${JE_SOURCE_COMMIT}")
JE_python_verify_file("${JE_SAMPLERATE_SOURCE_ARCHIVE}" "${JE_SOURCE_SHA256}" "libsamplerate source archive")
JE_python_verify_file("${JE_SAMPLERATE_SOURCE_MANIFEST}"
    "35d7d9f0a5ca3f563be8294b2d9cbc43a89b520dfe88bccdd40fa2dd6ee12ff0" "libsamplerate source manifest")
# Both inputs are pinned before creating any output. The audited archive has
# no links or path collisions; the immutable snapshot is never a build tree.
file(READ "${JE_SAMPLERATE_SOURCE_MANIFEST}" JE_MANIFEST)
string(JSON JE_SOURCE_COUNT GET "${JE_MANIFEST}" count)
if(NOT JE_SOURCE_COUNT EQUAL 99)
    message(FATAL_ERROR "JE local libsamplerate: unexpected source manifest count")
endif()
set(JE_WORK "${JE_SAMPLERATE_OUTPUT}/source-work/libsamplerate-${JE_SOURCE_COMMIT}")
set(JE_BUILD "${JE_SAMPLERATE_OUTPUT}/build")
set(JE_STAGE "${JE_SAMPLERATE_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_SAMPLERATE_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_SAMPLERATE_SOURCE_ARCHIVE}" DESTINATION "${JE_SAMPLERATE_OUTPUT}/source-work")
math(EXPR JE_SOURCE_LAST "${JE_SOURCE_COUNT} - 1")
foreach(JE_INDEX RANGE 0 ${JE_SOURCE_LAST})
    string(JSON JE_RELATIVE GET "${JE_MANIFEST}" entries ${JE_INDEX} path)
    string(JSON JE_HASH GET "${JE_MANIFEST}" entries ${JE_INDEX} sha256)
    string(JSON JE_SIZE GET "${JE_MANIFEST}" entries ${JE_INDEX} bytes)
    JE_python_verify_file("${JE_WORK}/${JE_RELATIVE}" "${JE_HASH}" "libsamplerate extracted source")
    file(SIZE "${JE_WORK}/${JE_RELATIVE}" JE_ACTUAL_SIZE)
    if(NOT JE_ACTUAL_SIZE EQUAL JE_SIZE)
        message(FATAL_ERROR "JE local libsamplerate: extracted source size differs")
    endif()
endforeach()
file(STRINGS "${JE_WORK}/CMakeLists.txt" JE_VERSION_LINE REGEX "^project\\(libsamplerate VERSION ")
if(NOT JE_VERSION_LINE STREQUAL "project(libsamplerate VERSION 0.2.1 LANGUAGES C)")
    message(FATAL_ERROR "JE local libsamplerate: source version is not 0.2.1")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/ApplySamplerateChannelGuard.cmake")

function(JE_samplerate_native JE_LABEL)
    file(WRITE "${JE_SAMPLERATE_OUTPUT}/${JE_LABEL}-argv.txt" "")
    foreach(JE_ARGUMENT IN LISTS ARGN)
        file(APPEND "${JE_SAMPLERATE_OUTPUT}/${JE_LABEL}-argv.txt" "${JE_ARGUMENT}\n")
    endforeach()
    string(TIMESTAMP JE_START "%Y-%m-%dT%H:%M:%SZ" UTC)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE JE_RESULT
        OUTPUT_FILE "${JE_SAMPLERATE_OUTPUT}/${JE_LABEL}-stdout.log"
        ERROR_FILE "${JE_SAMPLERATE_OUTPUT}/${JE_LABEL}-stderr.log")
    string(TIMESTAMP JE_END "%Y-%m-%dT%H:%M:%SZ" UTC)
    file(WRITE "${JE_SAMPLERATE_OUTPUT}/${JE_LABEL}-result.txt" "start=${JE_START}\nend=${JE_END}\nresult=${JE_RESULT}\n")
    if(NOT JE_RESULT STREQUAL "0")
        message(FATAL_ERROR "JE local libsamplerate ${JE_LABEL} failed (${JE_RESULT}); outputs retained")
    endif()
endfunction()
# Upstream sets C99 and respects CMP0091 when the CRT is explicit. Preserve
# every converter; tests/examples alone introduce the optional SndFile search.
JE_samplerate_native(configure "${CMAKE_COMMAND}" -S "${JE_WORK}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF
    -DBUILD_TESTING=OFF -DLIBSAMPLERATE_EXAMPLES=OFF -DLIBSAMPLERATE_INSTALL=OFF
    -DLIBSAMPLERATE_ENABLE_SINC_FAST_CONVERTER=ON
    -DLIBSAMPLERATE_ENABLE_SINC_MEDIUM_CONVERTER=ON
    -DLIBSAMPLERATE_ENABLE_SINC_BEST_CONVERTER=ON)
JE_samplerate_native(build-Release "${CMAKE_COMMAND}" --build "${JE_BUILD}" --config Release --target samplerate)
if(NOT EXISTS "${JE_BUILD}/src/Release/samplerate.lib")
    message(FATAL_ERROR "JE local libsamplerate: expected Release static library is missing")
endif()
file(MAKE_DIRECTORY "${JE_STAGE}/libsamplerate/include" "${JE_STAGE}/libsamplerate/lib"
    "${JE_STAGE}/LICENSES/libsamplerate" "${JE_STAGE}/LICENSES/O3DE")
file(COPY "${JE_WORK}/include/samplerate.h" DESTINATION "${JE_STAGE}/libsamplerate/include")
file(COPY "${JE_BUILD}/src/Release/samplerate.lib" DESTINATION "${JE_STAGE}/libsamplerate/lib")
file(COPY "${JE_WORK}/COPYING" DESTINATION "${JE_STAGE}/libsamplerate")
file(COPY "${JE_WORK}/COPYING" DESTINATION "${JE_STAGE}/LICENSES/libsamplerate")
# Preserve full compiled-library/header notices. GPL check_asm.sh is an
# unselected source inspection helper, not compiled or shipped in this SDK.
file(GLOB JE_NOTICE_SOURCES "${JE_WORK}/src/*.c" "${JE_WORK}/src/*.h")
file(COPY ${JE_NOTICE_SOURCES} DESTINATION "${JE_STAGE}/LICENSES/libsamplerate/source-notices")
file(COPY "${JE_WORK}/include/samplerate.h" DESTINATION "${JE_STAGE}/LICENSES/libsamplerate/source-notices")
file(COPY "${JE_SAMPLERATE_OUTPUT}/patch-provenance/samplerate.c.original"
    "${CMAKE_CURRENT_LIST_DIR}/SamplerateChannelGuard.patch"
    "${CMAKE_CURRENT_LIST_DIR}/SamplerateChannelGuard.json"
    DESTINATION "${JE_STAGE}/LICENSES/libsamplerate/patch-provenance")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/Findlibsamplerate.cmake" "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt"
    "${CMAKE_CURRENT_LIST_DIR}/README.md" DESTINATION "${JE_STAGE}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake" "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
configure_file("${JE_SAMPLERATE_SOURCE_MANIFEST}" "${JE_STAGE}/LICENSES/libsamplerate/SourceManifest.json" COPYONLY)
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(WRITE "${JE_STAGE}/libsamplerate/JERecipe.json" [=[
{
  "version": "0.2.1", "originalRecipe": "0.2.1-rev2",
  "recipeCommit": "70f0b7b5257bf7cf935bf1cfd270e1b53ccead18",
  "generator": "Visual Studio 17 2022", "toolset": "v143", "architecture": "x64",
  "configuration": "Release", "language": "C99", "CRT": "/MD",
  "BUILD_SHARED_LIBS": false, "BUILD_TESTING": false,
  "LIBSAMPLERATE_EXAMPLES": false, "LIBSAMPLERATE_INSTALL": false,
  "SINC_FAST": true, "SINC_MEDIUM": true, "SINC_BEST": true,
  "sourcePatches": ["LICENSES/libsamplerate/patch-provenance/SamplerateChannelGuard.json"], "legacyBinaryABI": "not asserted",
  "DebugConsumer": "/MDd against Release /MD; bounded testing only, no general CRT compatibility claim"
}
]=])
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "libsamplerate-0.2.1-je-rev2-windows")
set(JE_PACKAGE_OUTPUT "${JE_SAMPLERATE_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "0.2.1")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_SHA256}")
set(JE_PACKAGE_SOURCE_URL "${JE_SOURCE_URL}")
set(JE_PACKAGE_LICENSE "BSD-2-Clause")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
