# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Exact hunks from package-system/tiff/o3de_patch.patch at 3p-package-source@70f0b7b5.
# Called only on the producer's newly extracted private work tree.
set(JE_PATCH_FILE "${JE_TIFF_WORK}/CMakeLists.txt")
JE_python_verify_file("${JE_PATCH_FILE}" "93666ce64aeb2e02542c0f912af818d3c5308030548f8023daac2bdd5fdf439e" "unmodified TIFF CMakeLists.txt")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/o3de_patch.patch" "7d80df4a02a6b3ba8f27cd3d8554b39a3b3e3202fcd86378337ab453ec05ecd5" "original O3DE TIFF patch")
file(READ "${JE_PATCH_FILE}" JE_PATCH_TEXT)
set(JE_PATCH_BEFORE [==[
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.

cmake_minimum_required(VERSION 2.8.11) # b/c of use of BUILD_INTERFACE generator expression

# Default policy is from 2.8.9
cmake_policy(VERSION 2.8.9)
# Set MacOSX @rpath usage globally.
if (POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
]==])
set(JE_PATCH_AFTER [==[
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.

cmake_minimum_required(VERSION 3.5) # b/c of use of BUILD_INTERFACE generator expression

# Default policy is from 3.5
cmake_policy(VERSION 3.5)
# Set MacOSX @rpath usage globally.
if (POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
]==])
string(FIND "${JE_PATCH_TEXT}" "${JE_PATCH_BEFORE}" JE_PATCH_AT)
if(JE_PATCH_AT LESS 0)
    message(FATAL_ERROR "JE local TIFF: audited O3DE patch context missing")
endif()
string(REPLACE "${JE_PATCH_BEFORE}" "${JE_PATCH_AFTER}" JE_PATCH_TEXT "${JE_PATCH_TEXT}")
set(JE_PATCH_BEFORE [==[
  set(TIFF_UINT32_FORMAT "%lu")
endif()

if(SIZEOF_SIGNED_LONG EQUAL 8)
  set(TIFF_INT64_T "signed long")
  set(TIFF_INT64_FORMAT "%ld")
elseif(SIZEOF_SIGNED_LONG_LONG EQUAL 8)
  set(TIFF_INT64_T "signed long long")
  if (MINGW)
    set(TIFF_INT64_FORMAT "%I64d")
  else()
    set(TIFF_INT64_FORMAT "%lld")
  endif()
endif()

if(SIZEOF_UNSIGNED_LONG EQUAL 8)
  set(TIFF_UINT64_T "unsigned long")
  set(TIFF_UINT64_FORMAT "%lu")
elseif(SIZEOF_UNSIGNED_LONG_LONG EQUAL 8)
  set(TIFF_UINT64_T "unsigned long long")
  if (MINGW)
    set(TIFF_UINT64_FORMAT "%I64u")
  else()
    set(TIFF_UINT64_FORMAT "%llu")
  endif()
endif()

if(SIZEOF_UNSIGNED_INT EQUAL SIZEOF_SIZE_T)
]==])
set(JE_PATCH_AFTER [==[
  set(TIFF_UINT32_FORMAT "%lu")
endif()

# Open 3d Engine change: always choose the 'longest long'
# so that its compatible with other code choosing long long
# to represent i64 since that is the same size across
# more platforms.
if(SIZEOF_SIGNED_LONG_LONG EQUAL 8)
  set(TIFF_INT64_T "signed long long")
  if (MINGW)
    set(TIFF_INT64_FORMAT "%I64d")
  else()
    set(TIFF_INT64_FORMAT "%lld")
  endif()
elseif(SIZEOF_SIGNED_LONG EQUAL 8)
  set(TIFF_INT64_T "signed long")
  set(TIFF_INT64_FORMAT "%ld")
endif()

if(SIZEOF_UNSIGNED_LONG_LONG EQUAL 8)
set(TIFF_UINT64_T "unsigned long long")
  if (MINGW)
    set(TIFF_UINT64_FORMAT "%I64u")
  else()
    set(TIFF_UINT64_FORMAT "%llu")
  endif()
elseif(SIZEOF_UNSIGNED_LONG EQUAL 8)
  set(TIFF_UINT64_T "unsigned long")
  set(TIFF_UINT64_FORMAT "%lu")
endif()

if(SIZEOF_UNSIGNED_INT EQUAL SIZEOF_SIZE_T)
]==])
string(FIND "${JE_PATCH_TEXT}" "${JE_PATCH_BEFORE}" JE_PATCH_AT)
if(JE_PATCH_AT LESS 0)
    message(FATAL_ERROR "JE local TIFF: audited O3DE patch context missing")
endif()
string(REPLACE "${JE_PATCH_BEFORE}" "${JE_PATCH_AFTER}" JE_PATCH_TEXT "${JE_PATCH_TEXT}")
file(CONFIGURE OUTPUT "${JE_PATCH_FILE}" CONTENT "${JE_PATCH_TEXT}" @ONLY NEWLINE_STYLE UNIX)
JE_python_verify_file("${JE_PATCH_FILE}" "908b1ec9438d1cefccd100da5a75fad1485a612a358033caa524bd4faf7a8f29" "patched TIFF CMakeLists.txt")
file(WRITE "${JE_TIFF_OUTPUT}/recipe-patch-receipt.json"
    "{\n  \"recipe\": \"o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18/package-system/tiff/o3de_patch.patch\",\n"
    "  \"hunksApplied\": 2, \"sourceOnlyPrivateWork\": true,\n"
    "  \"beforeSha256\": \"93666ce64aeb2e02542c0f912af818d3c5308030548f8023daac2bdd5fdf439e\",\n"
    "  \"afterSha256\": \"908b1ec9438d1cefccd100da5a75fad1485a612a358033caa524bd4faf7a8f29\"\n}\n")