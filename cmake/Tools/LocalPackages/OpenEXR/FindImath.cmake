#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::Imath)
    return()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/JERequireLocalPath.cmake")
set(JE_OPENEXR_LOCAL_ROOT "${CMAKE_CURRENT_LIST_DIR}")
JE_python_require_local_path(JE_OPENEXR_LOCAL_ROOT)
include("${CMAKE_CURRENT_LIST_DIR}/o3de_package_utilities.cmake")

set(Imath_DIR "${CMAKE_CURRENT_LIST_DIR}/OpenEXR/lib/cmake/Imath")
find_package(Imath 3.1.3 EXACT CONFIG REQUIRED PATHS "${Imath_DIR}" NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Imath REQUIRED_VARS Imath_DIR VERSION_VAR Imath_VERSION)

o3de_import_targets(NAMESPACE_FROM
                        Imath
                    NAMESPACE_TO
                        3rdParty
                    COMPONENTS
                        Imath
                        ImathConfig)

# fixup Imath:: it forces /EHsc when compiled on windows MSVC, when it should be doing so when targetted to it instead.
set_target_properties(Imath::Imath PROPERTIES INTERFACE_COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>")

# if we're not in O3DE, it's also extremely helpful to show a message to logs that indicate that this
# library was successfully picked up, as opposed to the system one.
# A good way to know if you're in O3DE or not is that O3DE sets various cache variables before
# calling find_package, specifically, LY_VERSION_ENGINE_NAME is always set very early:
if (NOT LY_VERSION_ENGINE_NAME)
    message(STATUS "Using Imath (${Imath_VERSION}) from ${CMAKE_CURRENT_LIST_DIR}")
endif()
