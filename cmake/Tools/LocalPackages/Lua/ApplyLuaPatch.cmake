# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Exact hunks from o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18,
# package-system/Lua/lua_5.4.4_o3de.patch. No Git/patch executable or fuzzy matching.
include("${CMAKE_CURRENT_LIST_DIR}/../Python/JERequireLocalPath.cmake")
JE_python_require_local_path(JE_LUA_WORK)
get_filename_component(JE_PATCH_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_PATCH_BUILD_ROOT "${JE_PATCH_ENGINE_ROOT}/build")
cmake_path(IS_PREFIX JE_PATCH_BUILD_ROOT "${JE_LUA_WORK}" NORMALIZE JE_PATCH_IN_BUILD)
if(NOT JE_PATCH_IN_BUILD OR JE_LUA_WORK STREQUAL JE_PATCH_BUILD_ROOT)
    message(FATAL_ERROR "JE local Lua: patch work must be beneath this checkout/build")
endif()
function(JE_lua_exact_hunk JE_VARIABLE JE_LABEL JE_OLD JE_NEW)
    string(FIND "${${JE_VARIABLE}}" "${JE_OLD}" JE_OFFSET)
    if(JE_OFFSET EQUAL -1)
        message(FATAL_ERROR "JE local Lua: original patch hunk missing: ${JE_LABEL}")
    endif()
    string(LENGTH "${JE_OLD}" JE_LENGTH)
    math(EXPR JE_AFTER "${JE_OFFSET} + ${JE_LENGTH}")
    string(SUBSTRING "${${JE_VARIABLE}}" ${JE_AFTER} -1 JE_REMAINDER)
    string(FIND "${JE_REMAINDER}" "${JE_OLD}" JE_DUPLICATE)
    if(NOT JE_DUPLICATE EQUAL -1)
        message(FATAL_ERROR "JE local Lua: ambiguous patch hunk: ${JE_LABEL}")
    endif()
    string(REPLACE "${JE_OLD}" "${JE_NEW}" JE_PATCHED "${${JE_VARIABLE}}")
    set(${JE_VARIABLE} "${JE_PATCHED}" PARENT_SCOPE)
endfunction()

file(READ "${JE_LUA_WORK}/loadlib.c" JE_LOADLIB)
file(READ "${JE_LUA_WORK}/loslib.c" JE_LOSLIB)
JE_lua_exact_hunk(JE_LOADLIB "loadlib Windows error" [=[static void pusherror (lua_State *L) {
  int error = GetLastError();
  char buffer[128];
  if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, error, 0, buffer, sizeof(buffer)/sizeof(char), NULL))
    lua_pushstring(L, buffer);
  else
    lua_pushfstring(L, "system error %d\n", error);
}]=] [=[static void pusherror (lua_State *L) {
  int error = GetLastError();
#if LUMBERYARD_DISABLED
  char buffer[128];
  if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, error, 0, buffer, sizeof(buffer)/sizeof(char), NULL))
    lua_pushstring(L, buffer);
  else
#endif
    lua_pushfstring(L, "system error %d\n", error);
}]=])
JE_lua_exact_hunk(JE_LOSLIB "loslib Apple include" [=[#include "lauxlib.h"
#include "lualib.h"

]=] [=[#include "lauxlib.h"
#include "lualib.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
]=])
JE_lua_exact_hunk(JE_LOSLIB "loslib execute" [=[static int os_execute (lua_State *L) {
  const char *cmd = luaL_optstring(L, 1, NULL);
  int stat;
  errno = 0;
  stat = system(cmd);]=] [=[static int os_execute (lua_State *L) {
  #if defined(TARGET_OS_IPHONE)
    const char* cmd = NULL;
    int stat = -1;
#else
  const char *cmd = luaL_optstring(L, 1, NULL);
  int stat;
  errno = 0;
  stat = system(cmd);
#endif]=])
JE_lua_exact_hunk(JE_LOSLIB "loslib rename" [=[static int os_rename (lua_State *L) {
  const char *fromname = luaL_checkstring(L, 1);
  const char *toname = luaL_checkstring(L, 2);
  return luaL_fileresult(L, rename(fromname, toname) == 0, NULL);
}]=] [=[static int os_rename (lua_State *L) {
#if !defined(TARGET_OS_IPHONE) && !defined(ANDROID)
  const char *fromname = luaL_checkstring(L, 1);
  const char *toname = luaL_checkstring(L, 2);
  return luaL_fileresult(L, rename(fromname, toname) == 0, NULL);
#else
  luaL_error(L, "os_rename not implemented on this platform");
  return 1;
#endif
}]=])
JE_lua_exact_hunk(JE_LOSLIB "loslib tmpname" [=[static int os_tmpname (lua_State *L) {
  char buff[LUA_TMPNAMBUFSIZE];
  int err;
  lua_tmpnam(buff, err);
  if (l_unlikely(err))
    return luaL_error(L, "unable to generate a unique filename");
  lua_pushstring(L, buff);
  return 1;
}]=] [=[static int os_tmpname (lua_State *L) {
#if !defined(TARGET_OS_IPHONE) && !defined(ANDROID)
  char buff[LUA_TMPNAMBUFSIZE];
  int err;
  lua_tmpnam(buff, err);
  if (l_unlikely(err))
#endif
    return luaL_error(L, "unable to generate a unique filename");
#if !defined(TARGET_OS_IPHONE) && !defined(ANDROID)
  lua_pushstring(L, buff);
  return 1;
#endif
}]=])
# All five hunks must match before either freshly extracted work file is written.
file(WRITE "${JE_LUA_WORK}/loadlib.c" "${JE_LOADLIB}")
file(WRITE "${JE_LUA_WORK}/loslib.c" "${JE_LOSLIB}")
