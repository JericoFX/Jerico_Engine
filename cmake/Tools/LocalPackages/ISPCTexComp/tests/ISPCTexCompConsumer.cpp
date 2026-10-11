// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ISPC/ispc_texcomp.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace JE::ISPCTexCompTests
{
    void Require(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void Exports()
    {
        const HMODULE library = GetModuleHandleW(L"ispc_texcomp.dll");
        Require(library != nullptr, "Original DLL import was not resolved");
        std::array<wchar_t, 32768> modulePath{};
        std::array<wchar_t, 32768> exePath{};
        Require(GetModuleFileNameW(library, modulePath.data(), static_cast<DWORD>(modulePath.size())) != 0,
            "DLL path unavailable");
        Require(GetModuleFileNameW(nullptr, exePath.data(), static_cast<DWORD>(exePath.size())) != 0,
            "Executable path unavailable");
        Require(std::filesystem::path(modulePath.data()).parent_path() == std::filesystem::path(exePath.data()).parent_path(),
            "Consumer loaded a DLL outside its explicit package copy");
        const char* exports[] = {
            "CompressBlocksBC1", "CompressBlocksBC3", "CompressBlocksBC6H", "CompressBlocksBC7",
            "CompressBlocksETC1", "CompressBlocksASTC", "GetProfile_ultrafast", "GetProfile_veryfast",
            "GetProfile_fast", "GetProfile_basic", "GetProfile_slow", "GetProfile_alpha_ultrafast",
            "GetProfile_alpha_veryfast", "GetProfile_alpha_fast", "GetProfile_alpha_basic", "GetProfile_alpha_slow",
            "GetProfile_bc6h_veryfast", "GetProfile_bc6h_fast", "GetProfile_bc6h_basic", "GetProfile_bc6h_slow",
            "GetProfile_bc6h_veryslow", "GetProfile_etc_slow", "GetProfile_astc_fast", "GetProfile_astc_alpha_fast",
            "GetProfile_astc_alpha_slow", "ReplicateBorders"
        };
        for (const char* name : exports)
        {
            Require(GetProcAddress(library, name) != nullptr, name);
        }
        static_assert(sizeof(rgba_surface) == 24);
        static_assert(offsetof(rgba_surface, stride) == 16);
    }

    using Color = std::array<int, 4>;
    constexpr Color First{32, 96, 192, 37};
    constexpr Color Second{208, 48, 16, 221};

    Color Color565(std::uint16_t value)
    {
        return {static_cast<int>((value >> 11) & 31) * 255 / 31,
            static_cast<int>((value >> 5) & 63) * 255 / 63, static_cast<int>(value & 31) * 255 / 31, 255};
    }

    // Test-only BC1/BC3 block oracle, not a product decompressor.
    void CheckBCColor(const std::uint8_t* block, const Color& expected, bool alpha)
    {
        const auto* color = block + (alpha ? 8 : 0);
        const std::uint16_t e0 = static_cast<std::uint16_t>(color[0] | (color[1] << 8));
        const std::uint16_t e1 = static_cast<std::uint16_t>(color[2] | (color[3] << 8));
        std::array<Color, 4> palette{Color565(e0), Color565(e1), Color{}, Color{}};
        for (std::size_t channel = 0; channel < 3; ++channel)
        {
            palette[2][channel] = (2 * palette[0][channel] + palette[1][channel]) / 3;
            palette[3][channel] = (palette[0][channel] + 2 * palette[1][channel]) / 3;
            if (!alpha && e0 <= e1)
            {
                palette[2][channel] = (palette[0][channel] + palette[1][channel]) / 2;
                palette[3][channel] = 0;
            }
        }
        std::uint32_t selectors = 0;
        std::uint64_t alphaSelectors = 0;
        std::array<int, 8> alphaPalette{};
        for (unsigned index = 0; index < 4; ++index)
        {
            selectors |= static_cast<std::uint32_t>(color[index + 4]) << (8 * index);
        }
        if (alpha)
        {
            alphaPalette[0] = block[0];
            alphaPalette[1] = block[1];
            for (unsigned index = 0; index < 6; ++index)
            {
                alphaSelectors |= static_cast<std::uint64_t>(block[index + 2]) << (8 * index);
            }
            for (int index = 2; index < 8; ++index)
            {
                alphaPalette[static_cast<std::size_t>(index)] = block[0] > block[1]
                    ? ((8 - index) * block[0] + (index - 1) * block[1]) / 7
                    : index < 6 ? ((6 - index) * block[0] + (index - 1) * block[1]) / 5 : index == 6 ? 0 : 255;
            }
        }
        for (unsigned pixel = 0; pixel < 16; ++pixel)
        {
            const auto& decoded = palette[(selectors >> (2 * pixel)) & 3];
            for (std::size_t channel = 0; channel < 3; ++channel)
            {
                Require(std::abs(decoded[channel] - expected[channel]) <= 8, "BC color constant fixture was not encoded");
            }
            if (alpha)
            {
                Require(std::abs(alphaPalette[(alphaSelectors >> (3 * pixel)) & 7] - expected[3]) <= 1,
                    "BC3 alpha constant fixture was not encoded");
            }
        }
    }

    template<class Compress, class CheckBlock>
    void Blocks(int blockWidth, int blockHeight, int blockBytes, bool hdr, Compress compress, CheckBlock checkBlock)
    {
        const int width = 2 * blockWidth;
        const int height = 2 * blockHeight;
        const int bytesPerPixel = hdr ? 8 : 4;
        const int compactStride = width * bytesPerPixel;
        const int paddedStride = compactStride + 32;
        std::vector<std::uint8_t> compact(static_cast<std::size_t>(compactStride * height));
        std::vector<std::uint8_t> padded(static_cast<std::size_t>(paddedStride * height), 0xA5);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const bool second = ((x / blockWidth + y / blockHeight) & 1) != 0;
                for (int channel = 0; channel < 4; ++channel)
                {
                    const std::size_t offset = static_cast<std::size_t>(y * compactStride + x * bytesPerPixel);
                    if (hdr)
                    {
                        const std::array<std::uint16_t, 4> firstHalf{0x3400, 0x3C00, 0x4000, 0x3C00};
                        const std::array<std::uint16_t, 4> secondHalf{0x4200, 0x3800, 0x3C00, 0x3C00};
                        const auto value = (second ? secondHalf : firstHalf)[static_cast<std::size_t>(channel)];
                        compact[offset + static_cast<std::size_t>(channel * 2)] = static_cast<std::uint8_t>(value & 255);
                        compact[offset + static_cast<std::size_t>(channel * 2 + 1)] = static_cast<std::uint8_t>(value >> 8);
                    }
                    else
                    {
                        compact[offset + static_cast<std::size_t>(channel)] =
                            static_cast<std::uint8_t>((second ? Second : First)[static_cast<std::size_t>(channel)]);
                    }
                }
            }
            std::copy_n(compact.data() + y * compactStride, compactStride, padded.data() + y * paddedStride);
        }
        const auto inputBefore = compact;
        const auto paddedBefore = padded;
        const std::size_t outputBytes = static_cast<std::size_t>(4 * blockBytes);
        std::vector<std::uint8_t> result(outputBytes + 128, 0xCD);
        std::vector<std::uint8_t> repeated(outputBytes + 128, 0xCD);
        std::vector<std::uint8_t> strided(outputBytes + 128, 0xCD);
        rgba_surface input{compact.data(), width, height, compactStride};
        rgba_surface inputPadded{padded.data(), width, height, paddedStride};
        compress(&input, result.data() + 64);
        compress(&input, repeated.data() + 64);
        compress(&inputPadded, strided.data() + 64);
        Require(result == repeated, "Repeated native dispatch changed compressed blocks");
        Require(result == strided, "Input stride changed compressed blocks");
        Require(compact == inputBefore && padded == paddedBefore, "Compression mutated caller-owned input");
        Require(std::all_of(result.begin(), result.begin() + 64, [](auto b) { return b == 0xCD; }) &&
            std::all_of(result.end() - 64, result.end(), [](auto b) { return b == 0xCD; }), "Compressed output bounds violated");
        Require(!std::equal(result.begin() + 64, result.begin() + 64 + blockBytes, result.begin() + 64 + blockBytes),
            "Distinct color/HDR blocks produced identical output");
        Require(std::equal(result.begin() + 64, result.begin() + 64 + blockBytes, result.begin() + 64 + 3 * blockBytes),
            "Raster block order lost repeated diagonal fixture");
        for (int index = 0; index < 4; ++index)
        {
            checkBlock(result.data() + 64 + index * blockBytes, index == 1 || index == 2 ? Second : First);
        }
    }

    void Codec(const std::string& name)
    {
        const auto noDecode = [](const std::uint8_t*, const Color&) {};
        if (name == "bc1" || name == "bc3")
        {
            const bool alpha = name == "bc3";
            Blocks(4, 4, alpha ? 16 : 8, false, alpha ? CompressBlocksBC3 : CompressBlocksBC1,
                [alpha](const auto* b, const Color& color) { CheckBCColor(b, color, alpha); });
        }
        else if (name == "bc6h")
        {
            const std::array<void (*)(bc6h_enc_settings*), 5> profiles{
                GetProfile_bc6h_veryfast, GetProfile_bc6h_fast, GetProfile_bc6h_basic, GetProfile_bc6h_slow, GetProfile_bc6h_veryslow};
            for (const auto profile : profiles)
            {
                bc6h_enc_settings settings{};
                profile(&settings);
                Blocks(4, 4, 16, true, [&settings](auto* s, auto* d) { CompressBlocksBC6H(s, d, &settings); },
                    [](const auto* b, const Color&)
                    {
                        const int prefix = b[0] & 31;
                        Require((prefix & 3) < 2 || prefix == 2 || prefix == 6 || prefix == 10 || prefix == 14 ||
                            prefix == 18 || prefix == 22 || prefix == 26 || prefix == 30 || prefix == 3 || prefix == 7 ||
                            prefix == 11 || prefix == 15, "Reserved BC6H mode emitted");
                    });
            }
        }
        else if (name == "bc7")
        {
            const std::array<void (*)(bc7_enc_settings*), 10> profiles{
                GetProfile_ultrafast, GetProfile_veryfast, GetProfile_fast, GetProfile_basic, GetProfile_slow,
                GetProfile_alpha_ultrafast, GetProfile_alpha_veryfast, GetProfile_alpha_fast, GetProfile_alpha_basic, GetProfile_alpha_slow};
            for (std::size_t index = 0; index < profiles.size(); ++index)
            {
                bc7_enc_settings settings{};
                profiles[index](&settings);
                Require(settings.channels == (index < 5 ? 3 : 4), "BC7 profile lost RGB/alpha semantics");
                Blocks(4, 4, 16, false, [&settings](auto* s, auto* d) { CompressBlocksBC7(s, d, &settings); },
                    [](const auto* b, const Color&) { Require(b[0] != 0, "Reserved BC7 mode emitted"); });
            }
        }
        else if (name == "etc1")
        {
            etc_enc_settings settings{};
            GetProfile_etc_slow(&settings);
            Blocks(4, 4, 8, false, [&settings](auto* s, auto* d) { CompressBlocksETC1(s, d, &settings); }, noDecode);
        }
        else if (name == "astc")
        {
            const std::array<void (*)(astc_enc_settings*, int, int), 3> profiles{
                GetProfile_astc_fast, GetProfile_astc_alpha_fast, GetProfile_astc_alpha_slow};
            for (int blockSize : {4, 6, 8})
            {
                for (std::size_t index = 0; index < profiles.size(); ++index)
                {
                    astc_enc_settings settings{};
                    profiles[index](&settings, blockSize, blockSize);
                    Require(settings.channels == (index == 0 ? 3 : 4) && settings.block_width == blockSize &&
                        settings.block_height == blockSize, "ASTC profile changed block/alpha contract");
                    Blocks(blockSize, blockSize, 16, false, [&settings](auto* s, auto* d) { CompressBlocksASTC(s, d, &settings); }, noDecode);
                }
            }
        }
        else
        {
            throw std::runtime_error("Unknown codec case");
        }
    }

    void Borders()
    {
        for (int bpp : {32, 64})
        {
            const int pixelBytes = bpp / 8;
            std::vector<std::uint8_t> source(static_cast<std::size_t>(3 * 2 * pixelBytes));
            for (std::size_t index = 0; index < source.size(); ++index)
            {
                source[index] = static_cast<std::uint8_t>(index + 1);
            }
            const auto before = source;
            std::vector<std::uint8_t> result(static_cast<std::size_t>(4 * 4 * pixelBytes + 32), 0xCD);
            rgba_surface input{source.data(), 3, 2, 3 * pixelBytes};
            rgba_surface output{result.data() + 16, 4, 4, 4 * pixelBytes};
            ReplicateBorders(&output, &input, 1, 1, bpp);
            for (int y = 0; y < 4; ++y)
            {
                for (int x = 0; x < 4; ++x)
                {
                    const auto* expected = source.data() + (3 + (std::min)(x + 1, 2)) * pixelBytes;
                    Require(std::equal(expected, expected + pixelBytes, output.ptr + (y * 4 + x) * pixelBytes),
                        "Border replication did not preserve the clipped source pixel");
                }
            }
            Require(before == source && std::all_of(result.begin(), result.begin() + 16, [](auto b) { return b == 0xCD; }) &&
                std::all_of(result.end() - 16, result.end(), [](auto b) { return b == 0xCD; }), "Border replication violated buffer ownership");
        }
    }
}

int main(int argc, char** argv)
{
    try
    {
        JE::ISPCTexCompTests::Require(argc == 2, "Select one native consumer case");
        const std::string name = argv[1];
        JE::ISPCTexCompTests::Exports();
        if (name == "borders")
        {
            JE::ISPCTexCompTests::Borders();
        }
        else if (name != "exports")
        {
            JE::ISPCTexCompTests::Codec(name);
        }
        std::cout << "PASS " << name << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
