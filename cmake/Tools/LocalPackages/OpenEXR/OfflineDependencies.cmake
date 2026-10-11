# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Included just after upstream project(), before OpenEXRSetup's fetch fallbacks.
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/VerifyZlibInput.cmake")
JE_python_require_local_path(JE_OPENEXR_IMATH_CONFIG)
list(PREPEND CMAKE_MODULE_PATH "${JE_ZLIB_PACKAGE_ROOT}")
find_package(ZLIB 1.2.11 EXACT REQUIRED MODULE)
find_package(Imath 3.1.3 EXACT REQUIRED CONFIG PATHS "${JE_OPENEXR_IMATH_CONFIG}" NO_DEFAULT_PATH)
if(NOT TARGET ZLIB::ZLIB OR NOT TARGET Imath::Imath)
    message(FATAL_ERROR "JE local OpenEXR: native offline dependencies are missing")
endif()
