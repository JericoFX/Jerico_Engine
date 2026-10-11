# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Adapted from the exact TIFF Windows recipe at 3p-package-source@70f0b7b5.
# No acquisition, global install, or modifications of the immutable source snapshot.
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE OUTPUT)
    if(NOT DEFINED JE_TIFF_${JE_ARGUMENT} OR "${JE_TIFF_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local TIFF: JE_TIFF_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_TIFF_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_TIFF_OUTPUT)
include("${CMAKE_CURRENT_LIST_DIR}/VerifyZlibInput.cmake")
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_TIFF_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_TIFF_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local TIFF: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local TIFF requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_TIFF_OUTPUT}")
    message(FATAL_ERROR "JE local TIFF: output must be new; existing outputs are preserved")
endif()
set(JE_SOURCE_COMMIT "4862b0d7bcc786304ff4e8c31e8d5ccfb868fb99")
set(JE_SOURCE_HASH "cce672d2f5941759573cb3904cc8f00f065d54a3689a80719229e961d6a324c8")
set(JE_SOURCE_URL "https://gitlab.com/libtiff/libtiff/-/archive/${JE_SOURCE_COMMIT}/libtiff-${JE_SOURCE_COMMIT}.tar.gz")
JE_python_verify_file("${JE_TIFF_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "audited v4.2.0 commit archive")
# Verify the original recipe before creating outputs or launching native tools.
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/o3de_patch.patch"
    "7d80df4a02a6b3ba8f27cd3d8554b39a3b3e3202fcd86378337ab453ec05ecd5" "original O3DE TIFF patch")

set(JE_TIFF_WORK "${JE_TIFF_OUTPUT}/source-work/libtiff-${JE_SOURCE_COMMIT}")
set(JE_BUILD "${JE_TIFF_OUTPUT}/build")
set(JE_STAGE "${JE_TIFF_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_TIFF_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_TIFF_SOURCE_ARCHIVE}" DESTINATION "${JE_TIFF_OUTPUT}/source-work")
include("${CMAKE_CURRENT_LIST_DIR}/ApplyTiffRecipePatch.cmake")
file(STRINGS "${JE_TIFF_WORK}/libtiff/tiffvers.h" JE_VERSION_LINE REGEX "^#define TIFFLIB_VERSION ")
if(NOT JE_VERSION_LINE STREQUAL "#define TIFFLIB_VERSION 20201219")
    message(FATAL_ERROR "JE local TIFF: source is not the audited libtiff 4.2.0 release")
endif()
# Keep all original internal codecs. External feature switches are the original recipe.
# Upstream 4.2.0 always configures tool/test targets; build only tiff, as the recipe did.
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${JE_TIFF_WORK}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143
    -DCMAKE_POLICY_DEFAULT_CMP0074=NEW -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL -DBUILD_SHARED_LIBS=OFF
    -Djpeg=OFF -Dold-jpeg=OFF -Dpixarlog=OFF -Dlzma=OFF -Dwebp=OFF
    -Djbig=OFF -Dzstd=OFF -Djpeg12=OFF -Dzlib=ON -Dlibdeflate=OFF -Dcxx=OFF
    "-DCMAKE_MODULE_PATH:PATH=${JE_ZLIB_PACKAGE_ROOT}"
    RESULT_VARIABLE JE_CONFIGURE_RESULT)
if(NOT JE_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local TIFF configure failed (${JE_CONFIGURE_RESULT}); outputs retained")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_BUILD}" --target tiff --config Release
    RESULT_VARIABLE JE_BUILD_RESULT)
if(NOT JE_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local TIFF build failed (${JE_BUILD_RESULT}); outputs retained")
endif()
if(NOT EXISTS "${JE_BUILD}/libtiff/Release/tiff.lib")
    message(FATAL_ERROR "JE local TIFF: expected Release static library is missing")
endif()

file(MAKE_DIRECTORY "${JE_STAGE}/tiff/include" "${JE_STAGE}/tiff/lib" "${JE_STAGE}/LICENSES/TIFF")
file(COPY "${JE_BUILD}/libtiff/tiffconf.h" "${JE_TIFF_WORK}/libtiff/tiff.h"
    "${JE_TIFF_WORK}/libtiff/tiffvers.h" "${JE_TIFF_WORK}/libtiff/tiffio.h"
    DESTINATION "${JE_STAGE}/tiff/include")
file(COPY "${JE_BUILD}/libtiff/Release/tiff.lib" DESTINATION "${JE_STAGE}/tiff/lib")
file(COPY "${JE_TIFF_WORK}/COPYRIGHT" DESTINATION "${JE_STAGE}/tiff")
# Full embedded library and port notices include Leffler/SGI, Larson, Cringle,
# UC Berkeley BSD notices and individual contributors; retain them byte-exact.
file(COPY "${JE_TIFF_WORK}/COPYRIGHT" DESTINATION "${JE_STAGE}/LICENSES/TIFF")
foreach(JE_NOTICE_DIR IN ITEMS libtiff port)
    file(GLOB_RECURSE JE_NOTICE_FILES LIST_DIRECTORIES FALSE RELATIVE "${JE_TIFF_WORK}"
        "${JE_TIFF_WORK}/${JE_NOTICE_DIR}/*.c" "${JE_TIFF_WORK}/${JE_NOTICE_DIR}/*.h")
    foreach(JE_NOTICE IN LISTS JE_NOTICE_FILES)
        configure_file("${JE_TIFF_WORK}/${JE_NOTICE}" "${JE_STAGE}/LICENSES/TIFF/source-notices/${JE_NOTICE}" COPYONLY)
    endforeach()
endforeach()
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake" "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(COPY "${CMAKE_CURRENT_LIST_DIR}/FindTIFF.cmake" "${CMAKE_CURRENT_LIST_DIR}/README.md"
    "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" DESTINATION "${JE_STAGE}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/o3de_patch.patch" "${JE_TIFF_OUTPUT}/recipe-patch-receipt.json"
    DESTINATION "${JE_STAGE}/LICENSES/TIFF")
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(WRITE "${JE_STAGE}/tiff/JERecipe.json" [=[
{
  "upstreamVersion": "4.2.0",
  "originalPackageVersion": "4.2.0.15-rev3",
  "recipeCommit": "70f0b7b5257bf7cf935bf1cfd270e1b53ccead18",
  "generator": "Visual Studio 17 2022", "toolset": "v143", "architecture": "x64",
  "configuration": "Release", "runtimeLibrary": "MultiThreadedDLL",
  "static": true, "cxx": false, "zlib": "1.2.11",
  "internalCodecs": ["ccitt", "packbits", "lzw", "thunder", "next", "logluv", "mdi"],
  "externalDisabledByOriginalRecipe": ["jpeg", "old-jpeg", "pixarlog", "lzma", "webp", "jbig", "zstd", "jpeg12", "libdeflate"],
  "historicalBinaryCompatibility": "not asserted"
}
]=])
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "tiff-4.2.0.15-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_TIFF_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "4.2.0.15")
set(JE_PACKAGE_SOURCE_COMMIT "${JE_SOURCE_COMMIT}")
set(JE_PACKAGE_LICENSE "libtiff AND BSD-3-Clause AND BSD-2-Clause AND LicenseRef-TIFF-LZW-Berkeley")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_SOURCE_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")