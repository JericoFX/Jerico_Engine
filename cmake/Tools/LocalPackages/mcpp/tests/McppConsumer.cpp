/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

// Match Atom's McppBinder import and callback interface, including the O3DE patch.
#define MCPP_DLL_IMPORT 1
#define MCPP_DONT_USE_SHORT_NAMES 1
#include <mcpp_lib.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    std::array<std::string, MCPP_NUM_OUTDEST> output;
    std::vector<std::string> includes;

    int Putc(int value, MCPP_OUTDEST destination)
    {
        output[static_cast<size_t>(destination)].push_back(static_cast<char>(value));
        return value;
    }

    int Puts(const char* text, MCPP_OUTDEST destination)
    {
        output[static_cast<size_t>(destination)] += text;
        return static_cast<int>(std::char_traits<char>::length(text));
    }

    int Printf(MCPP_OUTDEST destination, const char* format, ...)
    {
        va_list arguments;
        va_start(arguments, format);
        va_list copy;
        va_copy(copy, arguments);
        const int length = std::vsnprintf(nullptr, 0, format, copy);
        va_end(copy);
        if (length < 0)
        {
            va_end(arguments);
            return length;
        }
        std::vector<char> buffer(static_cast<size_t>(length) + 1);
        const int written = std::vsnprintf(buffer.data(), buffer.size(), format, arguments);
        va_end(arguments);
        if (written >= 0)
        {
            output[static_cast<size_t>(destination)].append(buffer.data(), static_cast<size_t>(written));
        }
        return written;
    }

    void Include(FILE* file, const char*, const char*, const char* path)
    {
        if (file && path)
        {
            includes.emplace_back(path);
        }
    }

    std::string Normalize(std::string text)
    {
        std::replace(text.begin(), text.end(), '\\', '/');
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch)
        {
            return static_cast<char>(std::tolower(ch));
        });
        return text;
    }

    std::string Compact(const std::string& text)
    {
        std::string result;
        for (unsigned char ch : text)
        {
            if (!std::isspace(ch))
            {
                result.push_back(static_cast<char>(ch));
            }
        }
        return result;
    }

    int Run(std::vector<std::string> arguments)
    {
        output = {};
        includes.clear();
        mcpp_set_report_include_callback(Include);
        mcpp_set_out_func(Putc, Puts, Printf);
        std::vector<const char*> argv;
        for (const auto& argument : arguments)
        {
            argv.push_back(argument.c_str());
        }
        const int argc = static_cast<int>(argv.size());
        argv.push_back(nullptr);
        const int result = mcpp_lib_main(argc, argv.data());
        mcpp_reset_def_out_func();
        mcpp_set_report_include_callback(nullptr);
        return result;
    }

    bool Require(bool condition, const char* description)
    {
        if (!condition)
        {
            std::cerr << description << "\n" << output[MCPP_ERR] << "\n" << output[MCPP_OUT] << "\n";
        }
        return condition;
    }

    bool Preprocessing(const std::filesystem::path& root)
    {
        if (!Require(Run({"builder", (root / "shader.azsl").generic_string(),
            "-I" + (root / "include").generic_string(), "-DJE_FEATURE=2"}) == 0, "preprocessing failed"))
        {
            return false;
        }
        const std::string code = Compact(output[MCPP_OUT]);
        bool valid = Require(code.find("intshader_value=((40)+(2));") != std::string::npos, "macro expansion/token pasting differs")
            && Require(code.find("constchar*label=\"jerico\";") != std::string::npos, "stringizing differs")
            && Require(code.find("float4JE_SHADER=float4(1,2,3,4);") != std::string::npos, "shader tokens were lost");
        const std::string normalized = Normalize(output[MCPP_OUT]);
        valid &= Require(normalized.rfind("#line", 0) == 0, "Atom requires the initial #line directive");
        for (const char* name : {"shader_include.h", "nested.h"})
        {
            const std::string expected = Normalize((root / "include" / name).generic_string());
            valid &= Require(std::any_of(includes.begin(), includes.end(), [&](const std::string& actual)
            {
                return Normalize(actual) == expected;
            }), "include callback did not report the full local path");
            valid &= Require(normalized.find('"' + expected + '"') != std::string::npos, "include #line origin is not absolute");
        }
        return valid;
    }

    bool Comments(const std::filesystem::path& root)
    {
        if (!Require(Run({"builder", (root / "comments.azsl").generic_string(), "-C", "-P"}) == 0, "comment preprocessing failed"))
        {
            return false;
        }
        const auto& code = output[MCPP_OUT];
        return Require(code.find("// first comment\n") != std::string::npos, "first comment lost")
            && Require(code.find("// second comment\n") != std::string::npos, "second comment lost")
            && Require(code.find("// first comment\n\n") == std::string::npos, "extra line after first comment")
            && Require(code.find("// second comment\n\n") == std::string::npos, "extra line after second comment")
            && Require(std::count(code.begin(), code.end(), '\n') == 3, "_az.2 comment/source line count differs")
            && Require(Compact(code).find("floatmarker=7;") != std::string::npos, "code after comments lost");
    }

    bool Errors(const std::filesystem::path& root)
    {
        for (const char* name : {"missing.azsl", "error.azsl", "unclosed.azsl"})
        {
            if (!Require(Run({"builder", (root / name).generic_string(), "-DJE_FEATURE=2"}) != 0,
                "invalid shader input was accepted") || !Require(!output[MCPP_ERR].empty(), "error diagnostic missing"))
            {
                return false;
            }
            if (!Require(Run({"builder", (root / "reset.azsl").generic_string(), "-P"}) == 0,
                "invocation after error did not reset macros/state")
                || !Require(Compact(output[MCPP_OUT]).find("intrecovered=9;") != std::string::npos, "recovery output differs"))
            {
                return false;
            }
        }
        return true;
    }
}

int main(int argc, char** argv)
{
    if (argc != 3 || !std::filesystem::path(argv[2]).is_absolute())
    {
        std::cerr << "Expected test case and an absolute local fixture directory\n";
        return 2;
    }
    const std::string test = argv[1];
    const std::filesystem::path root(argv[2]);
    const bool valid = test == "preprocessing" ? Preprocessing(root) : test == "comments" ? Comments(root)
        : test == "errors" ? Errors(root) : false;
    std::cout << "JE mcpp " << test << (valid ? " PASS\n" : " FAIL\n");
    return valid ? 0 : 1;
}
