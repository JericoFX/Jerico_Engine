# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Exact dependency of the historical Windows recipe, using its audited JE rebuild.
JE_python_require_local_path(JE_ZLIB_PACKAGE_ROOT)
foreach(JE_ZLIB_INPUT IN ITEMS FindZLIB.cmake zlib/include/zlib.h zlib/include/zconf.h zlib/lib/zlibstatic.lib)
    if(NOT EXISTS "${JE_ZLIB_PACKAGE_ROOT}/${JE_ZLIB_INPUT}")
        message(FATAL_ERROR "JE local PNG: complete local JE zlib package is required")
    endif()
endforeach()
JE_python_verify_file("${JE_ZLIB_PACKAGE_ROOT}/FindZLIB.cmake"
    "b2662eb8469bad3c2cf93e3ea797e3df53c49ab4a9a1ce4c2ac119f7ef19bbd0" "JE zlib Find module")
JE_python_verify_file("${JE_ZLIB_PACKAGE_ROOT}/zlib/include/zlib.h"
    "4ddc82b4af931ab55f44d977bde81bfbc4151b5dcdccc03142831a301b5ec3c8" "zlib 1.2.11 header")
JE_python_verify_file("${JE_ZLIB_PACKAGE_ROOT}/zlib/include/zconf.h"
    "7f4e7f02c4c1b7ef830a8b4331e1ba04f34e97f2c0dcdd81f967ff16fd5ee6d0" "JE zlib generated configuration")
JE_python_verify_file("${JE_ZLIB_PACKAGE_ROOT}/zlib/lib/zlibstatic.lib"
    "00842f0c5be67674da3a0c1369da818ffeff1dea4f805beb06dd50f84d5a4fc1" "JE zlib static library")