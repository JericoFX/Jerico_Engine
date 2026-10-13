# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/SourcePins.cmake")
foreach(JE_PATH IN ITEMS JE_OIIO_INPUTS JE_OIIO_OUTPUT)
    JE_python_require_local_path(${JE_PATH})
endforeach()
if(NOT DEFINED JE_OIIO_PARALLEL_JOBS OR NOT JE_OIIO_PARALLEL_JOBS MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "JE local OIIO: positive integer JE_OIIO_PARALLEL_JOBS is required")
endif()
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_OIIO_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_OIIO_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local OIIO: output must be beneath this checkout/build")
endif()
if(EXISTS "${JE_OIIO_OUTPUT}")
    message(FATAL_ERROR "JE local OIIO: output must be new; existing output is preserved")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local OIIO requires Windows x64 VS2022/v143")
endif()
JE_python_verify_file("${JE_OIIO_INPUTS}" "${JE_OIIO_INPUTS_SHA256}" "pinned OIIO data witness")
include("${JE_OIIO_INPUTS}")
# The source snapshot contains only approved regular members. In particular,
# no OIIO test fixture links are extracted/copied by this producer.
foreach(JE_ID IN LISTS JE_SOURCE_IDS)
    JE_python_require_local_path(JE_SRC_${JE_ID}_ROOT)
    JE_python_verify_file("${JE_SRC_${JE_ID}_ARCHIVE}" "${JE_PIN_${JE_ID}}" "${JE_ID} source archive")
    JE_python_verify_file("${JE_SRC_${JE_ID}_MANIFEST}" "${JE_SRC_${JE_ID}_MANIFEST_SHA256}" "${JE_ID} regular membership")
    file(STRINGS "${JE_SRC_${JE_ID}_MANIFEST}" JE_RECORDS ENCODING UTF-8)
    set(JE_EXPECTED_FILES)
    foreach(JE_RECORD IN LISTS JE_RECORDS)
        if(NOT JE_RECORD MATCHES "^([a-f0-9]+)[|]([0-9]+)[|](.+)$")
            message(FATAL_ERROR "JE local OIIO: invalid pinned source record")
        endif()
        set(JE_HASH "${CMAKE_MATCH_1}")
        set(JE_SIZE "${CMAKE_MATCH_2}")
        set(JE_RELATIVE "${CMAKE_MATCH_3}")
        set(JE_FILE "${JE_SRC_${JE_ID}_ROOT}/${JE_RELATIVE}")
        JE_python_verify_file("${JE_FILE}" "${JE_HASH}" "${JE_ID}/${JE_RELATIVE}")
        file(SIZE "${JE_FILE}" JE_ACTUAL_SIZE)
        if(NOT JE_ACTUAL_SIZE EQUAL JE_SIZE)
            message(FATAL_ERROR "JE local OIIO: pinned source size differs")
        endif()
        list(APPEND JE_EXPECTED_FILES "${JE_RELATIVE}")
    endforeach()
    file(GLOB_RECURSE JE_ACTUAL_FILES RELATIVE "${JE_SRC_${JE_ID}_ROOT}" LIST_DIRECTORIES FALSE "${JE_SRC_${JE_ID}_ROOT}/*")
    file(GLOB_RECURSE JE_ACTUAL_NODES LIST_DIRECTORIES TRUE "${JE_SRC_${JE_ID}_ROOT}/*")
    foreach(JE_NODE IN LISTS JE_ACTUAL_NODES)
        if(IS_SYMLINK "${JE_NODE}")
            message(FATAL_ERROR "JE local OIIO: snapshot must not contain linked nodes")
        endif()
    endforeach()
    list(SORT JE_ACTUAL_FILES)
    list(SORT JE_EXPECTED_FILES)
    if(NOT JE_ACTUAL_FILES STREQUAL JE_EXPECTED_FILES)
        message(FATAL_ERROR "JE local OIIO: ${JE_ID} snapshot regular membership differs")
    endif()
    set(JE_FILES_${JE_ID} "${JE_EXPECTED_FILES}")
endforeach()
JE_python_verify_file("${JE_NATIVE_MANIFEST}" "${JE_NATIVE_MANIFEST_SHA256}" "native package/tool witness")
file(STRINGS "${JE_NATIVE_MANIFEST}" JE_RECORDS ENCODING UTF-8)
foreach(JE_RECORD IN LISTS JE_RECORDS)
    if(NOT JE_RECORD MATCHES "^([a-f0-9]+)[|]([0-9]+)[|](.+)$")
        message(FATAL_ERROR "JE local OIIO: invalid native witness record")
    endif()
    JE_python_verify_file("${CMAKE_MATCH_3}" "${CMAKE_MATCH_1}" "native package/tool input")
endforeach()
JE_python_require_local_path(JE_TOOL_VS_ROOT)
if(NOT EXISTS "${JE_TOOL_VS_ROOT}/VC/Auxiliary/Build/vcvars64.bat")
    message(FATAL_ERROR "JE local OIIO: explicit VS2022/v143 installation is missing")
endif()
if(NOT CMAKE_COMMAND STREQUAL JE_TOOL_CMAKE)
    message(FATAL_ERROR "JE local OIIO: invoke the pinned CMake executable")
endif()
file(MAKE_DIRECTORY "${JE_OIIO_OUTPUT}/source-work" "${JE_OIIO_OUTPUT}/native")
foreach(JE_ID IN LISTS JE_SOURCE_IDS)
    set(JE_WORK_${JE_ID} "${JE_OIIO_OUTPUT}/source-work/${JE_ID}")
    foreach(JE_RELATIVE IN LISTS JE_FILES_${JE_ID})
        configure_file("${JE_SRC_${JE_ID}_ROOT}/${JE_RELATIVE}" "${JE_WORK_${JE_ID}}/${JE_RELATIVE}" COPYONLY)
    endforeach()
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/ApplyHistoricalPatches.cmake")
set(JE_STAGE "${JE_OIIO_OUTPUT}/stage")
set(JE_PREFIX "${JE_OIIO_OUTPUT}/private-install")
set_property(GLOBAL PROPERTY JE_OIIO_NATIVE_SEQUENCE 0)
function(JE_oiio_native JE_LABEL JE_DIRECTORY)
    get_property(JE_SEQUENCE GLOBAL PROPERTY JE_OIIO_NATIVE_SEQUENCE)
    math(EXPR JE_SEQUENCE "${JE_SEQUENCE}+1")
    set_property(GLOBAL PROPERTY JE_OIIO_NATIVE_SEQUENCE "${JE_SEQUENCE}")
    set(JE_ARGS)
    math(EXPR JE_LAST "${ARGC}-1")
    foreach(JE_INDEX RANGE 2 ${JE_LAST})
        string(REPLACE ";" "\\;" JE_ARGUMENT "${ARGV${JE_INDEX}}")
        list(APPEND JE_ARGS "${JE_ARGUMENT}")
    endforeach()
    file(WRITE "${JE_OIIO_OUTPUT}/native/${JE_SEQUENCE}-${JE_LABEL}.argv.txt" "${JE_ARGS}\n")
    string(TIMESTAMP JE_STARTED "%Y-%m-%dT%H:%M:%SZ" UTC)
    execute_process(COMMAND ${JE_ARGS} WORKING_DIRECTORY "${JE_DIRECTORY}"
        OUTPUT_FILE "${JE_OIIO_OUTPUT}/native/${JE_SEQUENCE}-${JE_LABEL}.stdout.log"
        ERROR_FILE "${JE_OIIO_OUTPUT}/native/${JE_SEQUENCE}-${JE_LABEL}.stderr.log" RESULT_VARIABLE JE_RESULT)
    string(TIMESTAMP JE_ENDED "%Y-%m-%dT%H:%M:%SZ" UTC)
    file(WRITE "${JE_OIIO_OUTPUT}/native/${JE_SEQUENCE}-${JE_LABEL}.timing.json"
        "{\"startedUTC\":\"${JE_STARTED}\",\"endedUTC\":\"${JE_ENDED}\",\"result\":\"${JE_RESULT}\"}\n")
    file(WRITE "${JE_OIIO_OUTPUT}/native/${JE_SEQUENCE}-${JE_LABEL}.exitcode.txt" "${JE_RESULT}\n")
    if(NOT JE_RESULT EQUAL 0)
        message(FATAL_ERROR "JE local OIIO: ${JE_LABEL} failed (${JE_RESULT}); private outputs/logs retained")
    endif()
endfunction()
set(JE_COMMON -G "Visual Studio 17 2022" -A x64 -T v143
    "-DCMAKE_GENERATOR_INSTANCE:PATH=${JE_TOOL_VS_ROOT}"
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    -DCMAKE_CXX_STANDARD=17 -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_CXX_VISIBILITY_PRESET=hidden -DCMAKE_VISIBILITY_INLINES_HIDDEN=ON
    -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF
    -DFETCHCONTENT_FULLY_DISCONNECTED=ON -DCMAKE_DISABLE_FIND_PACKAGE_Git=ON
    -DGIT_EXECUTABLE:FILEPATH=GIT_EXECUTABLE-NOTFOUND)
function(JE_oiio_build_install JE_ID JE_EXTRA)
    JE_oiio_native("${JE_ID}-configure" "${JE_OIIO_OUTPUT}" "${CMAKE_COMMAND}"
        -S "${JE_WORK_${JE_ID}}" -B "${JE_OIIO_OUTPUT}/${JE_ID}-build" ${JE_COMMON}
        "-DCMAKE_INSTALL_PREFIX:PATH=${JE_PREFIX}/${JE_ID}" ${JE_EXTRA})
    foreach(JE_CONFIG IN ITEMS Debug Release)
        JE_oiio_native("${JE_ID}-${JE_CONFIG}-build" "${JE_OIIO_OUTPUT}" "${CMAKE_COMMAND}" --build
            "${JE_OIIO_OUTPUT}/${JE_ID}-build" --config "${JE_CONFIG}" --parallel "${JE_OIIO_PARALLEL_JOBS}" --target install)
    endforeach()
endfunction()
# Use the existing audited v143 environment explicitly. Boost 1.76's msvc
# rule accepts an explicit compiler/version/setup; no toolchain installer runs.
file(CONFIGURE OUTPUT "${JE_OIIO_OUTPUT}/bootstrap-boost.cmd" CONTENT
"@echo off\nsetlocal\ncall \"${JE_TOOL_VCVARS}\"\nif errorlevel 1 exit /b 1\npushd \"${JE_WORK_Boost}/tools/build/src/engine\"\ncall build.bat msvc\nif errorlevel 1 exit /b 1\ncopy /b b2.exe \"${JE_WORK_Boost}/b2.exe\" >nul\nif errorlevel 1 exit /b 1\nexit /b 0\n" @ONLY NEWLINE_STYLE CRLF)
file(TO_NATIVE_PATH "${JE_TOOL_CMD}" JE_BOOTSTRAP_CMD_NATIVE)
file(TO_NATIVE_PATH "${JE_OIIO_OUTPUT}/bootstrap-boost.cmd" JE_BOOTSTRAP_SCRIPT_NATIVE)
JE_oiio_native("boost-bootstrap" "${JE_WORK_Boost}" "${JE_BOOTSTRAP_CMD_NATIVE}" /d /c "${JE_BOOTSTRAP_SCRIPT_NATIVE}")
file(WRITE "${JE_WORK_Boost}/project-config.jam"
    "using msvc : 14.3 : \"${JE_TOOL_CL}\" : <setup>\"${JE_TOOL_VCVARS}\" ;\n")
foreach(JE_CONFIG IN ITEMS debug release)
    JE_oiio_native("boost-${JE_CONFIG}" "${JE_WORK_Boost}" "${JE_WORK_Boost}/b2.exe"
        "--build-dir=${JE_OIIO_OUTPUT}/boost-build" --with-filesystem --with-atomic --with-thread --with-system
        --with-headers --with-date_time --with-chrono toolset=msvc-14.3 architecture=x86 address-model=64
        link=static runtime-link=shared threading=multi "--prefix=${JE_PREFIX}/Boost" "${JE_CONFIG}" install
        visibility=hidden "-j${JE_OIIO_PARALLEL_JOBS}")
endforeach()
JE_oiio_native("jpeg-configure" "${JE_OIIO_OUTPUT}" "${CMAKE_COMMAND}" -S "${JE_WORK_libjpeg_turbo}"
    -B "${JE_OIIO_OUTPUT}/jpeg-build" ${JE_COMMON} "-DCMAKE_INSTALL_PREFIX:PATH=${JE_PREFIX}/JPEG"
    -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_JAVA=OFF -DWITH_SIMD=ON
    "-DCMAKE_ASM_NASM_COMPILER:FILEPATH=${JE_TOOL_NASM}")
JE_oiio_native("jpeg-release" "${JE_OIIO_OUTPUT}" "${CMAKE_COMMAND}" --build "${JE_OIIO_OUTPUT}/jpeg-build"
    --config Release --parallel "${JE_OIIO_PARALLEL_JOBS}" --target install)
JE_oiio_build_install(yaml_cpp "-DYAML_CPP_BUILD_TESTS=OFF;-DYAML_CPP_BUILD_TOOLS=OFF;-DYAML_CPP_BUILD_CONTRIB=ON;-DYAML_MSVC_SHARED_RT=ON;-DBUILD_SHARED_LIBS=OFF")
configure_file("${JE_WORK_OpenColorIO}/share/cmake/projects/Buildpystring.cmake" "${JE_WORK_pystring}/CMakeLists.txt" COPYONLY)
configure_file("${JE_WORK_OpenColorIO}/share/cmake/projects/Buildlcms2.cmake" "${JE_WORK_Little_CMS}/CMakeLists.txt" COPYONLY)
JE_oiio_build_install(pystring "-DBUILD_SHARED_LIBS=OFF;-DCMAKE_DEBUG_POSTFIX=d")
JE_oiio_build_install(Little_CMS "-DBUILD_SHARED_LIBS=OFF;-DCMAKE_DEBUG_POSTFIX=d;-DCMAKE_CXX_STANDARD=11")
set(JE_MAIN_ARGS "-DCMAKE_PROJECT_INCLUDE:FILEPATH=${CMAKE_CURRENT_LIST_DIR}/BuildNativeDependencies.cmake"
    "-DJE_OIIO_INPUTS:FILEPATH=${JE_OIIO_INPUTS}" "-DJE_OIIO_PRIVATE_PREFIX:PATH=${JE_PREFIX}")
set(JE_OCIO_ARGS -DBUILD_SHARED_LIBS=ON -DCMAKE_DEBUG_POSTFIX=d -DOCIO_INSTALL_EXT_PACKAGES=NONE
    -DOCIO_BUILD_APPS=ON -DOCIO_BUILD_OPENFX=OFF -DOCIO_BUILD_TESTS=OFF -DOCIO_BUILD_GPU_TESTS=OFF
    -DOCIO_BUILD_PYTHON=OFF -DOCIO_BUILD_DOCS=OFF -DOCIO_USE_SSE=ON)
JE_oiio_build_install(OpenColorIO "${JE_MAIN_ARGS};${JE_OCIO_ARGS};-DCMAKE_DISABLE_FIND_PACKAGE_OpenImageIO=ON")
set(JE_OIIO_ARGS -DBUILD_SHARED_LIBS=ON -DOIIO_BUILD_TOOLS=ON -DOIIO_BUILD_TESTS=OFF -DBUILD_TESTING=OFF
    -DUSE_PYTHON=OFF -DBUILD_DOCS=OFF -DINSTALL_DOCS=OFF -DINSTALL_FONTS=OFF -DLINKSTATIC=ON
    -DUSE_OpenGL=OFF -DUSE_Qt5=OFF -DUSE_BZip2=OFF -DUSE_FFmpeg=OFF -DUSE_Field3D=OFF -DUSE_DCMTK=OFF
    -DUSE_OpenJPEG=OFF -DUSE_Libheif=OFF -DUSE_Libsquish=OFF -DUSE_Nuke=OFF -DUSE_OpenCV=OFF
    -DUSE_OpenVDB=OFF -DUSE_Ptex=OFF -DUSE_R3DSDK=OFF -DUSE_WebP=OFF -DUSE_TBB=OFF -DUSE_GIF=OFF -DUSE_LibRaw=OFF
    -DBUILD_MISSING_FMT=OFF -DBUILD_FMT_FORCE=OFF -DBUILD_MISSING_ROBINMAP=OFF -DBUILD_ROBINMAP_FORCE=OFF
    "-DFMT_INCLUDE_DIR:PATH=${JE_WORK_fmt}/include" "-DROBINMAP_INCLUDE_DIR:PATH=${JE_WORK_robin_map}/include"
    "-DBoost_ROOT:PATH=${JE_PREFIX}/Boost" -DBoost_NO_SYSTEM_PATHS=ON -DBoost_USE_STATIC_LIBS=ON
    -DBoost_COMPILER:STRING=-vc143
    "-DJPEG_ROOT:PATH=${JE_PREFIX}/JPEG" "-DJPEGTurbo_ROOT:PATH=${JE_PREFIX}/JPEG")
JE_oiio_build_install(OpenImageIO "${JE_MAIN_ARGS};${JE_OIIO_ARGS}")
# The original second OCIO pass enables its OIIO-dependent native apps.
JE_oiio_build_install(OpenColorIO "${JE_MAIN_ARGS};${JE_OCIO_ARGS};-DCMAKE_DISABLE_FIND_PACKAGE_OpenImageIO=OFF")
include("${CMAKE_CURRENT_LIST_DIR}/StageNativePackage.cmake")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "openimageio-opencolorio-2.3.17-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_OIIO_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "2.3.17.0")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SRC_OpenImageIO_COMMIT}")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_PIN_OpenImageIO}")
set(JE_PACKAGE_SOURCE_URL "${JE_SRC_OpenImageIO_URL}")
set(JE_PACKAGE_LICENSE "BSD-3-Clause and bundled third-party notices")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
