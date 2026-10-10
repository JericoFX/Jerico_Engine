# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Adapted from o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18,
# package-system/python/win_x64/python-config.cmake.

include("${CMAKE_CURRENT_LIST_DIR}/JERequireLocalPath.cmake")
set(JE_PYTHON_CONFIG_ROOT "${CMAKE_CURRENT_LIST_DIR}")
JE_python_require_local_path(JE_PYTHON_CONFIG_ROOT)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "JE local Python package requires Windows x64")
endif()
set(Python_VERSION 3.10.13)
set(Python_INTERPRETER_ID Python)
set(Python_EXECUTABLE "${JE_PYTHON_CONFIG_ROOT}/python/python.exe")
set(Python_HOME "${JE_PYTHON_CONFIG_ROOT}/python")
set(Python_PATHS "${Python_HOME}/Lib" "${Python_HOME}/Lib/site-packages" "${Python_HOME}/DLLs")
set(Python_LIBRARY_DEBUG "${Python_HOME}/libs/python310_d.lib")
set(Python_LIBRARY_RELEASE "${Python_HOME}/libs/python310.lib")
set(Python_INCLUDE_DIR "${Python_HOME}/include")
set(Python_DYLIBS_DEBUG "${Python_HOME}/python310_d.dll" "${Python_HOME}/python3_d.dll"
    "${Python_HOME}/python310_d.pdb" "${Python_HOME}/python3_d.pdb")
set(Python_DYLIBS_RELEASE "${Python_HOME}/python310.dll" "${Python_HOME}/python3.dll"
    "${Python_HOME}/python310.pdb" "${Python_HOME}/python3.pdb")
foreach(JE_REQUIRED_FILE IN ITEMS "${Python_EXECUTABLE}" "${Python_HOME}/python_d.exe"
    "${Python_LIBRARY_DEBUG}" "${Python_LIBRARY_RELEASE}" "${Python_INCLUDE_DIR}/Python.h"
    "${Python_INCLUDE_DIR}/pyconfig.h" "${Python_HOME}/Lib/venv/__init__.py"
    "${Python_HOME}/Lib/ensurepip/__init__.py" ${Python_DYLIBS_DEBUG} ${Python_DYLIBS_RELEASE})
    JE_python_require_local_path(JE_REQUIRED_FILE)
    if(NOT EXISTS "${JE_REQUIRED_FILE}" OR IS_DIRECTORY "${JE_REQUIRED_FILE}")
        message(FATAL_ERROR "JE local Python package file is missing: ${JE_REQUIRED_FILE}")
    endif()
endforeach()
set(Python_Interpreter_FOUND TRUE)
set(Python_Development_FOUND TRUE)
set(Python_COMPILE_DEFINITIONS DEFAULT_LY_PYTHONHOME="${Python_HOME}")
set(Python_LIBRARY "$<IF:$<CONFIG:Debug>,${Python_LIBRARY_DEBUG},${Python_LIBRARY_RELEASE}>")
set(Python_DYLIBS "$<IF:$<CONFIG:Debug>,${Python_DYLIBS_DEBUG},${Python_DYLIBS_RELEASE}>")
if(NOT TARGET 3rdParty::Python)
    add_library(3rdParty::Python INTERFACE IMPORTED GLOBAL)
    if(COMMAND ly_target_include_system_directories)
        ly_target_include_system_directories(TARGET 3rdParty::Python INTERFACE "${Python_INCLUDE_DIR}")
    else()
        target_include_directories(3rdParty::Python SYSTEM INTERFACE "${Python_INCLUDE_DIR}")
    endif()
    target_link_libraries(3rdParty::Python INTERFACE "${Python_LIBRARY}")
    target_compile_definitions(3rdParty::Python INTERFACE "${Python_COMPILE_DEFINITIONS}")
    set_target_properties(3rdParty::Python PROPERTIES INTERFACE_IMPORTED_LOCATION "${Python_DYLIBS}")
endif()
set(Python_FOUND TRUE)
