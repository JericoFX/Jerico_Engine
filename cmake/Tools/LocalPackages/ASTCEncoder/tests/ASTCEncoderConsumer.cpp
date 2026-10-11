// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <astcenc.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace JE::AstcTest
{
    void Require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
    void Success(astcenc_error result) { if (result != ASTCENC_SUCCESS) { throw std::runtime_error(astcenc_get_error_string(result)); } }
    using Context = std::unique_ptr<astcenc_context, decltype(&astcenc_context_free)>;
    Context Create(astcenc_profile profile, unsigned x, unsigned y, float quality, unsigned flags, unsigned threads)
    {
        astcenc_config config{};
        Success(astcenc_config_init(profile, x, y, 1, quality, flags, &config));
        astcenc_context* context = nullptr;
        Success(astcenc_context_alloc(&config, threads, &context));
        Require(context != nullptr, "allocated actual ASTC context");
        return Context(context, astcenc_context_free);
    }
    float Value(uint8_t value) { return static_cast<float>(value) / 255.0f; }
    float Value(float value) { return value; }
    float Value(uint16_t value)
    {
        const unsigned exponent = (value >> 10) & 31;
        Require(exponent != 31, "decoded half remains finite");
        const float sign = (value & 0x8000) ? -1.0f : 1.0f;
        return sign * (exponent ? std::ldexp(1.0f + static_cast<float>(value & 1023) / 1024.0f,
            static_cast<int>(exponent) - 15) : std::ldexp(static_cast<float>(value & 1023), -24));
    }
    template<class T>
    void RoundTrip(astcenc_profile profile, astcenc_type type, const std::array<T, 4>& color,
        unsigned blockX, unsigned blockY, unsigned threads = 1, bool discardAlpha = false)
    {
        constexpr unsigned width = 8, height = 8;
        auto encoder = Create(profile, blockX, blockY, ASTCENC_PRE_MEDIUM, 0, threads);
        auto decoder = Create(profile, blockX, blockY, ASTCENC_PRE_MEDIUM, ASTCENC_FLG_DECOMPRESS_ONLY, 1);
        std::vector<T> pixels(width * height * 4);
        for (size_t pixel = 0; pixel < width * height; ++pixel) { std::copy(color.begin(), color.end(), pixels.begin() + pixel * 4); }
        void* plane = pixels.data();
        astcenc_image input{width, height, 1, type, &plane};
        const astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, discardAlpha ? ASTCENC_SWZ_1 : ASTCENC_SWZ_A};
        const astcenc_swizzle rgba{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
        const size_t blockCount = ((width + blockX - 1) / blockX) * ((height + blockY - 1) / blockY);
        std::vector<uint8_t> encoded(blockCount * 16, 0xA5);
        std::vector<astcenc_error> results(threads, ASTCENC_ERR_BAD_PARAM);
        std::vector<std::thread> workers;
        for (unsigned index = 0; index < threads; ++index)
        {
            workers.emplace_back([&, index] { results[index] = astcenc_compress_image(encoder.get(), &input, &swizzle, encoded.data(), encoded.size(), index); });
        }
        for (auto& worker : workers) { worker.join(); }
        for (auto result : results) { Success(result); }
        Require(std::any_of(encoded.begin(), encoded.end(), [](uint8_t value) { return value != 0xA5; }), "real ASTC blocks emitted");
        for (size_t offset = 0; offset < encoded.size(); offset += 16)
        {
            astcenc_block_info info{};
            Success(astcenc_get_block_info(encoder.get(), encoded.data() + offset, &info));
            Require(!info.is_error_block && info.profile == profile && info.block_x == blockX && info.block_y == blockY,
                "encoded blocks have the selected profile and footprint");
        }
        std::vector<T> decoded(pixels.size());
        void* outputPlane = decoded.data();
        astcenc_image output{width, height, 1, type, &outputPlane};
        Success(astcenc_decompress_image(decoder.get(), encoded.data(), encoded.size(), &output, &rgba, 0));
        for (size_t index = 0; index < decoded.size(); ++index)
        {
            const float expected = discardAlpha && index % 4 == 3 ? 1.0f : Value(color[index % 4]);
            const float actual = Value(decoded[index]);
            Require(std::isfinite(actual) && std::fabs(actual - expected) <= 0.008f, "decoded RGBA/HDR values match authored pixels");
        }
        // Atom reuses the context for the next mip; both operations must remain usable after reset.
        Success(astcenc_compress_reset(encoder.get()));
        Success(astcenc_decompress_reset(decoder.get()));
        input.dim_x = input.dim_y = 4;
        output.dim_x = output.dim_y = 4;
        const size_t mipBytes = ((4 + blockX - 1) / blockX) * ((4 + blockY - 1) / blockY) * 16;
        workers.clear();
        for (unsigned index = 0; index < threads; ++index)
        {
            workers.emplace_back([&, index] { results[index] = astcenc_compress_image(encoder.get(), &input, &swizzle, encoded.data(), mipBytes, index); });
        }
        for (auto& worker : workers) { worker.join(); }
        for (auto result : results) { Success(result); }
        Success(astcenc_decompress_image(decoder.get(), encoded.data(), mipBytes, &output, &rgba, 0));
        Require(std::fabs(Value(decoded[0]) - Value(color[0])) <= 0.008f, "second mip roundtrip after reset");
    }
    void Gradient()
    {
        auto encoder = Create(ASTCENC_PRF_LDR, 4, 4, ASTCENC_PRE_MEDIUM, 0, 1);
        auto decoder = Create(ASTCENC_PRF_LDR, 4, 4, ASTCENC_PRE_MEDIUM, ASTCENC_FLG_DECOMPRESS_ONLY, 1);
        std::array<uint8_t, 256> pixels{}, decoded{};
        for (unsigned y = 0; y < 8; ++y)
        {
            for (unsigned x = 0; x < 8; ++x)
            {
                const unsigned offset = (y * 8 + x) * 4;
                pixels[offset] = pixels[offset + 1] = pixels[offset + 2] = static_cast<uint8_t>(x * 32);
                pixels[offset + 3] = 255;
            }
        }
        void* plane = pixels.data(); void* outputPlane = decoded.data();
        astcenc_image input{8, 8, 1, ASTCENC_TYPE_U8, &plane}, output{8, 8, 1, ASTCENC_TYPE_U8, &outputPlane};
        const astcenc_swizzle rgba{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
        std::array<uint8_t, 64> encoded{};
        Success(astcenc_compress_image(encoder.get(), &input, &rgba, encoded.data(), encoded.size(), 0));
        for (size_t offset = 0; offset < encoded.size(); offset += 16)
        {
            astcenc_block_info info{};
            Success(astcenc_get_block_info(encoder.get(), encoded.data() + offset, &info));
            Require(!info.is_error_block && !info.is_constant_block, "spatial gradient exercises actual nonconstant ASTC blocks");
        }
        Success(astcenc_decompress_image(decoder.get(), encoded.data(), encoded.size(), &output, &rgba, 0));
        for (size_t index = 0; index < decoded.size(); ++index)
        {
            Require(std::abs(static_cast<int>(decoded[index]) - pixels[index]) <= 4, "lossy grayscale gradient retains spatial values within four U8 levels");
        }
    }
    void Normal()
    {
        auto encoder = Create(ASTCENC_PRF_LDR, 4, 4, ASTCENC_PRE_THOROUGH, ASTCENC_FLG_MAP_NORMAL, 1);
        auto decoder = Create(ASTCENC_PRF_LDR, 4, 4, ASTCENC_PRE_MEDIUM, ASTCENC_FLG_DECOMPRESS_ONLY, 1);
        std::array<uint8_t, 64> pixels{}, decoded{};
        for (size_t index = 0; index < pixels.size(); index += 4) { pixels[index] = pixels[index + 1] = 128; pixels[index + 2] = pixels[index + 3] = 255; }
        void* plane = pixels.data(); void* outputPlane = decoded.data();
        astcenc_image input{4, 4, 1, ASTCENC_TYPE_U8, &plane}, output{4, 4, 1, ASTCENC_TYPE_U8, &outputPlane};
        const astcenc_swizzle pack{ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_G};
        const astcenc_swizzle unpack{ASTCENC_SWZ_R, ASTCENC_SWZ_A, ASTCENC_SWZ_Z, ASTCENC_SWZ_1};
        std::array<uint8_t, 16> encoded{};
        Success(astcenc_compress_image(encoder.get(), &input, &pack, encoded.data(), encoded.size(), 0));
        Success(astcenc_decompress_image(decoder.get(), encoded.data(), encoded.size(), &output, &unpack, 0));
        for (size_t index = 0; index < decoded.size(); index += 4)
        {
            Require(std::abs(static_cast<int>(decoded[index]) - 128) <= 2 && std::abs(static_cast<int>(decoded[index + 1]) - 128) <= 2
                && decoded[index + 2] >= 253 && decoded[index + 3] == 255, "Atom normal-map RRRG/RAZ1 swizzles preserve XY and reconstruct Z");
        }
    }
    void Invalid()
    {
        auto context = Create(ASTCENC_PRF_LDR, 4, 4, ASTCENC_PRE_FAST, 0, 1);
        std::array<uint8_t, 64> pixels{}; pixels.fill(0x5A);
        void* plane = pixels.data(); astcenc_image image{4, 4, 1, ASTCENC_TYPE_U8, &plane};
        const astcenc_swizzle rgba{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
        std::array<uint8_t, 16> buffer{}; buffer.fill(0xA5); const auto before = buffer;
        Require(astcenc_compress_image(context.get(), &image, &rgba, buffer.data(), 15, 0) == ASTCENC_ERR_OUT_OF_MEM && buffer == before,
            "undersized compressed output rejected without modifying output");
        const auto pixelBefore = pixels;
        Require(astcenc_decompress_image(context.get(), buffer.data(), 15, &image, &rgba, 0) == ASTCENC_ERR_OUT_OF_MEM && pixels == pixelBefore,
            "truncated ASTC data rejected without modifying decoded pixels");
        auto badSwizzle = rgba; badSwizzle.r = static_cast<astcenc_swz>(999);
        Require(astcenc_compress_image(context.get(), &image, &badSwizzle, buffer.data(), buffer.size(), 0) == ASTCENC_ERR_BAD_SWIZZLE && buffer == before,
            "invalid swizzle rejected without modifying output");
        Require(astcenc_compress_image(context.get(), &image, &rgba, buffer.data(), buffer.size(), 1) == ASTCENC_ERR_BAD_PARAM && buffer == before,
            "out-of-range worker rejected without modifying output");
        astcenc_config config{};
        Require(astcenc_config_init(ASTCENC_PRF_LDR, 3, 3, 1, ASTCENC_PRE_FAST, 0, &config) == ASTCENC_ERR_BAD_BLOCK_SIZE,
            "unsupported block footprint rejected");
        Require(std::strlen(astcenc_get_error_string(ASTCENC_ERR_BAD_SWIZZLE)) > 0, "invalid input has an actual diagnostic");
    }
}
int main(int argc, char** argv)
{
    try
    {
        using namespace JE::AstcTest;
        Require(argc == 2, "one named native ASTC case required");
        const std::string mode = argv[1];
        if (mode == "ldr8")
        {
            Gradient();
            constexpr unsigned footprints[][2] = {{4,4},{5,4},{5,5},{6,5},{6,6},{8,5},{8,6},{10,5},{10,6},{8,8},{10,8},{10,10},{12,10},{12,12}};
            for (const auto& footprint : footprints) { RoundTrip<uint8_t>(ASTCENC_PRF_LDR, ASTCENC_TYPE_U8, {64,128,192,255}, footprint[0], footprint[1]); }
            RoundTrip<uint8_t>(ASTCENC_PRF_LDR, ASTCENC_TYPE_U8, {128,64,32,0}, 4, 4, 2, true);
        }
        else if (mode == "ldr16") { RoundTrip<uint16_t>(ASTCENC_PRF_LDR, ASTCENC_TYPE_F16, {0x3400,0x3800,0x3A00,0x3C00}, 4, 4); }
        else if (mode == "ldr32") { RoundTrip<float>(ASTCENC_PRF_LDR, ASTCENC_TYPE_F32, {0.25f,0.5f,0.75f,1.0f}, 4, 4); }
        else if (mode == "srgb") { RoundTrip<uint8_t>(ASTCENC_PRF_LDR_SRGB, ASTCENC_TYPE_U8, {64,128,192,255}, 4, 4); }
        else if (mode == "hdr-rgb") { RoundTrip<uint16_t>(ASTCENC_PRF_HDR_RGB_LDR_A, ASTCENC_TYPE_F16, {0x4400,0x4000,0x3800,0x3C00}, 4, 4); }
        else if (mode == "hdr") { RoundTrip<float>(ASTCENC_PRF_HDR, ASTCENC_TYPE_F32, {4.0f,2.0f,0.5f,2.0f}, 4, 4); }
        else if (mode == "normal") { Normal(); }
        else if (mode == "invalid") { Invalid(); }
        else { throw std::runtime_error("unknown native ASTC case"); }
        std::printf("CASE PASS: real ASTC %s\n", mode.c_str()); return 0;
    }
    catch (const std::exception& error) { std::fprintf(stderr, "CASE FAIL: %s\n", error.what()); return 1; }
}
