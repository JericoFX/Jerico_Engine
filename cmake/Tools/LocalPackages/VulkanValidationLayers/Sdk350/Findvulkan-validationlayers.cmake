# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# SDK Release tool pair; shared by engine Debug/Profile, never Release/Shipping.

if(TARGET 3rdParty::vulkan-validationlayers)
    return()
endif()
include("${CMAKE_CURRENT_LIST_DIR}/JERequireLocalPath.cmake")
set(JE_VVL_ROOT "${CMAKE_CURRENT_LIST_DIR}")
JE_python_require_local_path(JE_VVL_ROOT)
set(JE_VVL_PAIR "${JE_VVL_ROOT}/vulkan-validationlayers/lib/release")
JE_python_require_local_path(JE_VVL_PAIR)
JE_python_verify_file("${JE_VVL_PAIR}/VkLayer_khronos_validation.dll"
    "37ba9ee56ebd4c57da6e1e0e2ce4aa15f147f308ed7c6c7cdf5f22d2636bc22d" "signed SDK 1.4.350 Release DLL")
JE_python_verify_file("${JE_VVL_PAIR}/VkLayer_khronos_validation.json"
    "e275925d7c6ac0c87e0bdffae4caef8b62d3cacee28268d535da42ad72448e5f" "matching SDK 1.4.350 manifest")
JE_python_verify_file("${JE_VVL_ROOT}/LICENSES/vvl/LICENSE.txt"
    "956a282096cfc658a59db5d7b1cf03625decfcc32a1059e9ad90a99e897436f7" "official 1.4.350 license")
file(READ "${JE_VVL_PAIR}/VkLayer_khronos_validation.json" JE_MANIFEST)
string(JSON JE_NAME GET "${JE_MANIFEST}" layer name)
string(JSON JE_API GET "${JE_MANIFEST}" layer api_version)
string(JSON JE_LIBRARY GET "${JE_MANIFEST}" layer library_path)
if(NOT JE_NAME STREQUAL "VK_LAYER_KHRONOS_validation" OR NOT JE_API STREQUAL "1.4.350"
    OR NOT JE_LIBRARY STREQUAL ".\\VkLayer_khronos_validation.dll")
    message(FATAL_ERROR "JE VVL: SDK layer identity or relative DLL path changed")
endif()
set(vulkan-validationlayers_VERSION "1.4.350")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(vulkan-validationlayers REQUIRED_VARS JE_VVL_ROOT
    VERSION_VAR vulkan-validationlayers_VERSION)
add_library(3rdParty::vulkan-validationlayers INTERFACE IMPORTED GLOBAL)
set(JE_VVL_DEPLOY_CONFIG "$<OR:$<CONFIG:Debug>,$<CONFIG:Profile>>")
set(vulkan-validationlayers_RUNTIME_JSON_DEPENDENCIES
    "$<${JE_VVL_DEPLOY_CONFIG}:${JE_VVL_PAIR}/VkLayer_khronos_validation.json>")
set(JE_VVL_RUNTIME_DLL
    "$<${JE_VVL_DEPLOY_CONFIG}:${JE_VVL_PAIR}/VkLayer_khronos_validation.dll>")
if(COMMAND ly_add_target_files)
    ly_add_target_files(TARGETS 3rdParty::vulkan-validationlayers
        FILES "${vulkan-validationlayers_RUNTIME_JSON_DEPENDENCIES}" "${JE_VVL_RUNTIME_DLL}")
endif()
set_property(TARGET 3rdParty::vulkan-validationlayers PROPERTY JE_RUNTIME_FILES
    "${vulkan-validationlayers_RUNTIME_JSON_DEPENDENCIES};${JE_VVL_RUNTIME_DLL}")
