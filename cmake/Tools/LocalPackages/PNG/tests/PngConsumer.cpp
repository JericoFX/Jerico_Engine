// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <png.h>
#include <zlib.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

#if !defined(_WIN64) || !defined(_MSC_VER) || !defined(_DLL)
#error This consumer requires the native Windows x64 MSVC DLL CRT.
#endif
#if defined(PNG_USE_DLL) || defined(PNG_BUILD_DLL)
#error The imported PNG package must be static.
#endif
#if !defined(PNG_READ_SUPPORTED) || !defined(PNG_WRITE_SUPPORTED) || !defined(PNG_SETJMP_SUPPORTED) \
    || !defined(PNG_USER_MEM_SUPPORTED) || !defined(PNG_READ_PACK_SUPPORTED) \
    || !defined(PNG_READ_GRAY_TO_RGB_SUPPORTED) || !defined(PNG_READ_STRIP_16_TO_8_SUPPORTED) \
    || !defined(PNG_READ_STRIP_ALPHA_SUPPORTED) || !defined(PNG_WRITE_INTERLACING_SUPPORTED)
#error Required original PNG read/write/configuration capabilities are absent.
#endif

namespace
{
    void Require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    struct Stream
    {
        std::array<png_byte, 65536> bytes{};
        png_size_t size = 0;
        png_size_t position = 0;
        std::size_t allocations = 0;
        std::size_t frees = 0;
        int errors = 0;
        int warnings = 0;
    };
    struct Image
    {
        std::array<png_byte, 96> bytes{};
        png_size_t size = 0;
        png_uint_32 width = 0;
        png_uint_32 height = 0;
        int bits = 0;
        int channels = 0;
    };

    void PNGCBAPI Error(png_structp png, png_const_charp)
    {
        ++static_cast<Stream*>(png_get_error_ptr(png))->errors;
        png_longjmp(png, 1);
    }
    void PNGCBAPI Warning(png_structp png, png_const_charp)
    {
        ++static_cast<Stream*>(png_get_error_ptr(png))->warnings;
    }
    png_voidp PNGCBAPI Allocate(png_structp png, png_alloc_size_t bytes)
    {
        auto* memory = std::malloc(static_cast<std::size_t>(bytes));
        if (memory != nullptr) { ++static_cast<Stream*>(png_get_mem_ptr(png))->allocations; }
        return memory;
    }
    void PNGCBAPI Free(png_structp png, png_voidp memory)
    {
        if (memory != nullptr)
        {
            ++static_cast<Stream*>(png_get_mem_ptr(png))->frees;
            std::free(memory);
        }
    }
    void PNGCBAPI Write(png_structp png, png_bytep bytes, png_size_t size)
    {
        auto& stream = *static_cast<Stream*>(png_get_io_ptr(png));
        if (stream.size > stream.bytes.size() || size > stream.bytes.size() - stream.size)
        {
            png_error(png, "bounded test output exhausted");
            return;
        }
        std::memcpy(stream.bytes.data() + stream.size, bytes, size);
        stream.size += size;
    }
    void PNGCBAPI Flush(png_structp) {}
    void PNGCBAPI Read(png_structp png, png_bytep bytes, png_size_t size)
    {
        auto& stream = *static_cast<Stream*>(png_get_io_ptr(png));
        if (stream.position > stream.size || size > stream.size - stream.position)
        {
            png_error(png, "truncated test input");
            return;
        }
        std::memcpy(bytes, stream.bytes.data() + stream.position, size);
        stream.position += size;
    }

    // Stream/Image storage belongs to callers outside the setjmp frame. These
    // helpers and callbacks contain no destructible C++ objects across longjmp.
#pragma warning(push)
#pragma warning(disable: 4611) // Same required setjmp diagnostic suppression as native PngFile.cpp.
    bool Encode(Stream& stream, png_bytep pixels, png_size_t stride,
        png_uint_32 width, png_uint_32 height, int bits, int color, int interlace, int level)
    {
        if (height > 4) { return false; }
        stream.size = 0;
        auto* png = png_create_write_struct_2(PNG_LIBPNG_VER_STRING, &stream,
            Error, Warning, &stream, Allocate, Free);
        if (png == nullptr) { return false; }
        auto* info = png_create_info_struct(png);
        if (info == nullptr) { png_destroy_write_struct(&png, nullptr); return false; }
        if (setjmp(png_jmpbuf(png)) != 0)
        {
            png_destroy_write_struct(&png, &info);
            return false;
        }
        png_set_write_fn(png, &stream, Write, Flush);
        png_set_IHDR(png, info, width, height, bits, color, interlace,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_set_compression_level(png, level);
        png_bytep rows[4]{};
        for (png_uint_32 row = 0; row < height; ++row) { rows[row] = pixels + row * stride; }
        png_set_rows(png, info, rows);
        png_write_png(png, info, PNG_TRANSFORM_IDENTITY, nullptr);
        png_destroy_write_struct(&png, &info);
        return true;
    }

    bool Decode(Stream& stream, Image& image, int transforms)
    {
        stream.position = 0;
        auto* png = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, &stream,
            Error, Warning, &stream, Allocate, Free);
        if (png == nullptr) { return false; }
        auto* info = png_create_info_struct(png);
        auto* end = png_create_info_struct(png);
        if (info == nullptr || end == nullptr)
        {
            png_destroy_read_struct(&png, &info, &end);
            return false;
        }
        if (setjmp(png_jmpbuf(png)) != 0)
        {
            png_destroy_read_struct(&png, &info, &end);
            return false;
        }
        png_set_read_fn(png, &stream, Read);
        // Match the PngFile sequence when a valid header has been consumed.
        if (stream.size >= 8 && png_sig_cmp(stream.bytes.data(), 0, 8) == 0)
        {
            stream.position = 8;
            png_set_sig_bytes(png, 8);
        }
        png_set_keep_unknown_chunks(png, PNG_HANDLE_CHUNK_NEVER, nullptr, 0);
        png_read_png(png, info, transforms, nullptr);
        const auto width = png_get_image_width(png, info);
        const auto height = png_get_image_height(png, info);
        const auto rowSize = png_get_rowbytes(png, info);
        if (height == 0 || rowSize > image.bytes.size() / height)
        {
            png_error(png, "decoded test dimensions exceed fixed output");
        }
        auto** rows = png_get_rows(png, info);
        for (png_uint_32 row = 0; row < height; ++row)
        {
            std::memcpy(image.bytes.data() + row * rowSize, rows[row], rowSize);
        }
        image.size = rowSize * height;
        image.width = width;
        image.height = height;
        image.bits = png_get_bit_depth(png, info);
        image.channels = png_get_channels(png, info);
        png_destroy_read_struct(&png, &info, &end);
        return true;
    }
#pragma warning(pop)

    void Clean(const Stream& stream)
    {
        Require(stream.allocations != 0 && stream.allocations == stream.frees,
            "PNG-owned allocations must all be released on success and failure");
    }
    constexpr int CallerTransforms = PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_GRAY_TO_RGB;

    void Identity()
    {
        Require(PNG_LIBPNG_VER == 10637 && std::strcmp(PNG_LIBPNG_VER_STRING, "1.6.37") == 0, "PNG header version");
        Require(png_access_version_number() == 10637 && std::strcmp(png_get_libpng_ver(nullptr), "1.6.37") == 0,
            "linked PNG exact version");
        Require(std::strcmp(zlibVersion(), "1.2.11") == 0 && std::strcmp(ZLIB_VERSION, "1.2.11") == 0,
            "linked/header zlib exact version");
        const std::array<png_byte, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
        Require(png_sig_cmp(signature.data(), 0, signature.size()) == 0, "PNG signature API");
    }

    void Roundtrip()
    {
        for (const int channels : {3, 4})
        {
            for (const int level : {0, 9})
            {
                auto stream = std::make_unique<Stream>();
                std::array<png_byte, 60> pixels{};
                const auto size = static_cast<std::size_t>(5 * 3 * channels);
                for (std::size_t i = 0; i < size; ++i) { pixels[i] = static_cast<png_byte>((i * 17) % 256); }
                Require(Encode(*stream, pixels.data(), static_cast<png_size_t>(5 * channels), 5, 3, 8,
                    channels == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, level), "encode RGB/RGBA");
                Image actual;
                Require(Decode(*stream, actual, CallerTransforms), "decode RGB/RGBA");
                Require(actual.width == 5 && actual.height == 3 && actual.bits == 8 && actual.channels == channels,
                    "RGB/RGBA metadata");
                Require(actual.size == size && std::memcmp(actual.bytes.data(), pixels.data(), size) == 0,
                    "RGB/RGBA pixels exact");
                Clean(*stream);
            }
        }
    }

    void Transforms()
    {
        auto stream = std::make_unique<Stream>();
        std::array<png_byte, 12> gray16{0, 0, 0x7f, 0x55, 0xff, 0xff, 0x42, 0x01, 1, 0x23, 0xc0, 0xff};
        Require(Encode(*stream, gray16.data(), 6, 3, 2, 16, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, 6), "encode gray16");
        Image actual;
        Require(Decode(*stream, actual, CallerTransforms) && actual.channels == 3 && actual.bits == 8 && actual.size == 18,
            "caller gray-to-RGB and strip16 metadata");
        for (std::size_t i = 0; i < 6; ++i)
        {
            for (std::size_t c = 0; c < 3; ++c) { Require(actual.bytes[i * 3 + c] == gray16[i * 2], "gray16 transform pixels"); }
        }
        Clean(*stream);
        stream = std::make_unique<Stream>();
        std::array<png_byte, 8> rgba{1, 2, 3, 0, 55, 66, 77, 255};
        Require(Encode(*stream, rgba.data(), 8, 2, 1, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, 6), "encode strip-alpha source");
        Require(Decode(*stream, actual, CallerTransforms | PNG_TRANSFORM_STRIP_ALPHA)
            && actual.channels == 3 && actual.size == 6, "caller strip-alpha metadata");
        const std::array<png_byte, 6> rgb{1, 2, 3, 55, 66, 77};
        Require(std::memcmp(actual.bytes.data(), rgb.data(), rgb.size()) == 0, "strip-alpha pixels");
        Clean(*stream);
        stream = std::make_unique<Stream>();
        std::array<png_byte, 1> packed{0x1b}; // Gray2 samples 0,1,2,3, stored most significant first.
        Require(Encode(*stream, packed.data(), 1, 4, 1, 2, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, 6), "encode packed gray2");
        Require(Decode(*stream, actual, CallerTransforms) && actual.size == 12 && actual.bits == 8 && actual.channels == 3,
            "caller packing and gray-to-RGB metadata");
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t c = 0; c < 3; ++c) { Require(actual.bytes[i * 3 + c] == static_cast<png_byte>(i * 85), "packed gray2 sample values"); }
        }
        Clean(*stream);
    }

    void Interlace()
    {
        auto stream = std::make_unique<Stream>();
        std::array<png_byte, 60> pixels{};
        for (std::size_t i = 0; i < pixels.size(); ++i) { pixels[i] = static_cast<png_byte>((i * 29) % 251); }
        Require(Encode(*stream, pixels.data(), 20, 5, 3, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_ADAM7, 6), "encode Adam7");
        Image actual;
        Require(Decode(*stream, actual, CallerTransforms) && actual.size == pixels.size(), "decode Adam7");
        Require(std::memcmp(actual.bytes.data(), pixels.data(), pixels.size()) == 0, "Adam7 pixels exact");
        Clean(*stream);
    }

    void Errors()
    {
        auto source = std::make_unique<Stream>();
        std::array<png_byte, 6> pixels{1, 2, 3, 4, 5, 6};
        Require(Encode(*source, pixels.data(), 6, 2, 1, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, 6), "encode error source");
        Clean(*source);
        for (const auto size : {png_size_t{0}, png_size_t{3}, png_size_t{8}, png_size_t{32}})
        {
            auto truncated = std::make_unique<Stream>();
            Require(size < source->size, "truncation fixture must exclude complete PNG");
            std::memcpy(truncated->bytes.data(), source->bytes.data(), size);
            truncated->size = size;
            Image output;
            output.bytes.fill(0xa5);
            const auto before = output.bytes;
            Require(!Decode(*truncated, output, CallerTransforms) && truncated->errors > 0,
                "truncated input must invoke native PNG error callback and reject");
            Require(output.bytes == before && output.size == 0, "failed decode must not publish caller pixels");
            Clean(*truncated);
        }
        auto corrupt = std::make_unique<Stream>();
        corrupt->bytes = source->bytes;
        corrupt->size = source->size;
        corrupt->bytes[29] ^= 1; // Corrupt IHDR CRC, leaving dimensions and compressed pixels intact.
        Image output;
        Require(!Decode(*corrupt, output, CallerTransforms) && corrupt->errors > 0, "IHDR CRC error must reject");
        Clean(*corrupt);
        Require(Decode(*source, output, CallerTransforms) && output.size == pixels.size(), "valid PNG remains readable after failures");
        Clean(*source);
    }
}

int main(int argc, char** argv)
{
    try
    {
        Require(argc == 2, "one test mode is required");
        const std::string mode = argv[1];
        if (mode == "Identity") { Identity(); }
        else if (mode == "Roundtrip") { Roundtrip(); }
        else if (mode == "Transforms") { Transforms(); }
        else if (mode == "Interlace") { Interlace(); }
        else if (mode == "Errors") { Errors(); }
        else { throw std::runtime_error("unknown mode"); }
        std::printf("PNG %s PASS\n", mode.c_str());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "PNG consumer failure: %s\n", error.what());
        return 1;
    }
}