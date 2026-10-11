# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Adapted from o3de/3p-package-source@70f0b7b5; original Release deployment.

if(TARGET 3rdParty::vulkan-validationlayers)
    return()
endif()
include("${CMAKE_CURRENT_LIST_DIR}/JERequireLocalPath.cmake")
set(JE_VVL_ROOT "${CMAKE_CURRENT_LIST_DIR}")
JE_python_require_local_path(JE_VVL_ROOT)
set(vulkan-validationlayers_VERSION "1.3.261")
foreach(JE_CONFIG IN ITEMS release debug)
    set(JE_PAIR "${JE_VVL_ROOT}/vulkan-validationlayers/lib/${JE_CONFIG}")
    JE_python_require_local_path(JE_PAIR)
    foreach(JE_FILE IN ITEMS VkLayer_khronos_validation.dll VkLayer_khronos_validation.json)
        set(JE_PAYLOAD "${JE_PAIR}/${JE_FILE}")
        JE_python_require_local_path(JE_PAYLOAD)
        if(NOT EXISTS "${JE_PAYLOAD}" OR IS_DIRECTORY "${JE_PAYLOAD}")
            message(FATAL_ERROR "JE VVL 1.3.261: required local ${JE_CONFIG} payload is missing: ${JE_PAYLOAD}")
        endif()
    endforeach()
    file(READ "${JE_PAIR}/VkLayer_khronos_validation.json" JE_MANIFEST)
    string(JSON JE_NAME GET "${JE_MANIFEST}" layer name)
    string(JSON JE_API GET "${JE_MANIFEST}" layer api_version)
    string(JSON JE_LIBRARY GET "${JE_MANIFEST}" layer library_path)
    if(NOT JE_NAME STREQUAL "VK_LAYER_KHRONOS_validation" OR NOT JE_API STREQUAL "1.3.261"
        OR NOT JE_LIBRARY STREQUAL ".\\VkLayer_khronos_validation.dll")
        message(FATAL_ERROR "JE VVL: manifest identity/relative DLL path differs from the original layer")
    endif()
endforeach()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(vulkan-validationlayers REQUIRED_VARS JE_VVL_ROOT
    VERSION_VAR vulkan-validationlayers_VERSION)
add_library(3rdParty::vulkan-validationlayers INTERFACE IMPORTED GLOBAL)
set(vulkan-validationlayers_RUNTIME_JSON_DEPENDENCIES
    "${JE_VVL_ROOT}/vulkan-validationlayers/lib/release/VkLayer_khronos_validation.json")
set(JE_VVL_RUNTIME_DLL "${JE_VVL_ROOT}/vulkan-validationlayers/lib/release/VkLayer_khronos_validation.dll")
if(COMMAND ly_add_target_files)
    ly_add_target_files(TARGETS 3rdParty::vulkan-validationlayers
        FILES "${vulkan-validationlayers_RUNTIME_JSON_DEPENDENCIES}" "${JE_VVL_RUNTIME_DLL}")
endif()
set_property(TARGET 3rdParty::vulkan-validationlayers PROPERTY JE_RUNTIME_FILES
    "${vulkan-validationlayers_RUNTIME_JSON_DEPENDENCIES};${JE_VVL_RUNTIME_DLL}")
