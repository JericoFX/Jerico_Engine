#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::OpenEXR)
    return()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/JERequireLocalPath.cmake")
set(JE_OPENEXR_LOCAL_ROOT "${CMAKE_CURRENT_LIST_DIR}")
JE_python_require_local_path(JE_OPENEXR_LOCAL_ROOT)
include("${CMAKE_CURRENT_LIST_DIR}/o3de_package_utilities.cmake")

# OpenEXR depends on ZLIB.  For maximum compatibility here, we use the
# official ZLIB library name, ie, ZLIB::ZLIB and not o3de 3rdParty::ZLIB.
# O3DE's zlib package will define both.  If we're in O3DE we can also
# auto-download ZLIB.
if (NOT TARGET ZLIB::ZLIB)
    if (COMMAND ly_download_associated_package)
        ly_download_associated_package(ZLIB REQUIRED MODULE)
    else()
        JE_python_require_local_path(JE_ZLIB_PACKAGE_ROOT)
        list(PREPEND CMAKE_MODULE_PATH "${JE_ZLIB_PACKAGE_ROOT}")
    endif()
    find_package(ZLIB 1.2.11 EXACT REQUIRED MODULE)
endif()

if(NOT DEFINED ZLIB_VERSION_STRING)
    find_package(ZLIB 1.2.11 EXACT REQUIRED MODULE)
endif()
if(NOT ZLIB_VERSION_STRING VERSION_EQUAL "1.2.11")
    message(FATAL_ERROR "JE local OpenEXR: original zlib 1.2.11 is required")
endif()

find_package(Imath 3.1.3 EXACT MODULE REQUIRED) # will bring in the FindImath.cmake in the same folder

# read the existing config files that OpenEXR made when it compiled:
set(OpenEXR_DIR "${CMAKE_CURRENT_LIST_DIR}/OpenEXR/lib/cmake/OpenEXR")
find_package(OpenEXR 3.1.3 EXACT CONFIG REQUIRED PATHS "${OpenEXR_DIR}" NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenEXR REQUIRED_VARS OpenEXR_DIR VERSION_VAR OpenEXR_VERSION)

# map from "OpenEXR::xxxxx to 3rdParty::xxxxxx"
o3de_import_targets(NAMESPACE_FROM 
                        OpenEXR
                    NAMESPACE_TO 
                        3rdParty
                    COMPONENTS 
                        OpenEXRConfig
                        IexConfig
                        IlmThreadConfig
                        Iex
                        IlmThread
                        OpenEXRCore
                        OpenEXRUtil
                        OpenEXR)

# fixup OpenEXR::xxxxxx it forces /EHsc when compiled on windows MSVC, when it should be doing so when targetted to it instead:
set_target_properties(OpenEXR::Iex PROPERTIES INTERFACE_COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>")
set_target_properties(OpenEXR::IlmThread PROPERTIES INTERFACE_COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>")
set_target_properties(OpenEXR::OpenEXRCore PROPERTIES INTERFACE_COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>")
set_target_properties(OpenEXR::OpenEXR PROPERTIES INTERFACE_COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>")
set_target_properties(OpenEXR::OpenEXRUtil PROPERTIES INTERFACE_COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>")

# if we're not in O3DE, it's also extremely helpful to show a message to logs that indicate that this
# library was successfully picked up, as opposed to the system one.
# A good way to know if you're in O3DE or not is that O3DE sets various cache variables before 
# calling find_package, specifically, LY_VERSION_ENGINE_NAME is always set very early:
if (NOT LY_VERSION_ENGINE_NAME)
    message(STATUS "Using OpenEXR ${OpenEXR_VERSION} from ${CMAKE_CURRENT_LIST_DIR}")
endif()

# compat - some libraries check for OPENEXR_VERSION even though the correct check is OpenEXR_VERSION
set(OPENEXR_VERSION ${OpenEXR_VERSION})
