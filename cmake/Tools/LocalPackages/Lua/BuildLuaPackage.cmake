# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Build only from the parent-audited tag archive. Work/source/build/stage/package
# outputs are new local paths; no acquisition, Git, global install or source mutation.
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
foreach(JE_ARGUMENT IN ITEMS SOURCE_ARCHIVE SOURCE_SHA256 SOURCE_URL OUTPUT)
    if(NOT DEFINED JE_LUA_${JE_ARGUMENT} OR "${JE_LUA_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local Lua: JE_LUA_${JE_ARGUMENT} is required")
    endif()
endforeach()
JE_python_require_local_path(JE_LUA_SOURCE_ARCHIVE)
JE_python_require_local_path(JE_LUA_OUTPUT)
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_BUILD_ROOT "${JE_LUA_OUTPUT}" NORMALIZE JE_OUTPUT_IN_BUILD)
if(NOT JE_OUTPUT_IN_BUILD OR JE_LUA_OUTPUT STREQUAL JE_BUILD_ROOT)
    message(FATAL_ERROR "JE local Lua: output must be beneath this checkout/build")
endif()
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local Lua recipe requires Windows x64 and Visual Studio 2022")
endif()
if(EXISTS "${JE_LUA_OUTPUT}")
    message(FATAL_ERROR "JE local Lua: output already exists: ${JE_LUA_OUTPUT}")
endif()
set(JE_EXPECTED_HASH "2ff89bbea22a7c8b910d6b006f5ba4559edcf468a25537cedf9988ed90f6b450")
string(TOLOWER "${JE_LUA_SOURCE_SHA256}" JE_SOURCE_HASH)
if(NOT JE_SOURCE_HASH STREQUAL JE_EXPECTED_HASH
    OR NOT JE_LUA_SOURCE_URL STREQUAL "https://codeload.github.com/lua/lua/tar.gz/refs/tags/v5.4.4")
    message(FATAL_ERROR "JE local Lua: source URL/SHA256 must match the audited v5.4.4 tag archive")
endif()
JE_python_verify_file("${JE_LUA_SOURCE_ARCHIVE}" "${JE_SOURCE_HASH}" "Lua source archive")
JE_python_verify_file("${CMAKE_CURRENT_LIST_DIR}/lua_5.4.4_o3de.patch"
    "5a8921ff20845030ccdfc3bc67f7797fe2339eb0e54a93c1089e748ea698120d" "original Lua patch")

set(JE_LUA_WORK "${JE_LUA_OUTPUT}/source-work/lua-5.4.4")
set(JE_BUILD "${JE_LUA_OUTPUT}/build")
set(JE_STAGE "${JE_LUA_OUTPUT}/stage")
file(MAKE_DIRECTORY "${JE_LUA_OUTPUT}/source-work")
file(ARCHIVE_EXTRACT INPUT "${JE_LUA_SOURCE_ARCHIVE}" DESTINATION "${JE_LUA_OUTPUT}/source-work")
foreach(JE_REQUIRED IN ITEMS onelua.c lua.h luaconf.h lauxlib.h lualib.h loadlib.c loslib.c)
    if(NOT EXISTS "${JE_LUA_WORK}/${JE_REQUIRED}")
        message(FATAL_ERROR "JE local Lua: pinned archive source file missing: ${JE_REQUIRED}")
    endif()
endforeach()
file(READ "${JE_LUA_WORK}/lua.h" JE_HEADER)
foreach(JE_COMPONENT IN ITEMS MAJOR MINOR RELEASE)
    if(JE_COMPONENT STREQUAL "MAJOR")
        set(JE_VERSION "5")
    else()
        set(JE_VERSION "4")
    endif()
    if(NOT JE_HEADER MATCHES "#define[ \t]+LUA_VERSION_${JE_COMPONENT}[ \t]+\"${JE_VERSION}\"")
        message(FATAL_ERROR "JE local Lua: source is not Lua 5.4.4")
    endif()
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/ApplyLuaPatch.cmake")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}" -B "${JE_BUILD}"
    -G "Visual Studio 17 2022" -A x64 -T v143 "-DJE_LUA_WORK:PATH=${JE_LUA_WORK}"
    RESULT_VARIABLE JE_CONFIGURE_RESULT)
if(NOT JE_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local Lua configure failed (${JE_CONFIGURE_RESULT}); outputs retained")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${JE_BUILD}" --target lualib --config Release
    RESULT_VARIABLE JE_BUILD_RESULT)
if(NOT JE_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local Lua build failed (${JE_BUILD_RESULT}); outputs retained")
endif()
file(MAKE_DIRECTORY "${JE_STAGE}/Lua/include/Lua" "${JE_STAGE}/Lua/lib/release"
    "${JE_STAGE}/LICENSES/Lua" "${JE_STAGE}/LICENSES/O3DE")
file(GLOB_RECURSE JE_HEADERS "${JE_LUA_WORK}/*.h")
file(COPY ${JE_HEADERS} DESTINATION "${JE_STAGE}/Lua/include/Lua")
file(COPY "${JE_BUILD}/Release/lualib.lib" DESTINATION "${JE_STAGE}/Lua/lib/release")
configure_file("${JE_LUA_WORK}/lua.h" "${JE_STAGE}/LICENSES/Lua/lua.h" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/LICENSE.txt" "${JE_STAGE}/Lua/LICENSE.txt" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" "${JE_STAGE}/NOTICE.txt" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/FindLua.cmake" "${JE_STAGE}/FindLua.cmake" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake"
    "${JE_STAGE}/JERequireLocalPath.cmake" COPYONLY)
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT"
    "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT" "${CMAKE_CURRENT_LIST_DIR}/lua_5.4.4_o3de.patch"
    DESTINATION "${JE_STAGE}/LICENSES/O3DE")
file(WRITE "${JE_STAGE}/Lua/JERecipe.json"
    "{\n  \"recipeCommit\": \"70f0b7b5257bf7cf935bf1cfd270e1b53ccead18\",\n"
    "  \"patchSHA256\": \"5a8921ff20845030ccdfc3bc67f7797fe2339eb0e54a93c1089e748ea698120d\",\n"
    "  \"toolset\": \"v143\", \"originalGenerator\": \"Visual Studio 16 2019\",\n"
    "  \"configuration\": \"Release\", \"runtimeLibrary\": \"MultiThreadedDLL\"\n}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_NAME "Lua-5.4.4-je-rev1-windows")
set(JE_PACKAGE_OUTPUT "${JE_LUA_OUTPUT}/packages")
set(JE_PACKAGE_VERSION "5.4.4-je-rev1")
set(JE_PACKAGE_SOURCE_COMMIT "5d708c3f9cae12820e415d4f89c9eacbe2ab964b")
set(JE_PACKAGE_LICENSE "MIT")
set(JE_PACKAGE_SOURCE_SHA256 "${JE_SOURCE_HASH}")
set(JE_PACKAGE_SOURCE_URL "${JE_LUA_SOURCE_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
