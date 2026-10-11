# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Build only the explicitly audited local source archive. No acquisition or global install.
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE SOURCE_SHA256 SOURCE_URL OUTPUT)
    if(NOT DEFINED JE_LZ4_${JE_ARGUMENT} OR "${JE_LZ4_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local lz4: JE_LZ4_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_LZ4_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_LZ4_OUTPUT)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_LZ4_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_LZ4_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local lz4: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local lz4 requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_LZ4_OUTPUT}")
    message(FATAL_ERROR "JE local lz4: output must be new; existing outputs are preserved")
endif()
set(JE_SOURCE_COMMIT "5ff839680134437dbf4678f3d0c7b371d84f4964")
set(JE_EXPECTED_HASH "0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b")
set(JE_EXPECTED_URL "https://codeload.github.com/lz4/lz4/tar.gz/refs/tags/v1.9.4")
string(TOLOWER "${JE_LZ4_SOURCE_SHA256}" JE_SOURCE_HASH)
if(NOT JE_SOURCE_HASH STREQUAL JE_EXPECTED_HASH OR NOT JE_LZ4_SOURCE_URL STREQUAL JE_EXPECTED_URL)
    message(FATAL_ERROR "JE local lz4: source URL/SHA256 must match the audited v1.9.4 archive")
endif()
JE_python_verify_file("${JE_LZ4_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "lz4 source archive")

set(JE_WORK "${JE_LZ4_OUTPUT}/source-work/lz4-1.9.4")
set(JE_BUILD "${JE_LZ4_OUTPUT}/build")
set(JE_STAGE "${JE_LZ4_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_LZ4_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_LZ4_SOURCE_ARCHIVE}" DESTINATION "${JE_LZ4_OUTPUT}/source-work")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143 "-DJE_LZ4_WORK:PATH=${JE_WORK}"
    RESULT_VARIABLE JE_CONFIGURE_RESULT)
if(NOT JE_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local lz4 configure failed (${JE_CONFIGURE_RESULT}); outputs retained")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_BUILD}" --target lz4_static --config Release
    RESULT_VARIABLE JE_BUILD_RESULT)
if(NOT JE_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local lz4 build failed (${JE_BUILD_RESULT}); outputs retained")
endif()
set(JE_LIBRARY "${JE_BUILD}/lib/lz4.lib")
if(NOT EXISTS "${JE_LIBRARY}")
    message(FATAL_ERROR "JE local lz4: expected Release archive is missing")
endif()
file(MAKE_DIRECTORY "${JE_STAGE}/lz4/include" "${JE_STAGE}/lz4/lib" "${JE_STAGE}/LICENSES/LZ4")
file(COPY "${JE_LIBRARY}" DESTINATION "${JE_STAGE}/lz4/lib")
file(COPY "${JE_WORK}/lib/lz4.h" "${JE_WORK}/lib/lz4hc.h" "${JE_WORK}/lib/lz4frame.h"
    DESTINATION "${JE_STAGE}/lz4/include")
configure_file("${JE_WORK}/lib/LICENSE" "${JE_STAGE}/LICENSES/LZ4/LICENSE" COPYONLY)
configure_file("${JE_WORK}/LICENSE" "${JE_STAGE}/LICENSES/LZ4/UPSTREAM-LICENSE-SCOPE.txt" COPYONLY)
# Preserve each compiled library/header notice, including xxHash's BSD2 attribution.
foreach(JE_NOTICE IN ITEMS lz4.c lz4.h lz4hc.c lz4hc.h lz4frame.c lz4frame.h lz4frame_static.h xxhash.c xxhash.h)
    configure_file("${JE_WORK}/lib/${JE_NOTICE}" "${JE_STAGE}/LICENSES/LZ4/library/${JE_NOTICE}" COPYONLY)
endforeach()
# Upstream CMake is CC0; preserve its complete original notice as build provenance.
configure_file("${JE_WORK}/build/cmake/CMakeLists.txt" "${JE_STAGE}/LICENSES/LZ4/upstream-CMakeLists.txt" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/Findlz4.cmake" "${JE_STAGE}/Findlz4.cmake" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake"
    "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" "${CMAKE_CURRENT_LIST_DIR}/README.md" DESTINATION "${JE_STAGE}")
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(WRITE "${JE_STAGE}/lz4/JERecipe.json"
    "{\n  \"sourceVersion\": \"1.9.4\", \"toolset\": \"v143\",\n"
    "  \"generator\": \"Visual Studio 17 2022\", \"architecture\": \"x64\",\n"
    "  \"configuration\": \"Release\", \"runtimeLibrary\": \"MultiThreadedDLL\",\n"
    "  \"static\": true, \"shared\": false, \"core\": true, \"highCompression\": true,\n"
    "  \"frame\": true, \"xxHash\": true, \"cli\": false, \"legacyCli\": false,\n"
    "  \"sourcePatched\": false, \"policyCompatibilityMinimum\": \"3.5\",\n"
    "  \"historicalRecipeCommit\": \"68e97674712afd8c5e5c99608b35442da579163a\",\n"
    "  \"historicalBinaryCompatibility\": \"not asserted\"\n}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "lz4-1.9.4-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_LZ4_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "1.9.4")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_LICENSE "BSD-2-Clause")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_EXPECTED_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
