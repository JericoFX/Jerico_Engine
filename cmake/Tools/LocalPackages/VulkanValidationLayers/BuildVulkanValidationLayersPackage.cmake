# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS ARCHIVES_ROOT PYTHON PYTHON_SHA256 OUTPUT)
    if(NOT DEFINED JE_VVL_${JE_ARGUMENT} OR "${JE_VVL_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE VVL: JE_VVL_${JE_ARGUMENT} is required")
    endif()
endforeach()
foreach(JE_PATH IN ITEMS JE_VVL_ARCHIVES_ROOT JE_VVL_PYTHON JE_VVL_OUTPUT)
    JE_python_require_local_path(${JE_PATH})
endforeach()
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_VVL_OUTPUT}" NORMALIZE JE_OUTPUT_LOCAL)
if(NOT JE_OUTPUT_LOCAL OR JE_VVL_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE VVL: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE VVL requires Windows x64 and Visual Studio 2022/v143")
endif()
if(EXISTS "${JE_VVL_OUTPUT}")
    message(FATAL_ERROR "JE VVL: output exists; retained attempts are never overwritten")
endif()
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/SourcePins.json"
    "4b589091d152a1b7d0b0a345a3b67312930f2b1b68e7fd64b21bf6c39845cf87" "approved seven-source pins")
file(READ "${CMAKE_CURRENT_LIST_DIR}/SourcePins.json" JE_PINS)
string(JSON JE_SOURCE_COUNT LENGTH "${JE_PINS}" sources)
if(NOT JE_SOURCE_COUNT EQUAL 7)
    message(FATAL_ERROR "JE VVL: original selected source closure must contain seven inputs")
endif()
foreach(JE_INDEX RANGE 0 6)
    foreach(JE_FIELD IN ITEMS id commit url sha256 archiveRelativePath archiveRoot cmakeSHA256)
        string(JSON JE_${JE_FIELD} GET "${JE_PINS}" sources ${JE_INDEX} ${JE_FIELD})
    endforeach()
    set(JE_ARCHIVE "${JE_VVL_ARCHIVES_ROOT}/${JE_archiveRelativePath}")
    JE_python_verify_file("${JE_ARCHIVE}" "${JE_sha256}" "${JE_id} exact source archive")
    set(JE_ARCHIVE_${JE_id} "${JE_ARCHIVE}")
    set(JE_ROOT_${JE_id} "${JE_archiveRoot}")
    set(JE_CMAKE_HASH_${JE_id} "${JE_cmakeSHA256}")
endforeach()
JE_python_verify_file("${JE_VVL_PYTHON}" "${JE_VVL_PYTHON_SHA256}" "audited external Python3 generation tool")
string(JSON JE_NOTICE_HASH GET "${JE_PINS}" sourceNoticeSHA256)
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/SourceNotices.txt" "${JE_NOTICE_HASH}" "source copyright/license notices")

# SPIRV generation uses the existing external tool without adding bytecode to
# that audited interpreter's immutable standard-library tree.
set(ENV{PYTHONDONTWRITEBYTECODE} "1")
# All archive/tool/path guards above run before creating any producer output.
file(MAKE_DIRECTORY "${JE_VVL_OUTPUT}/logs" "${JE_VVL_OUTPUT}/source-work")
function(JE_vvl_native JE_LABEL)
    set(JE_ARGUMENTS "[]")
    set(JE_INDEX 0)
    foreach(JE_ARGUMENT IN LISTS ARGN)
        string(REPLACE "\\" "\\\\" JE_ARGUMENT "${JE_ARGUMENT}")
        string(REPLACE "\"" "\\\"" JE_ARGUMENT "${JE_ARGUMENT}")
        string(JSON JE_ARGUMENTS SET "${JE_ARGUMENTS}" ${JE_INDEX} "\"${JE_ARGUMENT}\"")
        math(EXPR JE_INDEX "${JE_INDEX}+1")
    endforeach()
    file(WRITE "${JE_VVL_OUTPUT}/logs/${JE_LABEL}.argv.json" "${JE_ARGUMENTS}\n")
    string(TIMESTAMP JE_START "%s" UTC)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE JE_RESULT
        OUTPUT_FILE "${JE_VVL_OUTPUT}/logs/${JE_LABEL}.stdout.txt"
        ERROR_FILE "${JE_VVL_OUTPUT}/logs/${JE_LABEL}.stderr.txt")
    string(TIMESTAMP JE_END "%s" UTC)
    math(EXPR JE_SECONDS "${JE_END}-${JE_START}")
    file(WRITE "${JE_VVL_OUTPUT}/logs/${JE_LABEL}.result.json"
        "{\"exitResult\":\"${JE_RESULT}\",\"elapsedWholeSeconds\":${JE_SECONDS}}\n")
    if(NOT JE_RESULT STREQUAL "0")
        message(FATAL_ERROR "JE VVL: ${JE_LABEL} failed (${JE_RESULT}); logs and partial work retained; no package ready")
    endif()
    message(STATUS "JE VVL: ${JE_LABEL} completed (${JE_SECONDS}s)")
endfunction()
JE_vvl_native(python-generation-version "${JE_VVL_PYTHON}" -I -c
    "print(__import__('sys').version) or __import__('sys').exit(0 if __import__('sys').version_info >= (3, 10) else 1)")
foreach(JE_ID IN ITEMS vvl headers utility spirv-headers spirv-tools robin mimalloc)
    # Each exact archive was audited for paths/types/links and matched its full
    # primary Git tree before provisioning. Never extract mutable source inputs.
    file(ARCHIVE_EXTRACT INPUT "${JE_ARCHIVE_${JE_ID}}"
        DESTINATION "${JE_VVL_OUTPUT}/source-work/${JE_ID}")
    set(JE_WORK_${JE_ID} "${JE_VVL_OUTPUT}/source-work/${JE_ID}/${JE_ROOT_${JE_ID}}")
    JE_python_verify_file("${JE_WORK_${JE_ID}}/CMakeLists.txt" "${JE_CMAKE_HASH_${JE_ID}}"
        "${JE_ID} pristine CMake source")
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/ApplySpirvToolsProvenance.cmake")
JE_vvl_apply_spirv_provenance("${JE_WORK_spirv-tools}")

foreach(JE_CONFIG IN ITEMS Release Debug)
    set(JE_INSTALL_ROOT "${JE_VVL_OUTPUT}/install/${JE_CONFIG}")
    if(JE_CONFIG STREQUAL "Debug")
        set(JE_CRT MultiThreadedDebugDLL)
    else()
        set(JE_CRT MultiThreadedDLL)
    endif()
    set(JE_COMMON -G "Visual Studio 17 2022" -A x64 -T v143
        "-DCMAKE_CONFIGURATION_TYPES:STRING=${JE_CONFIG}" "-DCMAKE_BUILD_TYPE:STRING=${JE_CONFIG}"
        "-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${JE_CRT}"
        -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF
        -DUPDATE_DEPS=OFF -DBUILD_TESTS=OFF "-DPython3_EXECUTABLE:FILEPATH=${JE_VVL_PYTHON}")
    foreach(JE_ID IN ITEMS headers spirv-headers utility robin mimalloc spirv-tools)
        set(JE_BUILD "${JE_VVL_OUTPUT}/build/${JE_CONFIG}/${JE_ID}")
        set(JE_OPTIONS)
        if(JE_ID STREQUAL "utility")
            list(APPEND JE_OPTIONS "-DVulkanHeaders_DIR:PATH=${JE_INSTALL_ROOT}/headers/share/cmake/VulkanHeaders")
        elseif(JE_ID STREQUAL "robin")
            list(APPEND JE_OPTIONS -DRH_STANDALONE_PROJECT=OFF)
        elseif(JE_ID STREQUAL "mimalloc")
            # Compatibility only for this private upstream minimum3.0 project.
            list(APPEND JE_OPTIONS -DCMAKE_POLICY_VERSION_MINIMUM=3.5
                -DMI_BUILD_STATIC=ON -DMI_BUILD_OBJECT=OFF -DMI_BUILD_SHARED=OFF -DMI_BUILD_TESTS=OFF)
        elseif(JE_ID STREQUAL "spirv-tools")
            list(APPEND JE_OPTIONS "-DSPIRV-Headers_SOURCE_DIR:PATH=${JE_WORK_spirv-headers}"
                -DSPIRV_WERROR=OFF -DSPIRV_SKIP_TESTS=ON -DSPIRV_SKIP_EXECUTABLES=ON
                -DSPIRV_TOOLS_BUILD_STATIC=ON)
        endif()
        JE_vvl_native("${JE_ID}-${JE_CONFIG}-configure" "${CMAKE_COMMAND}"
            -S "${JE_WORK_${JE_ID}}" -B "${JE_BUILD}" ${JE_COMMON}
            "-DCMAKE_INSTALL_PREFIX:PATH=${JE_INSTALL_ROOT}/${JE_ID}" ${JE_OPTIONS})
        JE_vvl_native("${JE_ID}-${JE_CONFIG}-build-install" "${CMAKE_COMMAND}"
            --build "${JE_BUILD}" --config ${JE_CONFIG} --target install --parallel 2)
    endforeach()
    set(JE_BUILD "${JE_VVL_OUTPUT}/build/${JE_CONFIG}/vvl")
    JE_vvl_native("vvl-${JE_CONFIG}-configure" "${CMAKE_COMMAND}"
        -S "${CMAKE_CURRENT_LIST_DIR}" -B "${JE_BUILD}" ${JE_COMMON}
        "-DJE_VVL_WORK:PATH=${JE_WORK_vvl}" "-DJE_VVL_INSTALL_ROOT:PATH=${JE_INSTALL_ROOT}")
    JE_vvl_native("vvl-${JE_CONFIG}-build" "${CMAKE_COMMAND}"
        --build "${JE_BUILD}" --config ${JE_CONFIG} --target vvl --parallel 2)
    set(JE_OUTPUT_${JE_CONFIG} "${JE_BUILD}/upstream/layers/${JE_CONFIG}")
    foreach(JE_FILE IN ITEMS VkLayer_khronos_validation.dll VkLayer_khronos_validation.json)
        if(NOT EXISTS "${JE_OUTPUT_${JE_CONFIG}}/${JE_FILE}")
            message(FATAL_ERROR "JE VVL: required native ${JE_CONFIG} output missing: ${JE_FILE}; no package ready")
        endif()
    endforeach()
    file(READ "${JE_OUTPUT_${JE_CONFIG}}/VkLayer_khronos_validation.json" JE_LAYER_JSON)
    string(JSON JE_LAYER_NAME GET "${JE_LAYER_JSON}" layer name)
    string(JSON JE_LAYER_API GET "${JE_LAYER_JSON}" layer api_version)
    string(JSON JE_LAYER_LIBRARY GET "${JE_LAYER_JSON}" layer library_path)
    if(NOT JE_LAYER_NAME STREQUAL "VK_LAYER_KHRONOS_validation" OR NOT JE_LAYER_API STREQUAL "1.3.261"
        OR NOT JE_LAYER_LIBRARY STREQUAL ".\\VkLayer_khronos_validation.dll")
        message(FATAL_ERROR "JE VVL: native manifest identity/relative DLL path differs; no package ready")
    endif()
endforeach()

# Staging starts only after both real configurations have built successfully.
set(JE_STAGE "${JE_VVL_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_STAGE}/vulkan-validationlayers/include" "${JE_STAGE}/LICENSES/O3DE")
foreach(JE_CONFIG IN ITEMS Release Debug)
    string(TOLOWER "${JE_CONFIG}" JE_LOWER)
    file(MAKE_DIRECTORY "${JE_STAGE}/vulkan-validationlayers/lib/${JE_LOWER}")
    file(COPY "${JE_OUTPUT_${JE_CONFIG}}/" DESTINATION "${JE_STAGE}/vulkan-validationlayers/lib/${JE_LOWER}")
endforeach()
foreach(JE_ID IN ITEMS vvl headers utility spirv-headers spirv-tools robin mimalloc)
    file(MAKE_DIRECTORY "${JE_STAGE}/LICENSES/${JE_ID}")
    if(JE_ID STREQUAL "vvl")
        file(COPY "${JE_WORK_${JE_ID}}/LICENSE.txt" DESTINATION "${JE_STAGE}/LICENSES/${JE_ID}")
    elseif(JE_ID STREQUAL "headers" OR JE_ID STREQUAL "utility")
        file(COPY "${JE_WORK_${JE_ID}}/LICENSE.md" "${JE_WORK_${JE_ID}}/LICENSES"
            DESTINATION "${JE_STAGE}/LICENSES/${JE_ID}")
    else()
        file(COPY "${JE_WORK_${JE_ID}}/LICENSE" DESTINATION "${JE_STAGE}/LICENSES/${JE_ID}")
    endif()
endforeach()
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/SourceNotices.txt" DESTINATION "${JE_STAGE}/LICENSES")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/Findvulkan-validationlayers.cmake" "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt"
    "${CMAKE_CURRENT_LIST_DIR}/README.md" DESTINATION "${JE_STAGE}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake"
    "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
string(JSON JE_RECIPE SET "${JE_PINS}" toolset "\"v143\"")
string(JSON JE_RECIPE SET "${JE_RECIPE}" generator "\"Visual Studio 17 2022\"")
string(JSON JE_RECIPE SET "${JE_RECIPE}" configurations "[\"Release\",\"Debug\"]")
string(JSON JE_RECIPE SET "${JE_RECIPE}" releaseCRT "\"MultiThreadedDLL\"")
string(JSON JE_RECIPE SET "${JE_RECIPE}" debugCRT "\"MultiThreadedDebugDLL\"")
string(JSON JE_RECIPE SET "${JE_RECIPE}" externalPythonToolSHA256 "\"${JE_VVL_PYTHON_SHA256}\"")
string(JSON JE_RECIPE SET "${JE_RECIPE}" pythonDistributed false)
string(JSON JE_RECIPE SET "${JE_RECIPE}" sharedSpirvAuxiliaryDistributed false)
file(WRITE "${JE_STAGE}/vulkan-validationlayers/JERecipe.json" "${JE_RECIPE}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "vulkan-validationlayers-1.3.261-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_VVL_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "1.3.261")
set(JE_PACKAGE_SOURCE_COMMIT "a228e25346a931c791714977becb48b6702ec118")
set(JE_PACKAGE_SOURCE_SHA256 "107ad92b5610395d37c7533d716f9ce7e23b86d9cede749711c8f437581ee6a5")
set(JE_PACKAGE_SOURCE_URL "https://codeload.github.com/KhronosGroup/Vulkan-ValidationLayers/tar.gz/${JE_PACKAGE_SOURCE_COMMIT}")
set(JE_PACKAGE_LICENSE "Apache-2.0 AND MIT AND BSD-2-Clause")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
file(WRITE "${JE_VVL_OUTPUT}/package-ready.json"
    "{\"status\":\"PRODUCED; NATIVE CONSUMER/ROOT/EDITOR ACCEPTANCE PENDING\",\"version\":\"1.3.261\"}\n")
