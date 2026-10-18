# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../../Python/JERequireLocalPath.cmake")
JE_python_require_local_path(JE_MCPP_PACKAGE_ROOT)
JE_python_require_local_path(CMAKE_BINARY_DIR)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${CMAKE_BINARY_DIR}" NORMALIZE JE_BUILD_IN_CHECKOUT)
if(NOT JE_BUILD_IN_CHECKOUT OR CMAKE_BINARY_DIR STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE mcpp consumer build must be beneath this checkout/build")
endif()
if(NOT EXISTS "${JE_MCPP_PACKAGE_ROOT}/Findmcpp.cmake")
    message(FATAL_ERROR "JE_MCPP_PACKAGE_ROOT must contain the extracted JE local Findmcpp.cmake")
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Consume the same Release DLL in both configurations" FORCE)
project(JELocalMcppConsumer LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "JE mcpp consumer requires Windows MSVC x64")
endif()
list(PREPEND CMAKE_MODULE_PATH "${JE_MCPP_PACKAGE_ROOT}")
find_package(mcpp 2.7.2 EXACT REQUIRED MODULE)
if(NOT mcpp_JE_PREOPEN_ABI_VERSION EQUAL 1)
    message(FATAL_ERROR "JE mcpp consumer requires the coherent pre-open ABI v1 package")
endif()
if(NOT mcpp_VARIANT STREQUAL "2.7.2_az.2"
    OR NOT mcpp_LIBRARY STREQUAL "${JE_MCPP_PACKAGE_ROOT}/mcpp/lib/mcpp0.lib")
    message(FATAL_ERROR "JE mcpp consumer resolved a different variant/library")
endif()
add_executable(JEMcppConsumer McppConsumer.cpp)
target_compile_features(JEMcppConsumer PRIVATE cxx_std_17)
target_compile_options(JEMcppConsumer PRIVATE /W4 /WX)
target_link_libraries(JEMcppConsumer PRIVATE 3rdParty::mcpp)
add_custom_command(TARGET JEMcppConsumer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${mcpp_RUNTIME_DEPENDENCIES}" "$<TARGET_FILE_DIR:JEMcppConsumer>")

set(JE_INPUT "${CMAKE_CURRENT_BINARY_DIR}/fixtures")
file(MAKE_DIRECTORY "${JE_INPUT}/include")
file(WRITE "${JE_INPUT}/include/nested.h" "#define JE_BASE 40\n")
file(WRITE "${JE_INPUT}/include/shader_include.h"
    "#include \"nested.h\"\n#define JE_ADD(x,y) ((x)+(y))\n#define JE_CAT(x,y) x ## y\n#define JE_STR(x) #x\n")
file(WRITE "${JE_INPUT}/shader.azsl"
    "#include \"shader_include.h\"\n#if JE_FEATURE != 2\n#error feature mismatch\n#endif\n"
    "int JE_CAT(shader_,value) = JE_ADD(JE_BASE,JE_FEATURE);\nconst char* label = JE_STR(jerico);\n"
    "float4 JE_SHADER = float4(1,2,3,4);\n")
file(WRITE "${JE_INPUT}/comments.azsl" "// first comment\n  // second comment\nfloat marker = 7;\n")
file(WRITE "${JE_INPUT}/missing.azsl" "#include \"je_missing_header.h\"\n")
file(WRITE "${JE_INPUT}/error.azsl" "#error JE_expected_error\n")
file(WRITE "${JE_INPUT}/unclosed.azsl" "#if 1\nfloat never_complete = 1;\n")
file(WRITE "${JE_INPUT}/reset.azsl" "#ifdef JE_FEATURE\n#error macro leaked from previous invocation\n#endif\nint recovered = 9;\n")
file(WRITE "${JE_INPUT}/resolver.azsl"
    "#define JE_HEADER <je_callback/native.h>\n#include JE_HEADER\nint resolved = JE_BASE;\n")
enable_testing()
foreach(JE_CASE IN ITEMS preprocessing comments errors include-resolution)
    add_test(NAME JE.LocalPackages.Mcpp.${JE_CASE} COMMAND JEMcppConsumer "${JE_CASE}" "${JE_INPUT}")
    set_tests_properties(JE.LocalPackages.Mcpp.${JE_CASE} PROPERTIES TIMEOUT 30)
endforeach()
add_test(NAME JE.LocalPackages.Mcpp.cli COMMAND "${mcpp_EXECUTABLE}" "${JE_INPUT}/shader.azsl"
    "-I${JE_INPUT}/include" -DJE_FEATURE=2 -P)
set_tests_properties(JE.LocalPackages.Mcpp.cli PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "shader_value")
