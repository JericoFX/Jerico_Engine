# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE IMATH_ARCHIVE OUTPUT)
    if(NOT DEFINED JE_OPENEXR_${JE_ARGUMENT} OR "${JE_OPENEXR_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local OpenEXR: JE_OPENEXR_${JE_ARGUMENT} is required")
    endif()
    JE_python_require_local_path(JE_OPENEXR_${JE_ARGUMENT})
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/VerifyZlibInput.cmake")
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_OPENEXR_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_OPENEXR_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local OpenEXR: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local OpenEXR requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_OPENEXR_OUTPUT}")
    message(FATAL_ERROR "JE local OpenEXR: output must be new; existing outputs are preserved")
endif()
set(JE_SOURCE_COMMIT "697cc449cc589ed780acda88e79e6145ddcd121f")
set(JE_SOURCE_HASH "5ed17a3be8ae029e26a7314a2d41e6febc2102cef1eafb1ffe6c0550ea1993be")
set(JE_IMATH_COMMIT "6dc0820482aeb24a22f520d7b165a410589648c3")
set(JE_IMATH_HASH "52aeb73f94911a4e107d0a2870ec98d7eb9d3907d25238b12e496c71a5fca4a3")
set(JE_SOURCE_URL "https://codeload.github.com/AcademySoftwareFoundation/openexr/tar.gz/${JE_SOURCE_COMMIT}")
JE_python_verify_file("${JE_OPENEXR_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "audited OpenEXR v3.1.3 archive")
JE_python_verify_file("${JE_OPENEXR_IMATH_ARCHIVE}" "${JE_IMATH_HASH}" "audited Imath v3.1.3 archive")
set(JE_STAGE "${JE_OPENEXR_OUTPUT}/stage")
set(JE_PREFIX "${JE_STAGE}/OpenEXR")
file(MAKE_DIRECTORY "${JE_OPENEXR_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_OPENEXR_SOURCE_ARCHIVE}" DESTINATION "${JE_OPENEXR_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_OPENEXR_IMATH_ARCHIVE}" DESTINATION "${JE_OPENEXR_OUTPUT}/source-work")
set(JE_OPENEXR_WORK "${JE_OPENEXR_OUTPUT}/source-work/openexr-${JE_SOURCE_COMMIT}")
set(JE_IMATH_WORK "${JE_OPENEXR_OUTPUT}/source-work/Imath-${JE_IMATH_COMMIT}")
foreach(JE_PROJECT IN ITEMS OpenEXR Imath)
    if(JE_PROJECT STREQUAL "OpenEXR")
        set(JE_WORK "${JE_OPENEXR_WORK}")
    else()
        set(JE_WORK "${JE_IMATH_WORK}")
    endif()
    file(STRINGS "${JE_WORK}/CMakeLists.txt" JE_VERSION_LINE REGEX "^project\\(${JE_PROJECT} VERSION ")
    if(NOT JE_VERSION_LINE STREQUAL "project(${JE_PROJECT} VERSION 3.1.3 LANGUAGES C CXX)")
        message(FATAL_ERROR "JE local OpenEXR: ${JE_PROJECT} source version differs")
    endif()
endforeach()
# Separate native projects preserve both installed configurations and upstream
# export files. Dependencies are required locally before upstream fallback paths.
set(JE_COMMON_ARGS -G "Visual Studio 17 2022" -A x64 -T v143
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DDOCS=OFF -DPYTHON=OFF
    "-DCMAKE_INSTALL_PREFIX:PATH=${JE_PREFIX}")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${JE_IMATH_WORK}" -B "${JE_OPENEXR_OUTPUT}/imath-build"
    ${JE_COMMON_ARGS} RESULT_VARIABLE JE_RESULT)
if(NOT JE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local Imath configure failed (${JE_RESULT}); outputs retained")
endif()
foreach(JE_CONFIG IN ITEMS Debug Release)
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_OPENEXR_OUTPUT}/imath-build" --config "${JE_CONFIG}"
        RESULT_VARIABLE JE_RESULT)
    if(NOT JE_RESULT EQUAL 0)
        message(FATAL_ERROR "JE local Imath ${JE_CONFIG} build failed (${JE_RESULT}); outputs retained")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" --install "${JE_OPENEXR_OUTPUT}/imath-build" --config "${JE_CONFIG}"
        RESULT_VARIABLE JE_RESULT)
    if(NOT JE_RESULT EQUAL 0)
        message(FATAL_ERROR "JE local Imath ${JE_CONFIG} stage failed (${JE_RESULT}); outputs retained")
    endif()
endforeach()
# The active upstream LibraryDefine helper omits C11; this MSVC toolchain
# also requires explicit opt-in for the stdatomic implementation.
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${JE_OPENEXR_WORK}" -B "${JE_OPENEXR_OUTPUT}/openexr-build"
    ${JE_COMMON_ARGS}
    -DCMAKE_C_STANDARD:STRING=11 -DCMAKE_C_STANDARD_REQUIRED:BOOL=ON
    -DCMAKE_C_EXTENSIONS:BOOL=OFF
    "-DCMAKE_C_FLAGS:STRING=/experimental:c11atomics"
    -DOPENEXR_BUILD_EXAMPLES=OFF -DOPENEXR_INSTALL_EXAMPLES=OFF
    -DOPENEXR_BUILD_TOOLS=OFF -DOPENEXR_INSTALL_TOOLS=OFF -DOPENEXR_RUN_FUZZ_TESTS=OFF
    -DOPENEXR_FORCE_INTERNAL_IMATH=OFF -DOPENEXR_FORCE_INTERNAL_ZLIB=OFF
    -DFETCHCONTENT_FULLY_DISCONNECTED=ON
    "-DCMAKE_PROJECT_OpenEXR_INCLUDE:FILEPATH=${CMAKE_CURRENT_LIST_DIR}/OfflineDependencies.cmake"
    "-DJE_OPENEXR_IMATH_CONFIG:PATH=${JE_PREFIX}/lib/cmake/Imath"
    "-DJE_ZLIB_PACKAGE_ROOT:PATH=${JE_ZLIB_PACKAGE_ROOT}"
    RESULT_VARIABLE JE_RESULT)
if(NOT JE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local OpenEXR configure failed (${JE_RESULT}); outputs retained")
endif()
foreach(JE_CONFIG IN ITEMS Debug Release)
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_OPENEXR_OUTPUT}/openexr-build" --config "${JE_CONFIG}"
        RESULT_VARIABLE JE_RESULT)
    if(NOT JE_RESULT EQUAL 0)
        message(FATAL_ERROR "JE local OpenEXR ${JE_CONFIG} build failed (${JE_RESULT}); outputs retained")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" --install "${JE_OPENEXR_OUTPUT}/openexr-build" --config "${JE_CONFIG}"
        RESULT_VARIABLE JE_RESULT)
    if(NOT JE_RESULT EQUAL 0)
        message(FATAL_ERROR "JE local OpenEXR ${JE_CONFIG} stage failed (${JE_RESULT}); outputs retained")
    endif()
endforeach()
file(GLOB_RECURSE JE_UNEXPECTED_DLLS "${JE_PREFIX}/*.dll")
if(JE_UNEXPECTED_DLLS)
    message(FATAL_ERROR "JE local OpenEXR: static payload unexpectedly contains DLLs")
endif()
foreach(JE_PROJECT IN ITEMS OpenEXR Imath)
    if(JE_PROJECT STREQUAL "OpenEXR")
        set(JE_WORK "${JE_OPENEXR_WORK}")
        set(JE_NOTICE_ROOT "${JE_WORK}/src/lib")
    else()
        set(JE_WORK "${JE_IMATH_WORK}")
        set(JE_NOTICE_ROOT "${JE_WORK}/src/Imath")
    endif()
    file(COPY "${JE_WORK}/LICENSE.md" DESTINATION "${JE_STAGE}/LICENSES/${JE_PROJECT}")
    # Retain contributor notices in every library source, including compressor
    # implementations; binary resources, tools and Python sources are not staged.
    file(GLOB_RECURSE JE_NOTICES RELATIVE "${JE_WORK}" LIST_DIRECTORIES FALSE
        "${JE_NOTICE_ROOT}/*.h" "${JE_NOTICE_ROOT}/*.cpp" "${JE_NOTICE_ROOT}/*.c")
    foreach(JE_NOTICE IN LISTS JE_NOTICES)
        configure_file("${JE_WORK}/${JE_NOTICE}" "${JE_STAGE}/LICENSES/${JE_PROJECT}/source-notices/${JE_NOTICE}" COPYONLY)
    endforeach()
endforeach()
file(COPY "${JE_OPENEXR_WORK}/PATENTS" DESTINATION "${JE_STAGE}/LICENSES/OpenEXR")
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/FindOpenEXR.cmake" "${CMAKE_CURRENT_LIST_DIR}/FindImath.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/o3de_package_utilities.cmake" "${CMAKE_CURRENT_LIST_DIR}/README.md"
    "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" DESTINATION "${JE_STAGE}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake" "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(WRITE "${JE_STAGE}/OpenEXR/JEImathSource.json"
    "{\"version\":\"3.1.3\",\"commit\":\"${JE_IMATH_COMMIT}\",\"sha256\":\"${JE_IMATH_HASH}\",\"url\":\"https://codeload.github.com/AcademySoftwareFoundation/Imath/tar.gz/${JE_IMATH_COMMIT}\",\"license\":\"BSD-3-Clause\"}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "OpenEXR-3.1.3-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_OPENEXR_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "3.1.3")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_LICENSE "BSD-3-Clause")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_SOURCE_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
