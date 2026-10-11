// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#define TIFF_DISABLE_DEPRECATED
#include <tiffio.h>
#include <zlib.h>
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#if !defined(_WIN64) || !defined(_MSC_VER) || !defined(_DLL)
#error This consumer requires the native Windows x64 MSVC DLL CRT.
#endif
static_assert(sizeof(TIFF_INT64_T) == 8 && std::is_same_v<TIFF_INT64_T, signed long long>);
static_assert(sizeof(TIFF_UINT64_T) == 8 && std::is_same_v<TIFF_UINT64_T, unsigned long long>);
static_assert(sizeof(tmsize_t) == 8 && std::is_signed_v<tmsize_t>);

namespace
{
    void Require(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    struct MemoryFile
    {
        std::vector<std::uint8_t> bytes;
        std::size_t position = 0;
        int closes = 0;
        bool writable = false;
    };
    constexpr std::size_t MaximumFileSize = 1024 * 1024;

    tmsize_t Read(thandle_t handle, void* output, tmsize_t count)
    {
        auto& file = *static_cast<MemoryFile*>(handle);
        if (count < 0 || file.position > file.bytes.size())
        {
            return 0;
        }
        const auto size = (std::min)(static_cast<std::size_t>(count), file.bytes.size() - file.position);
        if (size != 0)
        {
            std::memcpy(output, file.bytes.data() + file.position, size);
        }
        file.position += size;
        return static_cast<tmsize_t>(size);
    }

    tmsize_t Write(thandle_t handle, void* input, tmsize_t count)
    {
        auto& file = *static_cast<MemoryFile*>(handle);
        if (!file.writable || count < 0 || file.position > MaximumFileSize
            || static_cast<std::size_t>(count) > MaximumFileSize - file.position)
        {
            return 0;
        }
        const auto size = static_cast<std::size_t>(count);
        try
        {
            if (file.position + size > file.bytes.size())
            {
                file.bytes.resize(file.position + size);
            }
        }
        catch (...)
        {
            return 0; // No C++ exception may cross a libtiff callback.
        }
        if (size != 0)
        {
            std::memcpy(file.bytes.data() + file.position, input, size);
        }
        file.position += size;
        return count;
    }

    toff_t Seek(thandle_t handle, toff_t offset, int origin)
    {
        auto& file = *static_cast<MemoryFile*>(handle);
        const auto base = origin == SEEK_SET ? 0 : origin == SEEK_CUR ? file.position
            : origin == SEEK_END ? file.bytes.size() : MaximumFileSize + 1;
        if (base > MaximumFileSize || offset > MaximumFileSize - base)
        {
            return static_cast<toff_t>(-1);
        }
        file.position = base + static_cast<std::size_t>(offset);
        return static_cast<toff_t>(file.position);
    }
    int Close(thandle_t handle)
    {
        ++static_cast<MemoryFile*>(handle)->closes;
        return 0; // TIFF closes its handle; the caller still owns the byte storage.
    }
    toff_t Size(thandle_t handle)
    {
        return static_cast<toff_t>(static_cast<MemoryFile*>(handle)->bytes.size());
    }
    int NoMap(thandle_t, void**, toff_t*) { return 0; }
    void NoUnmap(thandle_t, void*, toff_t) {}
    using TiffHandle = std::unique_ptr<TIFF, decltype(&TIFFClose)>;

    TiffHandle Open(MemoryFile& file, const char* mode)
    {
        file.position = 0;
        file.writable = mode[0] == 'w';
        return TiffHandle(TIFFClientOpen("JE local memory TIFF", mode, &file,
            Read, Write, Seek, Close, Size, NoMap, NoUnmap), TIFFClose);
    }

    void Fields(TIFF* tiff, std::uint32_t width, std::uint32_t height,
        std::uint16_t bits, std::uint16_t format, std::uint16_t codec)
    {
        Require(TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width) == 1, "set width");
        Require(TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height) == 1, "set height");
        Require(TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 3) == 1, "set RGB samples");
        Require(TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, bits) == 1, "set bits/sample");
        Require(TIFFSetField(tiff, TIFFTAG_COMPRESSION, codec) == 1, "set codec");
        Require(TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) == 1, "set orientation");
        Require(TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) == 1, "set planar config");
        Require(TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB) == 1, "set photometric");
        Require(TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, format) == 1, "set sample format");
    }

    void Identity()
    {
        Require(TIFFLIB_VERSION == 20201219, "header must identify libtiff 4.2.0");
        Require(std::strcmp(TIFFGetVersion(), TIFFLIB_VERSION_STR) == 0, "linked/header TIFF versions differ");
        Require(std::strcmp(zlibVersion(), "1.2.11") == 0 && std::strcmp(ZLIB_VERSION, "1.2.11") == 0,
            "linked/header zlib must be 1.2.11");
        void* owned = _TIFFmalloc(64);
        Require(owned != nullptr, "TIFF allocation");
        _TIFFmemset(owned, 0x3c, 64);
        Require(static_cast<std::uint8_t*>(owned)[63] == 0x3c, "TIFF allocation contents");
        _TIFFfree(owned);
    }

    void Codecs()
    {
        for (const auto codec : {COMPRESSION_NONE, COMPRESSION_LZW, COMPRESSION_PACKBITS,
            COMPRESSION_THUNDERSCAN, COMPRESSION_NEXT, COMPRESSION_CCITTRLE, COMPRESSION_CCITTRLEW,
            COMPRESSION_CCITTFAX3, COMPRESSION_CCITTFAX4, COMPRESSION_DEFLATE,
            COMPRESSION_ADOBE_DEFLATE, COMPRESSION_SGILOG, COMPRESSION_SGILOG24})
        {
            Require(TIFFIsCODECConfigured(static_cast<std::uint16_t>(codec)) == 1, "original enabled codec missing");
        }
        for (const auto codec : {COMPRESSION_JPEG, COMPRESSION_OJPEG, COMPRESSION_JBIG,
            COMPRESSION_PIXARLOG, COMPRESSION_LZMA, COMPRESSION_ZSTD, COMPRESSION_WEBP})
        {
            Require(TIFFIsCODECConfigured(static_cast<std::uint16_t>(codec)) == 0, "codec disabled by original recipe present");
        }
    }

    MemoryFile ScanlineFile(std::uint16_t codec, bool bigTiff = false)
    {
        MemoryFile file;
        auto tiff = Open(file, bigTiff ? "w8" : "w");
        Require(tiff != nullptr, "open memory writer");
        Fields(tiff.get(), 4, 3, 8, SAMPLEFORMAT_UINT, codec);
        Require(TIFFSetField(tiff.get(), TIFFTAG_ROWSPERSTRIP, 1U) == 1, "set strip rows");
        for (std::uint32_t row = 0; row < 3; ++row)
        {
            std::array<std::uint8_t, 12> pixels{};
            for (std::size_t i = 0; i < pixels.size(); ++i)
            {
                pixels[i] = static_cast<std::uint8_t>(i + row * 37);
            }
            Require(TIFFWriteScanline(tiff.get(), pixels.data(), row, 0) == 1, "write scanline");
        }
        tiff.reset();
        Require(file.closes == 1, "writer closes exactly once");
        return file;
    }

    void Scanlines()
    {
        for (const auto codec : {COMPRESSION_NONE, COMPRESSION_PACKBITS, COMPRESSION_LZW, COMPRESSION_ADOBE_DEFLATE})
        {
            auto file = ScanlineFile(static_cast<std::uint16_t>(codec), codec == COMPRESSION_ADOBE_DEFLATE);
            auto tiff = Open(file, "r");
            Require(tiff != nullptr, "open scanline reader");
            std::uint32_t width = 0, height = 0;
            std::uint16_t channels = 0, bits = 0, format = 0;
            Require(TIFFGetField(tiff.get(), TIFFTAG_IMAGEWIDTH, &width) == 1 && width == 4, "read width");
            Require(TIFFGetField(tiff.get(), TIFFTAG_IMAGELENGTH, &height) == 1 && height == 3, "read height");
            Require(TIFFGetFieldDefaulted(tiff.get(), TIFFTAG_SAMPLESPERPIXEL, &channels) == 1 && channels == 3, "read channels");
            Require(TIFFGetFieldDefaulted(tiff.get(), TIFFTAG_BITSPERSAMPLE, &bits) == 1 && bits == 8, "read bits");
            Require(TIFFGetFieldDefaulted(tiff.get(), TIFFTAG_SAMPLEFORMAT, &format) == 1 && format == SAMPLEFORMAT_UINT, "read format");
            Require(TIFFScanlineSize(tiff.get()) == 12 && TIFFNumberOfStrips(tiff.get()) == 3, "scanline/strip sizing");
            for (std::uint32_t row = 0; row < 3; ++row)
            {
                std::array<std::uint8_t, 12> pixels{};
                Require(TIFFReadScanline(tiff.get(), pixels.data(), row, 0) == 1, "read scanline");
                for (std::size_t i = 0; i < pixels.size(); ++i)
                {
                    Require(pixels[i] == static_cast<std::uint8_t>(i + row * 37), "scanline pixels differ");
                }
            }
            tiff.reset();
            Require(file.closes == 2, "reader closes exactly once");
        }
    }

    void Tiles()
    {
        MemoryFile file;
        std::array<std::uint8_t, 16 * 16 * 3> expected{};
        for (std::size_t i = 0; i < expected.size(); ++i) { expected[i] = static_cast<std::uint8_t>(i % 251); }
        auto tiff = Open(file, "w");
        Require(tiff != nullptr, "open tile writer");
        Fields(tiff.get(), 16, 16, 8, SAMPLEFORMAT_UINT, COMPRESSION_ADOBE_DEFLATE);
        Require(TIFFSetField(tiff.get(), TIFFTAG_TILEWIDTH, 16U) == 1
            && TIFFSetField(tiff.get(), TIFFTAG_TILELENGTH, 16U) == 1, "tile dimensions");
        Require(TIFFWriteTile(tiff.get(), expected.data(), 0, 0, 0, 0) == static_cast<tmsize_t>(expected.size()), "write tile");
        tiff.reset();
        tiff = Open(file, "r");
        Require(tiff != nullptr && TIFFIsTiled(tiff.get()) == 1, "read tile layout");
        Require(TIFFTileSize(tiff.get()) == static_cast<tmsize_t>(expected.size()), "tile size");
        std::array<std::uint8_t, 16 * 16 * 3> actual{};
        Require(TIFFReadTile(tiff.get(), actual.data(), 0, 0, 0, 0) == static_cast<tmsize_t>(actual.size()), "read tile");
        Require(actual == expected, "tile pixels differ");
        tiff.reset();
        Require(file.closes == 2, "tile handles close exactly once");
    }

    void Float()
    {
        MemoryFile file;
        std::array<float, 12> expected{-4.0F, 0.0F, 0.25F, 1.0F, 1.5F, 10.0F, 0.5F, 2.0F, 8.0F, -0.5F, 0.125F, 32.0F};
        auto tiff = Open(file, "w");
        Require(tiff != nullptr, "open float writer");
        Fields(tiff.get(), 2, 2, 32, SAMPLEFORMAT_IEEEFP, COMPRESSION_NONE);
        Require(TIFFSetField(tiff.get(), TIFFTAG_ROWSPERSTRIP, 1U) == 1, "float strip rows");
        for (std::uint32_t row = 0; row < 2; ++row)
        {
            Require(TIFFWriteScanline(tiff.get(), expected.data() + row * 6, row, 0) == 1, "write float scanline");
        }
        tiff.reset();
        tiff = Open(file, "r");
        Require(tiff != nullptr && TIFFScanlineSize(tiff.get()) == 6 * static_cast<tmsize_t>(sizeof(float)), "float scanline size");
        std::uint16_t bits = 0, format = 0;
        Require(TIFFGetFieldDefaulted(tiff.get(), TIFFTAG_BITSPERSAMPLE, &bits) == 1 && bits == 32, "float bit depth");
        Require(TIFFGetFieldDefaulted(tiff.get(), TIFFTAG_SAMPLEFORMAT, &format) == 1 && format == SAMPLEFORMAT_IEEEFP, "float sample format");
        std::array<float, 12> actual{};
        for (std::uint32_t row = 0; row < 2; ++row)
        {
            Require(TIFFReadScanline(tiff.get(), actual.data() + row * 6, row, 0) == 1, "read float scanline");
        }
        Require(std::memcmp(actual.data(), expected.data(), sizeof(expected)) == 0, "float bits differ");
        tiff.reset();
        Require(file.closes == 2, "float handles close exactly once");
    }

    void NativeFile(const char* path)
    {
        // Explicit fresh file under the consumer build directory, exercising tif_win32.c.
        Require(path != nullptr && std::strstr(path, "://") == nullptr
            && std::strncmp(path, "\\\\", 2) != 0 && std::strncmp(path, "//", 2) != 0, "local native file path");
        FILE* existing = nullptr;
        if (fopen_s(&existing, path, "rb") == 0 && existing != nullptr)
        {
            std::fclose(existing);
            throw std::runtime_error("native file output already exists; preserve it");
        }
        std::array<std::uint8_t, 12> expected{0, 1, 2, 33, 44, 55, 66, 77, 88, 254, 253, 252};
        TiffHandle tiff(TIFFOpen(path, "w"), TIFFClose);
        Require(tiff != nullptr, "TIFFOpen native writer");
        Fields(tiff.get(), 4, 1, 8, SAMPLEFORMAT_UINT, COMPRESSION_NONE);
        Require(TIFFWriteScanline(tiff.get(), expected.data(), 0, 0) == 1, "native write scanline");
        tiff.reset();
        tiff.reset(TIFFOpen(path, "r"));
        Require(tiff != nullptr, "TIFFOpen native reader");
        std::array<std::uint8_t, 12> actual{};
        Require(TIFFReadScanline(tiff.get(), actual.data(), 0, 0) == 1 && actual == expected, "native file pixel roundtrip");
        tiff.reset();
        Require(std::remove(path) == 0, "native file handles must be closed before removing own fixture");
    }
    int errorCount = 0;
    void Error(const char*, const char*, va_list) { ++errorCount; }
    void Warning(const char*, const char*, va_list) {}
    struct ErrorHandlers
    {
        TIFFErrorHandler oldError = TIFFSetErrorHandler(Error);
        TIFFErrorHandler oldWarning = TIFFSetWarningHandler(Warning);
        ~ErrorHandlers() { TIFFSetErrorHandler(oldError); TIFFSetWarningHandler(oldWarning); }
    };

    void Errors()
    {
        ErrorHandlers handlers;
        const auto good = ScanlineFile(COMPRESSION_NONE);
        for (const auto size : {std::size_t{0}, std::size_t{3}, std::size_t{8}})
        {
            MemoryFile truncated;
            truncated.bytes.assign(good.bytes.begin(), good.bytes.begin() + size);
            const auto previousErrors = errorCount;
            auto tiff = Open(truncated, "r");
            Require(tiff == nullptr && errorCount > previousErrors, "truncated header/directory must reject and report error");
        }
        MemoryFile corrupt = good;
        corrupt.bytes[0] = 0;
        corrupt.bytes[1] = 0;
        const auto previousErrors = errorCount;
        auto tiff = Open(corrupt, "r");
        Require(tiff == nullptr && errorCount > previousErrors, "bad byte order must reject and report error");
        auto readerFile = good;
        tiff = Open(readerFile, "r");
        Require(tiff != nullptr, "valid reader remains available after errors");
        std::array<std::uint8_t, 12> sentinel{};
        sentinel.fill(0xa5);
        const auto before = sentinel;
        const auto beforeRowErrors = errorCount;
        Require(TIFFReadScanline(tiff.get(), sentinel.data(), 3, 0) == -1, "out-of-range row must fail");
        Require(sentinel == before && errorCount > beforeRowErrors, "invalid row leaves buffer intact and reports error");
    }
}

int main(int argc, char** argv)
{
    try
    {
        Require(argc >= 2, "one test mode is required");
        const std::string mode = argv[1];
        Require(argc == (mode == "NativeFile" ? 3 : 2), "mode argument count");
        if (mode == "Identity") { Identity(); }
        else if (mode == "Codecs") { Codecs(); }
        else if (mode == "Scanlines") { Scanlines(); }
        else if (mode == "Tiles") { Tiles(); }
        else if (mode == "Float") { Float(); }
        else if (mode == "Errors") { Errors(); }
        else if (mode == "NativeFile") { NativeFile(argv[2]); }
        else { throw std::runtime_error("unknown test mode"); }
        std::printf("TIFF %s PASS\n", mode.c_str());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "TIFF consumer failure: %s\n", error.what());
        return 1;
    }
}