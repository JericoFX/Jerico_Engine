/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

extern "C"
{
#include <Lua/lua.h>
#include <Lua/lauxlib.h>
#include <Lua/lualib.h>
}

static_assert(LUA_VERSION_RELEASE_NUM == 50404, "The consumer requires Lua 5.4.4");
static_assert(sizeof(lua_Integer) == 8, "The engine Lua integer ABI must remain 64-bit");
static_assert(std::is_same_v<lua_Number, double>, "The engine Lua number ABI must remain double");

namespace
{
    struct Allocations
    {
        size_t liveBlocks = 0;
        size_t allocations = 0;
        size_t reallocations = 0;
        size_t frees = 0;
    };

    void* Allocate(void* context, void* pointer, size_t, size_t size)
    {
        auto& stats = *static_cast<Allocations*>(context);
        if (size == 0)
        {
            if (pointer)
            {
                --stats.liveBlocks;
                ++stats.frees;
            }
            std::free(pointer);
            return nullptr;
        }
        const bool wasNew = pointer == nullptr;
        void* result = std::realloc(pointer, size);
        if (result)
        {
            if (wasNew)
            {
                ++stats.liveBlocks;
                ++stats.allocations;
            }
            else
            {
                ++stats.reallocations;
            }
        }
        return result;
    }

    bool Api(lua_State* state)
    {
        constexpr const char* script =
            "local t = {}; for i=1,100 do t[i]=i end; "
            "local function sum(a) local s=0; for _,v in ipairs(a) do s=s+v end; return s end; "
            "return sum(t), 1.25 * 2, _VERSION";
        if (luaL_loadbuffer(state, script, std::strlen(script), "JE-api") != LUA_OK
            || lua_pcall(state, 0, 3, 0) != LUA_OK)
        {
            return false;
        }
        const char* version = lua_tostring(state, -1);
        const bool results = lua_isinteger(state, -3) && lua_tointeger(state, -3) == 5050
            && lua_tonumber(state, -2) == 2.5 && version && std::strcmp(version, "Lua 5.4") == 0;
        lua_settop(state, 0);
        constexpr const char* syntaxError = "local =";
        const bool syntaxRejected = luaL_loadbuffer(state, syntaxError, std::strlen(syntaxError), "JE-bad") == LUA_ERRSYNTAX;
        lua_settop(state, 0);
        constexpr const char* runtimeError = "error('JE expected error')";
        const bool runtimeRejected = luaL_loadbuffer(state, runtimeError, std::strlen(runtimeError), "JE-error") == LUA_OK
            && lua_pcall(state, 0, 0, 0) == LUA_ERRRUN;
        const char* error = lua_tostring(state, -1);
        const bool message = error && std::strstr(error, "JE expected error");
        lua_settop(state, 0);
        const bool recovered = luaL_dostring(state, "return 6 * 7") == LUA_OK
            && lua_tointeger(state, -1) == 42;
        lua_settop(state, 0);
        return results && syntaxRejected && runtimeRejected && message && recovered;
    }

    struct Bytecode
    {
        std::array<unsigned char, 65536> bytes{};
        size_t size = 0;
    };

    int WriteBytecode(lua_State*, const void* bytes, size_t size, void* context)
    {
        auto& code = *static_cast<Bytecode*>(context);
        if (size > code.bytes.size() - code.size)
        {
            return 1;
        }
        std::memcpy(code.bytes.data() + code.size, bytes, size);
        code.size += size;
        return 0;
    }

    bool Roundtrip(lua_State* state)
    {
        constexpr const char* script = "local v={10,20,12}; return v[1]+v[2]+v[3]";
        Bytecode code;
        if (luaL_loadbuffer(state, script, std::strlen(script), "JE-bytecode") != LUA_OK
            || lua_dump(state, WriteBytecode, &code, 0) != 0 || code.size < 4)
        {
            return false;
        }
        lua_settop(state, 0);
        if (luaL_loadbufferx(state, reinterpret_cast<const char*>(code.bytes.data()), code.size, "JE-reload", "b") != LUA_OK
            || lua_pcall(state, 0, 1, 0) != LUA_OK || lua_tointeger(state, -1) != 42)
        {
            return false;
        }
        lua_settop(state, 0);
        code.bytes[0] = 0; // A corrupt signature must not be accepted as a binary chunk.
        const bool corruptRejected = luaL_loadbufferx(state, reinterpret_cast<const char*>(code.bytes.data()),
            code.size, "JE-corrupt", "b") == LUA_ERRSYNTAX;
        lua_settop(state, 0);
        return corruptRejected;
    }

    bool WindowsPatch(lua_State* state, const char* missingDll)
    {
        lua_getglobal(state, "package");
        lua_getfield(state, -1, "loadlib");
        lua_remove(state, -2);
        lua_pushstring(state, missingDll);
        lua_pushliteral(state, "luaopen_JE_missing");
        if (lua_pcall(state, 2, 3, 0) != LUA_OK || !lua_isnil(state, -3))
        {
            return false;
        }
        const char* error = lua_tostring(state, -2);
        const char* where = lua_tostring(state, -1);
        constexpr const char* prefix = "system error ";
        if (!error || !where || std::strcmp(where, "open") != 0
            || std::strncmp(error, prefix, std::strlen(prefix)) != 0)
        {
            return false; // Unpatched Lua formats a localized Windows message instead.
        }
        const char* digit = error + std::strlen(prefix);
        if (*digit < '0' || *digit > '9')
        {
            return false;
        }
        while (*digit >= '0' && *digit <= '9')
        {
            ++digit;
        }
        const bool numericError = digit[0] == '\n' && digit[1] == '\0';
        lua_settop(state, 0);
        return numericError;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2 || std::strcmp(LUA_RELEASE, "Lua 5.4.4") != 0)
    {
        return 2;
    }
    Allocations stats;
    lua_State* state = lua_newstate(Allocate, &stats);
    if (!state)
    {
        return 3;
    }
    luaL_openlibs(state);
    bool passed = false;
    if (std::strcmp(argv[1], "api") == 0)
    {
        passed = Api(state);
    }
    else if (std::strcmp(argv[1], "bytecode") == 0)
    {
        passed = Roundtrip(state);
    }
    else if (std::strcmp(argv[1], "patch") == 0 && argc == 3)
    {
        passed = WindowsPatch(state, argv[2]);
    }
    passed = passed && lua_version(state) == LUA_VERSION_NUM;
    lua_close(state);
    passed = passed && stats.liveBlocks == 0 && stats.allocations > 0 && stats.frees == stats.allocations;
    std::printf("JE Lua %s: %s (allocations=%zu, reallocations=%zu, live=%zu)\n",
        argv[1], passed ? "PASS" : "FAIL", stats.allocations, stats.reallocations, stats.liveBlocks);
    return passed ? 0 : 1;
}
