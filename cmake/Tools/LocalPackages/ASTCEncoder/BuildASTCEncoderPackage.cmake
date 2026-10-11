# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE SOURCE_SHA256 SOURCE_URL OUTPUT)
    if(NOT DEFINED JE_ASTC_${JE_ARGUMENT} OR "${JE_ASTC_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local ASTC: JE_ASTC_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_ASTC_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_ASTC_OUTPUT)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_ASTC_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_ASTC_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local ASTC: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local ASTC requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_ASTC_OUTPUT}")
    message(FATAL_ERROR "JE local ASTC: output must be new; existing outputs are preserved")
endif()
set(JE_SOURCE_COMMIT "f32d8d4dd9e06f8986cd5e10f12e199fc023f150")
set(JE_EXPECTED_HASH "291c26fcf20b8e23a6eb1ad4dd75f5afecf79d316c0477c532df6d5693291403")
set(JE_EXPECTED_URL "https://codeload.github.com/ARM-software/astc-encoder/tar.gz/${JE_SOURCE_COMMIT}")
string(TOLOWER "${JE_ASTC_SOURCE_SHA256}" JE_SOURCE_HASH)
if(NOT JE_SOURCE_HASH STREQUAL JE_EXPECTED_HASH OR NOT JE_ASTC_SOURCE_URL STREQUAL JE_EXPECTED_URL)
    message(FATAL_ERROR "JE local ASTC: source URL/SHA256 must match the audited 3.2 archive")
endif()
JE_python_verify_file("${JE_ASTC_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "ASTC source archive")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/astc-encoder-3.2-rev1.patch" "88e0389289a28db8d937842d890a7ec4953d2295c263997236b05be4b3b5bd00" "original ASTC recipe patch")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/PatchHunks.json" "1d45acd2e7674fdb62afd8187c5ff7d41c72590cd359f84ce518cad5e3ab2dca" "ASTC patch manifest")

# Only the pinned, completely audited zero-link archive is extracted into fresh private work.
set(JE_ASTC_WORK "${JE_ASTC_OUTPUT}/source-work/astc-encoder-${JE_SOURCE_COMMIT}")
set(JE_BUILD "${JE_ASTC_OUTPUT}/build")
set(JE_STAGE "${JE_ASTC_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_ASTC_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_ASTC_SOURCE_ARCHIVE}" DESTINATION "${JE_ASTC_OUTPUT}/source-work")
JE_python_verify_file("${JE_ASTC_WORK}/CMakeLists.txt" "86f785318c024bbe740ca5ff05982c195a3e4a14a8dc01e663b2e245ea3c2a85" "ASTC source project")
JE_python_verify_file("${JE_ASTC_WORK}/Source/astcenc.h" "a19c5a0ba650eb7f61c24b29673eee78b0d6f72b5b17426cfe349bd9f82967aa" "ASTC 3.2 API header")
include("${CMAKE_CURRENT_LIST_DIR}/ApplyASTCPatch.cmake")

function(JE_astc_native JE_LABEL)
    file(WRITE "${JE_ASTC_OUTPUT}/${JE_LABEL}-argv.txt" "")
    foreach(JE_ARG IN LISTS ARGN)
        file(APPEND "${JE_ASTC_OUTPUT}/${JE_LABEL}-argv.txt" "${JE_ARG}\n")
    endforeach()
    string(TIMESTAMP JE_START "%Y-%m-%dT%H:%M:%SZ" UTC)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE JE_RESULT
        OUTPUT_FILE "${JE_ASTC_OUTPUT}/${JE_LABEL}-stdout.log" ERROR_FILE "${JE_ASTC_OUTPUT}/${JE_LABEL}-stderr.log")
    string(TIMESTAMP JE_END "%Y-%m-%dT%H:%M:%SZ" UTC)
    file(WRITE "${JE_ASTC_OUTPUT}/${JE_LABEL}-result.txt" "start=${JE_START}\nend=${JE_END}\nresult=${JE_RESULT}\n")
    if(NOT JE_RESULT STREQUAL "0")
        message(FATAL_ERROR "JE local ASTC ${JE_LABEL} failed (${JE_RESULT}); outputs retained, package not published")
    endif()
endfunction()
JE_astc_native(configure "${CMAKE_COMMAND}" -S "${JE_ASTC_WORK}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143
    -DISA_SSE41=ON -DISA_AVX2=OFF -DISA_SSE2=OFF -DISA_NEON=OFF -DISA_NONE=OFF -DISA_NATIVE=OFF
    -DDECOMPRESSOR=OFF -DCLI=ON -DDIAGNOSTICS=OFF -DUNITTEST=OFF -DBUILD_TESTING=OFF)
foreach(JE_CONFIG IN ITEMS Release Debug)
    JE_astc_native(build-${JE_CONFIG} "${CMAKE_COMMAND}" --build "${JE_BUILD}" --config ${JE_CONFIG}
        --target astcenc-sse4.1-static astcenc-sse4.1 --parallel 8)
    foreach(JE_FILE IN ITEMS astcenc-sse4.1-static.lib astcenc-sse4.1.exe)
        if(NOT EXISTS "${JE_BUILD}/Source/${JE_CONFIG}/${JE_FILE}")
            message(FATAL_ERROR "JE local ASTC: expected ${JE_CONFIG} artifact missing: ${JE_FILE}")
        endif()
    endforeach()
endforeach()
# Stage only after both configurations succeed. No global install, tools download or runtime deploy.
file(MAKE_DIRECTORY "${JE_STAGE}/astc-encoder/include" "${JE_STAGE}/LICENSES/ASTC" "${JE_STAGE}/LICENSES/O3DE")
foreach(JE_CONFIG IN ITEMS Release Debug)
    file(MAKE_DIRECTORY "${JE_STAGE}/astc-encoder/bin/${JE_CONFIG}")
    file(COPY "${JE_BUILD}/Source/${JE_CONFIG}/astcenc-sse4.1-static.lib" "${JE_BUILD}/Source/${JE_CONFIG}/astcenc-sse4.1.exe"
        DESTINATION "${JE_STAGE}/astc-encoder/bin/${JE_CONFIG}")
endforeach()
file(COPY "${JE_ASTC_WORK}/Source/astcenc.h" DESTINATION "${JE_STAGE}/astc-encoder/include")
configure_file("${JE_ASTC_WORK}/LICENSE.txt" "${JE_STAGE}/astc-encoder/LICENSE.txt" COPYONLY)
# Keep full original notices of compiled library/CLI code, including embedded miniz/OpenEXR/STB.
file(GLOB JE_NOTICE_SOURCES "${JE_ASTC_WORK}/Source/*.cpp" "${JE_ASTC_WORK}/Source/*.h")
file(COPY ${JE_NOTICE_SOURCES} DESTINATION "${JE_STAGE}/LICENSES/ASTC/source-notices")
file(COPY "${JE_ASTC_WORK}/LICENSE.txt" DESTINATION "${JE_STAGE}/LICENSES/ASTC")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/astc-encoder-3.2-rev1.patch" "${CMAKE_CURRENT_LIST_DIR}/PatchHunks.json"
    DESTINATION "${JE_STAGE}/LICENSES/O3DE/recipe")
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT" "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/Findastc-encoder.cmake" "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" "${CMAKE_CURRENT_LIST_DIR}/README.md" DESTINATION "${JE_STAGE}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake" "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(WRITE "${JE_STAGE}/astc-encoder/JERecipe.json"
    "{\n  \"sourceVersion\": \"3.2.0\", \"sourceCommit\": \"${JE_SOURCE_COMMIT}\",\n"
    "  \"historicalRecipeCommit\": \"90480ca41d15e1b20a28f90e2f996fd0e5a3a68c\",\n"
    "  \"historicalPackageVersion\": \"3.2-rev2\", \"historicalBinaryHashMatch\": false,\n"
    "  \"generator\": \"Visual Studio 17 2022\", \"toolset\": \"v143\", \"architecture\": \"x64\",\n"
    "  \"configurations\": [\"Release\",\"Debug\"], \"parallelJobs\": 8,\n"
    "  \"runtimeLibrary\": \"MultiThreaded$<$<CONFIG:Debug>:Debug>DLL\",\n"
    "  \"isa\": \"SSE4.1\", \"fullCodec\": true, \"cliDebugTool\": true, \"unitTests\": false,\n"
    "  \"patchSHA256\": \"88e0389289a28db8d937842d890a7ec4953d2295c263997236b05be4b3b5bd00\",\n"
    "  \"patchedSourceCMakeSHA256\": \"4e425cff86bb25deb666ec8f8135e607c8833c32ea08f16b6b6f5839d6420fd7\",\n"
    "  \"patchedCoreCMakeSHA256\": \"eecc5233bedd1d2e6340bb7edc98d75474019a24df52eb912f7db3301292b80f\"\n}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "astc-encoder-3.2-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_ASTC_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "3.2.0")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_EXPECTED_URL}")
set(JE_PACKAGE_LICENSE "Apache-2.0 AND MIT AND BSD-3-Clause AND Unlicense")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
