# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE SOURCE_SHA256 SOURCE_URL OUTPUT)
    if(NOT DEFINED JE_MCPP_${JE_ARGUMENT} OR "${JE_MCPP_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local mcpp: JE_MCPP_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_MCPP_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_MCPP_OUTPUT)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_MCPP_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_MCPP_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local mcpp: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local mcpp requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_MCPP_OUTPUT}")
    message(FATAL_ERROR "JE local mcpp: output already exists: ${JE_MCPP_OUTPUT}")
endif()
set(JE_EXPECTED_HASH "3b9b4421888519876c4fc68ade324a3bbd81ceeb7092ecdbbc2055099fcb8864")
string(TOLOWER "${JE_MCPP_SOURCE_SHA256}" JE_SOURCE_HASH)
if(NOT JE_SOURCE_HASH STREQUAL JE_EXPECTED_HASH
    OR NOT JE_MCPP_SOURCE_URL STREQUAL "https://sourceforge.net/projects/mcpp/files/mcpp/V.2.7.2/mcpp-2.7.2.tar.gz/download")
    message(FATAL_ERROR "JE local mcpp: source URL/SHA256 must match the audited mcpp-2.7.2 release archive")
endif()
JE_python_verify_file("${JE_MCPP_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "mcpp source archive")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/mcpp_2.7.2_az.patch"
    "8797760af297be90586f03ae69c3ffaeb89591fa69199c4242e05f4df6efdc6b" "original mcpp _az.2 patch")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/McppPatchManifest.json"
    "9f8522a8a06ab9c4207f5f5ee7e7b41837831118512b07b0a8507bb5997aaec3" "mcpp exact patch manifest")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/visualc.mak.original"
    "a35dc1b5c93acf904ebbad69065125ff93f1b2905d822ae518aa6593fb000c7f" "original Windows recipe")

set(JE_MCPP_WORK "${JE_MCPP_OUTPUT}/source-work/mcpp-2.7.2")
set(JE_BUILD "${JE_MCPP_OUTPUT}/build")
set(JE_STAGE "${JE_MCPP_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_MCPP_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_MCPP_SOURCE_ARCHIVE}" DESTINATION "${JE_MCPP_OUTPUT}/source-work")
foreach(JE_FILE IN ITEMS LICENSE README src/main.c src/directive.c src/eval.c src/expand.c
    src/support.c src/system.c src/mbchar.c src/system.H src/internal.H)
    if(NOT EXISTS "${JE_MCPP_WORK}/${JE_FILE}")
        message(FATAL_ERROR "JE local mcpp: pinned source file missing: ${JE_FILE}")
    endif()
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/ApplyMcppPatch.cmake")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/ApplyMcppPreopenExtension.cmake" "9BA1CA73649710102D8FB5EB3C133BD531382309CAC4BDDF164D76295716A9A4" "pre-open apply layer")
include("${CMAKE_CURRENT_LIST_DIR}/ApplyMcppPreopenExtension.cmake")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143 "-DJE_MCPP_WORK:PATH=${JE_MCPP_WORK}"
    RESULT_VARIABLE JE_CONFIGURE_RESULT)
if(NOT JE_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local mcpp configure failed (${JE_CONFIGURE_RESULT}); outputs retained")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_BUILD}" --config Release
    --target JEMcppDll JEMcppCli --parallel 1 -- /m:1 /nr:false
    /p:UseMultiToolTask=true /p:MultiProcCL=true /p:MultiProcMaxCount=1
    /p:EnableClServerMode=false /p:EnforceProcessCountAcrossBuilds=true /p:CL_MPCount=1
    RESULT_VARIABLE JE_BUILD_RESULT)
if(NOT JE_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local mcpp build failed (${JE_BUILD_RESULT}); outputs retained")
endif()
file(MAKE_DIRECTORY "${JE_STAGE}/mcpp/include" "${JE_STAGE}/mcpp/lib"
    "${JE_STAGE}/LICENSES/mcpp" "${JE_STAGE}/LICENSES/O3DE")
file(COPY "${JE_MCPP_WORK}/src/mcpp_lib.h" "${JE_MCPP_WORK}/src/mcpp_out.h"
    DESTINATION "${JE_STAGE}/mcpp/include")
file(COPY "${JE_BUILD}/Release/mcpp0.dll" "${JE_BUILD}/Release/mcpp0.lib"
    "${JE_BUILD}/Release/mcpp.exe" DESTINATION "${JE_STAGE}/mcpp/lib")
configure_file("${JE_MCPP_WORK}/LICENSE" "${JE_STAGE}/mcpp/LICENSE" COPYONLY)
configure_file("${JE_MCPP_WORK}/README" "${JE_STAGE}/LICENSES/mcpp/README.original" COPYONLY)
set(JE_SOURCE_NOTICES "")
foreach(JE_FILE IN ITEMS main.c directive.c eval.c expand.c support.c system.c mbchar.c system.H internal.H)
    file(READ "${JE_MCPP_WORK}/src/${JE_FILE}" JE_SOURCE)
    string(FIND "${JE_SOURCE}" "*/" JE_NOTICE_END)
    if(JE_NOTICE_END LESS 0)
        message(FATAL_ERROR "JE local mcpp: source notice missing: ${JE_FILE}")
    endif()
    math(EXPR JE_NOTICE_LENGTH "${JE_NOTICE_END} + 2")
    string(SUBSTRING "${JE_SOURCE}" 0 ${JE_NOTICE_LENGTH} JE_NOTICE)
    string(APPEND JE_SOURCE_NOTICES "src/${JE_FILE}\n${JE_NOTICE}\n\n")
endforeach()
file(WRITE "${JE_STAGE}/LICENSES/mcpp/source-notices.txt" "${JE_SOURCE_NOTICES}")
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" "${CMAKE_CURRENT_LIST_DIR}/mcpp_2.7.2_az.patch"
    "${CMAKE_CURRENT_LIST_DIR}/visualc.mak.original" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
configure_file("${CMAKE_CURRENT_LIST_DIR}/McppPatchManifest.json"
    "${JE_STAGE}/mcpp/McppPatchManifest.json" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/McppPreopenExtensionManifest.json" "${JE_STAGE}/mcpp/McppPreopenExtensionManifest.json" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/mcpp_preopen_read.patch" "${JE_STAGE}/LICENSES/O3DE/mcpp_preopen_read.patch" COPYONLY)
file(SHA256 "${JE_STAGE}/mcpp/include/mcpp_lib.h" JE_ABI_HEADER_HASH)
file(SHA256 "${JE_STAGE}/mcpp/lib/mcpp0.lib" JE_ABI_LIBRARY_HASH)
file(SHA256 "${JE_STAGE}/mcpp/lib/mcpp0.dll" JE_ABI_DLL_HASH)
file(WRITE "${JE_STAGE}/mcpp/JEPreopenAbi.json"
    "{\n  \"abiVersion\": 1,\n  \"callback\": \"mcpp_set_resolve_include_callback\",\n"
    "  \"extensionManifestSHA256\": \"0102D042875C0F02D7FB6EF578BE1B5827ADCF5DC5D5E3EF906BB9597E29D886\",\n"
    "  \"header\": {\"path\": \"mcpp/include/mcpp_lib.h\", \"sha256\": \"${JE_ABI_HEADER_HASH}\"},\n"
    "  \"importLibrary\": {\"path\": \"mcpp/lib/mcpp0.lib\", \"sha256\": \"${JE_ABI_LIBRARY_HASH}\"},\n"
    "  \"runtimeDLL\": {\"path\": \"mcpp/lib/mcpp0.dll\", \"sha256\": \"${JE_ABI_DLL_HASH}\"}\n}\n")
configure_file("${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" "${JE_STAGE}/NOTICE.txt" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/Findmcpp.cmake" "${JE_STAGE}/Findmcpp.cmake" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake"
    "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(WRITE "${JE_STAGE}/mcpp/JERecipe.json"
    "{\n  \"sourceRelease\": \"mcpp-2.7.2\",\n  \"upstreamCommit\": null,\n"
    "  \"sourceArchiveSHA256\": \"${JE_SOURCE_HASH}\",\n"
    "  \"recipeCommit\": \"ce8be74060cf231c11d464edb0862234dea7782d\",\n"
    "  \"recipeSnapshotCommit\": \"70f0b7b5257bf7cf935bf1cfd270e1b53ccead18\",\n"
    "  \"variant\": \"2.7.2_az.2\",\n"
    "  \"patchSHA256\": \"8797760af297be90586f03ae69c3ffaeb89591fa69199c4242e05f4df6efdc6b\",\n"
    "  \"preopenReadExtensionSHA256\": \"0102D042875C0F02D7FB6EF578BE1B5827ADCF5DC5D5E3EF906BB9597E29D886\",\n  \"callbackAbiVersion\": 1,\n"
    "  \"toolset\": \"v143\",\n  \"configuration\": \"Release\",\n"
    "  \"dllRuntimeLibrary\": \"MultiThreadedDLL\",\n  \"cliRuntimeLibrary\": \"MultiThreaded\"\n}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "mcpp-2.7.2_az.2-je-preopen-pathprobe-r1-windows")
set(JE_PACKAGE_OUTPUT "${JE_MCPP_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "2.7.2_az.2-je-preopen-pathprobe-r1")
set(JE_PACKAGE_SOURCE_RELEASE "mcpp-2.7.2")
unset(JE_PACKAGE_SOURCE_COMMIT)
set(JE_PACKAGE_LICENSE "BSD-2-Clause AND (Apache-2.0 OR MIT)")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_MCPP_SOURCE_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
