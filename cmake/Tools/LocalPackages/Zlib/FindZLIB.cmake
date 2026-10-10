#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# Adapted from o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18,
# package-system/zlib/FindZLIB.cmake. JE package paths are local and fail closed.

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local ZLIB package supports Windows x64 only")
endif()
if(DEFINED CMAKE_SIZEOF_VOID_P AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "JE local ZLIB package requires x64")
endif()

set(ZLIB_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/zlib/lib/zlibstatic.lib")
set(ZLIB_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/zlib/include")
set(ZLIB_LIBRARY "${ZLIB_LIBRARIES}")
set(ZLIB_INCLUDE_DIR "${ZLIB_INCLUDE_DIRS}")
set(ZLIB_VERSION "1.2.11")
set(ZLIB_VERSION_STRING "1.2.11")
set(ZLIB_VERSION_MAJOR "1")
set(ZLIB_VERSION_MINOR "2")
set(ZLIB_VERSION_PATCH "11")
set(ZLIB_MAJOR_VERSION "1")
set(ZLIB_MINOR_VERSION "2")
set(ZLIB_PATCH_VERSION "11")
if(NOT EXISTS "${ZLIB_LIBRARY}")
    set(ZLIB_LIBRARY ZLIB_LIBRARY-NOTFOUND)
endif()
if(NOT EXISTS "${ZLIB_INCLUDE_DIR}/zlib.h" OR NOT EXISTS "${ZLIB_INCLUDE_DIR}/zconf.h")
    set(ZLIB_INCLUDE_DIR ZLIB_INCLUDE_DIR-NOTFOUND)
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZLIB REQUIRED_VARS ZLIB_LIBRARY ZLIB_INCLUDE_DIR VERSION_VAR ZLIB_VERSION)
if(NOT ZLIB_FOUND)
    return()
endif()

if(NOT TARGET 3rdParty::ZLIB)
    add_library(3rdParty::ZLIB STATIC IMPORTED GLOBAL)
    add_library(ZLIB::ZLIB ALIAS 3rdParty::ZLIB)
    if(COMMAND ly_target_include_system_directories)
        ly_target_include_system_directories(TARGET 3rdParty::ZLIB INTERFACE "${ZLIB_INCLUDE_DIR}")
    else()
        target_include_directories(3rdParty::ZLIB SYSTEM INTERFACE "${ZLIB_INCLUDE_DIR}")
    endif()
    set_target_properties(3rdParty::ZLIB PROPERTIES IMPORTED_LOCATION "${ZLIB_LIBRARY}")
endif()
message(STATUS "Using JE local ZLIB 1.2.11 from ${CMAKE_CURRENT_LIST_DIR}")
