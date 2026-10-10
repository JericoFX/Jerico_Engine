# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../JERequireLocalPath.cmake")
JE_python_require_local_path(JE_PYTHON_PACKAGE_ROOT)
JE_python_require_local_path(CMAKE_BINARY_DIR)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${CMAKE_BINARY_DIR}" NORMALIZE JE_IN_BUILD)
if(NOT JE_IN_BUILD OR CMAKE_BINARY_DIR STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local Python consumer outputs must be beneath this checkout/build")
endif()
if(NOT EXISTS "${JE_PYTHON_PACKAGE_ROOT}/FindPython.cmake")
    message(FATAL_ERROR "JE local Python consumer requires the explicitly extracted package")
endif()
project(JELocalPythonConsumer LANGUAGES CXX)
list(PREPEND CMAKE_MODULE_PATH "${JE_PYTHON_PACKAGE_ROOT}")
find_package(Python 3.10.13 EXACT REQUIRED MODULE)
if(NOT TARGET 3rdParty::Python OR NOT Python_VERSION STREQUAL "3.10.13"
    OR NOT Python_EXECUTABLE STREQUAL "${JE_PYTHON_PACKAGE_ROOT}/python/python.exe")
    message(FATAL_ERROR "JE local Python consumer resolved a different package")
endif()
add_executable(JEPythonEmbed PythonEmbed.cpp)
target_compile_features(JEPythonEmbed PRIVATE cxx_std_17)
target_link_libraries(JEPythonEmbed PRIVATE 3rdParty::Python)
if(MSVC)
    target_compile_options(JEPythonEmbed PRIVATE /W4 /WX)
endif()
foreach(JE_DLL_NAME IN ITEMS python310 python3)
    add_custom_command(TARGET JEPythonEmbed POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${Python_HOME}/${JE_DLL_NAME}$<IF:$<CONFIG:Debug>,_d,>.dll" "$<TARGET_FILE_DIR:JEPythonEmbed>")
endforeach()
enable_testing()
add_test(NAME JE.LocalPackages.PythonEmbed COMMAND JEPythonEmbed)
foreach(JE_CONFIGURATION IN ITEMS Debug Release)
    add_test(NAME JE.LocalPackages.PythonRuntime${JE_CONFIGURATION}
        COMMAND "${CMAKE_COMMAND}" "-DJE_PYTHON_PACKAGE_ROOT=${JE_PYTHON_PACKAGE_ROOT}"
            "-DJE_RUNTIME_CONFIGURATION=${JE_CONFIGURATION}"
            "-DJE_VENV_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/venv-${JE_CONFIGURATION}"
            -P "${CMAKE_CURRENT_LIST_DIR}/RunRuntimeTest.cmake")
endforeach()
