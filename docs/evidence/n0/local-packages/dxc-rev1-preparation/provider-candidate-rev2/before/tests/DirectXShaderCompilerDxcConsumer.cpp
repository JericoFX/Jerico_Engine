/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winver.h>
#include <wrl/client.h>
#include <dxc/dxcapi.h>
#include <dxc/DxilHash/DxilHash.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace JE::DxcTest
{
    using Microsoft::WRL::ComPtr;
    void Require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }
    void Hr(HRESULT result, const char* message) { Require(SUCCEEDED(result), message); }
    void FileVersion(const std::filesystem::path& dll)
    {
        DWORD unused = 0;
        const DWORD size = GetFileVersionInfoSizeW(dll.c_str(), &unused);
        Require(size > 0, "real DLL version resource");
        std::vector<BYTE> data(size);
        Require(GetFileVersionInfoW(dll.c_str(), 0, size, data.data()) != 0, "DLL version resource read");
        VS_FIXEDFILEINFO* fixed = nullptr;
        UINT length = 0;
        Require(VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&fixed), &length) != 0
            && fixed && length >= sizeof(*fixed), "fixed file version");
        Require(HIWORD(fixed->dwFileVersionMS) == 1 && LOWORD(fixed->dwFileVersionMS) == 8
            && HIWORD(fixed->dwFileVersionLS) == 2505 && LOWORD(fixed->dwFileVersionLS) == 0, "original numeric1.8.2505.0");
        struct Translation { WORD language; WORD codePage; };
        Translation* translation = nullptr;
        Require(VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void**>(&translation), &length) != 0
            && translation && length >= sizeof(Translation), "version resource translation");
        wchar_t query[96]{};
        swprintf_s(query, L"\\StringFileInfo\\%04x%04x\\ProductVersion", translation->language, translation->codePage);
        wchar_t* product = nullptr;
        Require(VerQueryValueW(data.data(), query, reinterpret_cast<void**>(&product), &length) != 0 && product,
            "product version text");
        Require(std::wstring(product).find(L"1cc0ab76dafe08a4144e419e619a8951a3d219db") != std::wstring::npos,
            "full archive source pin in product version");
    }
    class Module
    {
    public:
        explicit Module(const std::filesystem::path& path)
            : handle(LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32))
        {
            Require(handle != nullptr, "LoadLibraryExW explicit package DLL");
            const FARPROC raw = GetProcAddress(handle, "DxcCreateInstance");
            Require(raw != nullptr, "DxcCreateInstance export");
            static_assert(sizeof(create) == sizeof(raw));
            std::memcpy(&create, &raw, sizeof(create));
        }
        ~Module() { FreeLibrary(handle); }
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;
        DxcCreateInstanceProc create{};
    private:
        HMODULE handle{};
    };
    void Provenance(IDxcCompiler3* compiler)
    {
        ComPtr<IDxcVersionInfo2> info;
        Hr(compiler->QueryInterface(IID_PPV_ARGS(&info)), "IDxcVersionInfo2 retained");
        UINT32 major = 0, minor = 0, count = 0;
        Hr(info->GetVersion(&major, &minor), "compiler version HRESULT");
        Require(major == 1 && minor == 8, "compiler1.8 API version");
        char* hash = nullptr;
        Hr(info->GetCommitInfo(&count, &hash), "commit info HRESULT");
        const bool exact = hash && std::strcmp(hash, "1cc0ab76") == 0 && count == 4954;
        CoTaskMemFree(hash);
        Require(exact, "exact8 source hash/API-history cardinality and ownership");
    }
    void RetailHash(IDxcBlob* object)
    {
        const auto* bytes = static_cast<const BYTE*>(object->GetBufferPointer());
        const SIZE_T size = object->GetBufferSize();
        Require(size >= 32 && size <= UINT32_MAX && std::memcmp(bytes, "DXBC", 4) == 0, "real DXIL container header");
        UINT32 declaredSize = 0;
        std::memcpy(&declaredSize, bytes + 24, sizeof(declaredSize));
        Require(declaredSize == size, "DXIL container byte count");
        std::array<BYTE, DXIL_CONTAINER_HASH_SIZE> digest{};
        // Reuse the pinned upstream retail algorithm as the oracle for the actual DLL output.
        ComputeHashRetail(bytes + 20, static_cast<UINT32>(size - 20), digest.data());
        Require(std::memcmp(digest.data(), bytes + 4, digest.size()) == 0, "retail DXIL hash, no preview/bypass hash");
        Require(digest != std::array<BYTE, DXIL_CONTAINER_HASH_SIZE>{}, "nonzero retail hash");
    }
    void Validate(Module& validator, IDxcBlob* object)
    {
        ComPtr<IDxcValidator> api;
        Hr(validator.create(CLSID_DxcValidator, IID_PPV_ARGS(&api)), "external source-built dxil validator");
        ComPtr<IDxcOperationResult> operation;
        Hr(api->Validate(object, DxcValidatorFlags_Default, &operation), "validator invocation HRESULT");
        HRESULT status = E_FAIL;
        Hr(operation->GetStatus(&status), "validator operation GetStatus");
        Require(SUCCEEDED(status), "actual DXIL validation operation status");
        ComPtr<IDxcBlob> validated;
        Hr(operation->GetResult(&validated), "validator result object");
        Require(validated.Get() != nullptr, "validator returned actual object");
        RetailHash(validated.Get());
    }
    void Write(IDxcBlob* object, const std::filesystem::path& path)
    {
        Require(!std::filesystem::exists(path), "consumer output already exists");
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream(path, std::ios::binary);
        stream.write(static_cast<const char*>(object->GetBufferPointer()), static_cast<std::streamsize>(object->GetBufferSize()));
        Require(stream.good(), "write actual compiled object");
    }
    void Spirv(IDxcBlob* object)
    {
        const SIZE_T bytes = object->GetBufferSize();
        Require(bytes >= 20 && bytes % 4 == 0, "SPIR-V word layout");
        std::vector<UINT32> words(bytes / 4);
        std::memcpy(words.data(), object->GetBufferPointer(), bytes);
        Require(words[0] == 0x07230203 && words[1] >= 0x00010000 && words[1] <= 0x00010300 && words[3] > 0 && words[4] == 0,
            "real Vulkan1.1 SPIR-V header");
        bool compute = false;
        for (size_t index = 5; index < words.size();)
        {
            const UINT32 length = words[index] >> 16;
            const UINT32 opcode = words[index] & 0xffff;
            Require(length > 0 && index + length <= words.size(), "valid SPIR-V instruction bounds");
            if (opcode == 15 && length >= 4 && words[index + 1] == 5)
            {
                const char* name = reinterpret_cast<const char*>(&words[index + 3]);
                const SIZE_T remaining = (length - 3) * 4;
                Require(std::memchr(name, 0, remaining) != nullptr, "terminated SPIR-V entry point");
                compute = std::strcmp(name, "main") == 0;
            }
            index += length;
        }
        Require(compute, "SPIR-V compute main entry point");
    }
    void Run(const std::wstring& mode, const std::filesystem::path& bin, const std::filesystem::path& output,
        const std::filesystem::path& includes)
    {
        Module compilerDll(bin / L"dxcompiler.dll");
        Module validatorDll(bin / L"dxil.dll");
        FileVersion(bin / L"dxcompiler.dll");
        ComPtr<IDxcCompiler3> compiler;
        ComPtr<IDxcUtils> utils;
        Hr(compilerDll.create(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)), "real compiler API");
        Hr(compilerDll.create(CLSID_DxcUtils, IID_PPV_ARGS(&utils)), "real utils API");
        Provenance(compiler.Get());
        if (mode == L"patched")
        {
            ComPtr<IDxcBlobEncoding> blob;
            Hr(utils->LoadFile((output / L"patched.dxil").c_str(), nullptr, &blob), "load real dxsc output");
            RetailHash(blob.Get());
            Validate(validatorDll, blob.Get());
            return;
        }
        constexpr const char* valid = "#include \"JE_DxcFixture.hlsli\"\nRWByteAddressBuffer output : register(u0);\n"
            "[numthreads(1,1,1)] void main(uint3 id : SV_DispatchThreadID) { volatile uint specialization = 7; output.Store(id.x * 4, specialization + JE_VALUE); }\n";
        constexpr const char* invalid = "[numthreads(1,1,1)] void main( { this is malformed HLSL; }";
        const char* text = mode == L"invalid" ? invalid : valid;
        DxcBuffer source{text, std::strlen(text), DXC_CP_UTF8};
        const std::wstring includeDirectory = includes.wstring();
        std::vector<LPCWSTR> args{L"-E", L"main", L"-T", L"cs_6_0", L"-I", includeDirectory.c_str()};
        if (mode == L"spirv") { args.push_back(L"-spirv"); args.push_back(L"-fspv-target-env=vulkan1.1"); }
        ComPtr<IDxcIncludeHandler> handler;
        Hr(utils->CreateDefaultIncludeHandler(&handler), "real include handler");
        ComPtr<IDxcResult> result;
        Hr(compiler->Compile(&source, args.data(), static_cast<UINT32>(args.size()), handler.Get(), IID_PPV_ARGS(&result)), "compile invocation HRESULT");
        HRESULT status = E_FAIL;
        Hr(result->GetStatus(&status), "compiler operation GetStatus");
        if (mode == L"invalid")
        {
            ComPtr<IDxcBlobUtf8> errors;
            Hr(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr), "invalid shader diagnostics");
            Require(FAILED(status) && errors.Get() != nullptr && errors->GetStringLength() > 0 && !result->HasOutput(DXC_OUT_OBJECT), "malformed shader rejected with diagnostics and no object");
            return;
        }
        if (FAILED(status))
        {
            ComPtr<IDxcBlobUtf8> errors;
            Hr(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr), "valid shader failure diagnostics");
            if (errors.Get() != nullptr) { std::fprintf(stderr, "%s\n", errors->GetStringPointer()); }
        }
        Require(SUCCEEDED(status), "valid HLSL compile operation status");
        ComPtr<IDxcBlob> object;
        Hr(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&object), nullptr), "actual compiled object");
        Require(object.Get() != nullptr, "compiled object present");
        if (mode == L"spirv") { Spirv(object.Get()); Write(object.Get(), output / L"fixture.spv"); }
        else { RetailHash(object.Get()); Validate(validatorDll, object.Get()); Write(object.Get(), output / L"fixture.dxil"); }
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        JE::DxcTest::Require(argc == 5, "usage: consumer dxil|spirv|invalid|patched package-bin output local-includes");
        const std::wstring mode(argv[1]);
        JE::DxcTest::Require(mode == L"dxil" || mode == L"spirv" || mode == L"invalid" || mode == L"patched", "known consumer case");
        JE::DxcTest::Run(mode, argv[2], argv[3], argv[4]);
        std::printf("CASE PASS: actual DXC API/provenance/validation\n");
        return 0;
    }
    catch (const std::exception& error) { std::fprintf(stderr, "CASE FAIL: %s\n", error.what()); return 1; }
}
