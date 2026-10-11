# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Exact libpng 1.6.37 source recipe; no acquisition or global install.
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE OUTPUT)
    if(NOT DEFINED JE_PNG_${JE_ARGUMENT} OR "${JE_PNG_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local PNG: JE_PNG_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_PNG_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_PNG_OUTPUT)
include("${CMAKE_CURRENT_LIST_DIR}/VerifyZlibInput.cmake")
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_PNG_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_PNG_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local PNG: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local PNG requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_PNG_OUTPUT}")
    message(FATAL_ERROR "JE local PNG: output must be new; existing outputs are preserved")
endif()
set(JE_SOURCE_COMMIT "a40189cf881e9f0db80511c382292a5604c3c3d1")
set(JE_SOURCE_HASH "ed747514984fddf095142aa85e983111f13d64b877b1065ed0d990f7adb31eb7")
set(JE_SOURCE_URL "https://codeload.github.com/pnggroup/libpng/tar.gz/${JE_SOURCE_COMMIT}")
JE_python_verify_file("${JE_PNG_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "audited v1.6.37 commit archive")
set(JE_WORK "${JE_PNG_OUTPUT}/source-work/libpng-${JE_SOURCE_COMMIT}")
set(JE_BUILD "${JE_PNG_OUTPUT}/build")
set(JE_STAGE "${JE_PNG_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_PNG_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_PNG_SOURCE_ARCHIVE}" DESTINATION "${JE_PNG_OUTPUT}/source-work")
file(STRINGS "${JE_WORK}/png.h" JE_VERSION_LINE REGEX "^#define PNG_LIBPNG_VER_STRING ")
if(NOT JE_VERSION_LINE STREQUAL "#define PNG_LIBPNG_VER_STRING \"1.6.37\"")
    message(FATAL_ERROR "JE local PNG: source is not libpng 1.6.37")
endif()
JE_python_verify_file("${JE_WORK}/scripts/pnglibconf.h.prebuilt"
    "3fc519a430e8f5be8b3962483e054373536ceae432168ce171b5c5e19b4694a3" "official full prebuilt PNG configuration")
# Original Windows options and no feature removals. The explicit upstream
# AWK fallback uses its own full configuration without an external tool search.
# Policy compatibility is confined to this child configure, not engine policy.
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${JE_WORK}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL
    -DPNG_SHARED=OFF -DPNG_TESTS=OFF -DPNG_DEBUG=OFF -DPNG_STATIC=ON
    -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE -DCMAKE_CXX_STANDARD=17
    -DBUILD_SHARED_LIBS=OFF -DPNG_ARM_NEON=on -DAWK:FILEPATH=AWK-NOTFOUND
    "-DCMAKE_MODULE_PATH:PATH=${JE_ZLIB_PACKAGE_ROOT}"
    RESULT_VARIABLE JE_CONFIGURE_RESULT)
if(NOT JE_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local PNG configure failed (${JE_CONFIGURE_RESULT}); outputs retained")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_BUILD}" --target png_static --config Release
    RESULT_VARIABLE JE_BUILD_RESULT)
if(NOT JE_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local PNG build failed (${JE_BUILD_RESULT}); outputs retained")
endif()
if(NOT EXISTS "${JE_BUILD}/Release/libpng16_static.lib")
    message(FATAL_ERROR "JE local PNG: expected Release static library is missing")
endif()
# Compare text because configure_file may use Windows newlines; preserve the
# generated header in the payload exactly as compiled, without rewriting it.
file(READ "${JE_WORK}/scripts/pnglibconf.h.prebuilt" JE_CONF_SOURCE)
file(READ "${JE_BUILD}/pnglibconf.h" JE_CONF_GENERATED)
string(REPLACE "\r\n" "\n" JE_CONF_SOURCE "${JE_CONF_SOURCE}")
string(REPLACE "\r\n" "\n" JE_CONF_GENERATED "${JE_CONF_GENERATED}")
if(NOT JE_CONF_SOURCE STREQUAL JE_CONF_GENERATED)
    message(FATAL_ERROR "JE local PNG: generated fallback configuration differs from audited upstream")
endif()
file(MAKE_DIRECTORY "${JE_STAGE}/png/include" "${JE_STAGE}/png/lib" "${JE_STAGE}/LICENSES/PNG")
file(COPY "${JE_WORK}/png.h" "${JE_WORK}/pngconf.h" "${JE_BUILD}/pnglibconf.h"
    DESTINATION "${JE_STAGE}/png/include")
file(COPY "${JE_BUILD}/Release/libpng16_static.lib" DESTINATION "${JE_STAGE}/png/lib")
file(COPY "${JE_WORK}/LICENSE" DESTINATION "${JE_STAGE}/png")
file(COPY "${JE_WORK}/LICENSE" "${JE_WORK}/AUTHORS" "${JE_WORK}/TRADEMARK"
    DESTINATION "${JE_STAGE}/LICENSES/PNG")
# Preserve all library source/header notices, including optional architecture
# implementations, while leaving contrib/test executables outside the payload.
file(GLOB JE_LIBRARY_NOTICES RELATIVE "${JE_WORK}" "${JE_WORK}/png*.c" "${JE_WORK}/png*.h")
foreach(JE_ARCH IN ITEMS arm intel mips powerpc)
    file(GLOB_RECURSE JE_ARCH_NOTICES RELATIVE "${JE_WORK}" LIST_DIRECTORIES FALSE
        "${JE_WORK}/${JE_ARCH}/*.c" "${JE_WORK}/${JE_ARCH}/*.h" "${JE_WORK}/${JE_ARCH}/*.S")
    list(APPEND JE_LIBRARY_NOTICES ${JE_ARCH_NOTICES})
endforeach()
list(APPEND JE_LIBRARY_NOTICES scripts/pnglibconf.h.prebuilt)
foreach(JE_NOTICE IN LISTS JE_LIBRARY_NOTICES)
    configure_file("${JE_WORK}/${JE_NOTICE}" "${JE_STAGE}/LICENSES/PNG/source-notices/${JE_NOTICE}" COPYONLY)
endforeach()
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake"
    "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/FindPNG.cmake" "${CMAKE_CURRENT_LIST_DIR}/README.md"
    "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" DESTINATION "${JE_STAGE}")
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(WRITE "${JE_STAGE}/png/JERecipe.json" [=[
{
  "upstreamVersion": "1.6.37", "originalPackageVersion": "1.6.37-rev2",
  "recipeCommit": "70f0b7b5257bf7cf935bf1cfd270e1b53ccead18",
  "generator": "Visual Studio 17 2022", "toolset": "v143", "architecture": "x64",
  "configuration": "Release", "runtimeLibrary": "MultiThreadedDLL",
  "PNG_SHARED": false, "PNG_TESTS": false, "PNG_DEBUG": false, "PNG_STATIC": true,
  "PIC": true, "PNG_ARM_NEON": "on (upstream applies only to ARM)",
  "configurationHeader": "upstream scripts/pnglibconf.h.prebuilt via explicit AWK-NOTFOUND fallback",
  "policyCompatibilityMinimum": "3.5 (producer configure only)", "zlib": "1.2.11",
  "sourcePatches": [], "historicalBinaryCompatibility": "not asserted"
}
]=])
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "png-1.6.37-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_PNG_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "1.6.37")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_LICENSE "libpng-2.0")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_SOURCE_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")