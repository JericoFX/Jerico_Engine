# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Exact executable changes from historical O3DE Windows/rpath patches.
# These full hashes cover context and raw bytes; only the fresh work copy changes.
function(JE_oiio_patch JE_RELATIVE JE_BEFORE_SHA JE_AFTER_SHA JE_BEFORE JE_AFTER JE_COUNT)
    set(JE_PATH "${JE_WORK_OpenImageIO}/${JE_RELATIVE}")
    JE_python_verify_file("${JE_PATH}" "${JE_BEFORE_SHA}" "historical OIIO patch input")
    file(READ "${JE_PATH}" JE_CONTENT)
    string(REPLACE "${JE_BEFORE}" "" JE_REMOVED "${JE_CONTENT}")
    string(LENGTH "${JE_CONTENT}" JE_LENGTH)
    string(LENGTH "${JE_REMOVED}" JE_REMOVED_LENGTH)
    string(LENGTH "${JE_BEFORE}" JE_BEFORE_LENGTH)
    math(EXPR JE_MATCHES "(${JE_LENGTH}-${JE_REMOVED_LENGTH})/${JE_BEFORE_LENGTH}")
    if(NOT JE_MATCHES EQUAL JE_COUNT)
        message(FATAL_ERROR "JE local OIIO: historical patch occurrence count differs")
    endif()
    string(REPLACE "${JE_BEFORE}" "${JE_AFTER}" JE_CONTENT "${JE_CONTENT}")
    file(CONFIGURE OUTPUT "${JE_PATH}" CONTENT "${JE_CONTENT}" @ONLY NEWLINE_STYLE UNIX)
    JE_python_verify_file("${JE_PATH}" "${JE_AFTER_SHA}" "historical OIIO patch output")
endfunction()
JE_oiio_patch("src/cmake/compiler.cmake"
    "cd31698fe412f02182fc232e7f65706f1f46629b425c43b71ae14281935b2403"
    "820b32c3924c2749b3fe326f46e91e73efe17dff01745d375bee66c0e4a74c3f"
    "set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)" "set (CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)" 1)
JE_oiio_patch("src/include/OpenImageIO/sysutil.h"
    "ead20350e006cf6c34ff895f4746559c2c323269a6bce75a6c10a59bf4226c5f"
    "14b811c1fad2e23d90941eb676809510b853d50e54c244a3470e174f31ac662f"
    "#define OIIO_HAS_STACKTRACE 1" "#define OIIO_HAS_STACKTRACE 0" 1)
set(JE_PATH "${JE_WORK_OpenImageIO}/src/libutil/sysutil.cpp")
JE_python_verify_file("${JE_PATH}" "ed23d93794cbbdcb411dcd1b6e93f8a76588fb9768f215e69107cd7133862928" "historical stacktrace input")
file(READ "${JE_PATH}" JE_CONTENT)
foreach(JE_BEFORE IN ITEMS "#if BOOST_VERSION >= 106500" "#    if BOOST_VERSION >= 106500")
    string(REPLACE "${JE_BEFORE}" "" JE_REMOVED "${JE_CONTENT}")
    string(LENGTH "${JE_CONTENT}" JE_LENGTH)
    string(LENGTH "${JE_REMOVED}" JE_REMOVED_LENGTH)
    string(LENGTH "${JE_BEFORE}" JE_BEFORE_LENGTH)
    math(EXPR JE_MATCHES "(${JE_LENGTH}-${JE_REMOVED_LENGTH})/${JE_BEFORE_LENGTH}")
    if(JE_BEFORE STREQUAL "#if BOOST_VERSION >= 106500")
        set(JE_COUNT 4)
        set(JE_AFTER "#if 0")
    else()
        set(JE_COUNT 1)
        set(JE_AFTER "#    if 0")
    endif()
    if(NOT JE_MATCHES EQUAL JE_COUNT)
        message(FATAL_ERROR "JE local OIIO: historical stacktrace hunk count differs")
    endif()
    string(REPLACE "${JE_BEFORE}" "${JE_AFTER}" JE_CONTENT "${JE_CONTENT}")
endforeach()
file(CONFIGURE OUTPUT "${JE_PATH}" CONTENT "${JE_CONTENT}" @ONLY NEWLINE_STYLE UNIX)
JE_python_verify_file("${JE_PATH}" "d215c5dc2dae9cb34d3e50bb125723275890ec7f5ea70965023a84b5a5c0a3c0" "historical stacktrace output")
