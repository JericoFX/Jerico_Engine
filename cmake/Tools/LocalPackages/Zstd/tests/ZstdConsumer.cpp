/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include <zstd_errors.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static_assert(ZSTD_VERSION_NUMBER == 10305, "The package header must be zstd 1.3.5");

namespace
{
    bool Check(bool condition, const char* description)
    {
        std::printf("%s: %s\n", condition ? "PASS" : "FAIL", description);
        return condition;
    }

    std::vector<unsigned char> Payload()
    {
        std::vector<unsigned char> bytes(262145);
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            bytes[i] = static_cast<unsigned char>((i * 29 + i / 23) & 255);
        }
        return bytes;
    }

    bool Basic()
    {
        const auto input = Payload();
        std::vector<unsigned char> compressed(ZSTD_compressBound(input.size()));
        const size_t size = ZSTD_compress(compressed.data(), compressed.size(), input.data(), input.size(), 3);
        if (!Check(!ZSTD_isError(size) && size <= compressed.size(), "bounded basic compression"))
        {
            return false;
        }
        compressed.resize(size);
        std::vector<unsigned char> decoded(input.size());
        const size_t restored = ZSTD_decompress(decoded.data(), decoded.size(), compressed.data(), compressed.size());
        return Check(ZSTD_getFrameContentSize(compressed.data(), compressed.size()) == 262145,
                   "frame records the literal fixture length")
            && Check(!ZSTD_isError(restored) && restored == input.size() && decoded == input,
                     "basic byte-exact roundtrip");
    }

    bool Errors()
    {
        const auto input = Payload();
        std::array<unsigned char, 18> guarded{};
        guarded.fill(0xa5);
        const size_t tooSmall = ZSTD_compress(guarded.data() + 1, 1, input.data(), input.size(), 3);
        if (!Check(ZSTD_isError(tooSmall) && ZSTD_getErrorCode(tooSmall) == ZSTD_error_dstSize_tooSmall,
                   "compression rejects a one-byte destination")
            || !Check(guarded.front() == 0xa5
                          && std::all_of(guarded.begin() + 2, guarded.end(), [](unsigned char c) { return c == 0xa5; }),
                      "compression respects destination bounds"))
        {
            return false;
        }
        std::vector<unsigned char> compressed(ZSTD_compressBound(input.size()));
        const size_t size = ZSTD_compress(compressed.data(), compressed.size(), input.data(), input.size(), 3);
        if (ZSTD_isError(size) || size < 5)
        {
            return false;
        }
        compressed.resize(size);
        guarded.fill(0xa5);
        const size_t smallDecode = ZSTD_decompress(guarded.data() + 1, 1, compressed.data(), compressed.size());
        if (!Check(ZSTD_isError(smallDecode) && ZSTD_getErrorCode(smallDecode) == ZSTD_error_dstSize_tooSmall,
                   "decompression rejects a one-byte destination")
            || !Check(guarded.front() == 0xa5
                          && std::all_of(guarded.begin() + 2, guarded.end(), [](unsigned char c) { return c == 0xa5; }),
                      "decompression respects destination bounds"))
        {
            return false;
        }
        std::vector<unsigned char> decoded(input.size());
        const size_t truncated = ZSTD_decompress(decoded.data(), decoded.size(), compressed.data(), compressed.size() - 1);
        compressed[0] ^= 0xff;
        const size_t corrupt = ZSTD_decompress(decoded.data(), decoded.size(), compressed.data(), compressed.size());
        return Check(ZSTD_isError(truncated), "truncated frame is rejected")
            && Check(ZSTD_isError(corrupt) && ZSTD_getErrorCode(corrupt) == ZSTD_error_prefix_unknown,
                     "corrupt frame magic is rejected");
    }

    struct Allocations
    {
        size_t live = 0;
        size_t allocated = 0;
        size_t freed = 0;
        size_t failures = 0;
        bool reject = false;
    };

    void* Allocate(void* opaque, size_t size)
    {
        auto& stats = *static_cast<Allocations*>(opaque);
        if (stats.reject)
        {
            ++stats.failures;
            return nullptr;
        }
        void* memory = std::malloc(size);
        if (memory)
        {
            ++stats.live;
            ++stats.allocated;
        }
        return memory;
    }

    void Free(void* opaque, void* memory)
    {
        if (memory)
        {
            auto& stats = *static_cast<Allocations*>(opaque);
            --stats.live;
            ++stats.freed;
            std::free(memory);
        }
    }

    bool Drain(ZSTD_CStream* stream, std::vector<unsigned char>& compressed, bool finish)
    {
        for (size_t attempts = 0; attempts < 200000; ++attempts)
        {
            std::array<unsigned char, 13> bytes{};
            ZSTD_outBuffer output{ bytes.data(), bytes.size(), 0 };
            const size_t remaining = finish ? ZSTD_endStream(stream, &output) : ZSTD_flushStream(stream, &output);
            if (ZSTD_isError(remaining))
            {
                return false;
            }
            compressed.insert(compressed.end(), bytes.data(), bytes.data() + output.pos);
            if (remaining == 0)
            {
                return true;
            }
            if (output.pos == 0)
            {
                return false;
            }
        }
        return false;
    }

    bool CompressFrame(ZSTD_CStream* stream, const std::vector<unsigned char>& input,
                       std::vector<unsigned char>& compressed)
    {
        for (size_t cursor = 0; cursor < input.size();)
        {
            const size_t chunk = std::min(size_t{ 1021 }, input.size() - cursor);
            ZSTD_inBuffer incoming{ input.data() + cursor, chunk, 0 };
            for (size_t attempts = 0; incoming.pos < incoming.size; ++attempts)
            {
                if (attempts >= 200000)
                {
                    return false;
                }
                const size_t before = incoming.pos;
                std::array<unsigned char, 13> bytes{};
                ZSTD_outBuffer output{ bytes.data(), bytes.size(), 0 };
                const size_t result = ZSTD_compressStream(stream, &output, &incoming);
                if (ZSTD_isError(result) || (before == incoming.pos && output.pos == 0))
                {
                    return false;
                }
                compressed.insert(compressed.end(), bytes.data(), bytes.data() + output.pos);
            }
            cursor += chunk;
            if (cursor == chunk && !Drain(stream, compressed, false))
            {
                return false; // Flush partway through the frame, then continue it.
            }
        }
        return Drain(stream, compressed, true);
    }

    bool DecodeFrame(ZSTD_DStream* stream, const std::vector<unsigned char>& compressed,
                     const std::vector<unsigned char>& expected)
    {
        std::vector<unsigned char> decoded;
        size_t cursor = 0;
        for (size_t attempts = 0; attempts < 200000; ++attempts)
        {
            const size_t chunk = std::min(size_t{ 11 }, compressed.size() - cursor);
            ZSTD_inBuffer incoming{ compressed.data() + cursor, chunk, 0 };
            std::array<unsigned char, 17> bytes{};
            ZSTD_outBuffer output{ bytes.data(), bytes.size(), 0 };
            const size_t remaining = ZSTD_decompressStream(stream, &output, &incoming);
            if (ZSTD_isError(remaining) || decoded.size() + output.pos > expected.size())
            {
                return false;
            }
            cursor += incoming.pos;
            decoded.insert(decoded.end(), bytes.data(), bytes.data() + output.pos);
            if (remaining == 0)
            {
                return cursor == compressed.size() && decoded == expected;
            }
            if (incoming.pos == 0 && output.pos == 0)
            {
                return false;
            }
        }
        return false;
    }

    bool Streams()
    {
        Allocations stats;
        const ZSTD_customMem memory{ Allocate, Free, &stats };
        ZSTD_CStream* compressor = ZSTD_createCStream_advanced(memory);
        ZSTD_DStream* decompressor = ZSTD_createDStream_advanced(memory);
        bool passed = Check(compressor && decompressor, "advanced custom allocator creates both streams");
        if (passed)
        {
            passed = Check(!ZSTD_isError(ZSTD_initCStream(compressor, 3))
                               && !ZSTD_isError(ZSTD_initDStream(decompressor)), "stream initialization");
        }
        if (passed)
        {
            auto input = Payload();
            std::vector<unsigned char> frame;
            passed = Check(CompressFrame(compressor, input, frame), "partial buffers, flush and finish create a frame")
                && Check(DecodeFrame(decompressor, frame, input), "fragmented streaming roundtrip");
            if (passed)
            {
                passed = Check(!ZSTD_isError(ZSTD_resetCStream(compressor, 0))
                                   && !ZSTD_isError(ZSTD_resetDStream(decompressor)), "AzCore 1.3.5 reset APIs");
                std::reverse(input.begin(), input.end());
                frame.clear();
                passed = passed && Check(CompressFrame(compressor, input, frame)
                                            && DecodeFrame(decompressor, frame, input), "same streams roundtrip after reset");
            }
        }
        const size_t freedCompression = ZSTD_freeCStream(compressor);
        const size_t freedDecompression = ZSTD_freeDStream(decompressor);
        return Check(!ZSTD_isError(freedCompression) && !ZSTD_isError(freedDecompression), "stream destruction")
            && Check(stats.allocated > 0 && stats.live == 0 && stats.allocated == stats.freed,
                     "custom allocator owns and releases all stream memory") && passed;
    }

    bool AllocatorFailure()
    {
        Allocations stats;
        stats.reject = true;
        const ZSTD_customMem memory{ Allocate, Free, &stats };
        ZSTD_CStream* compressor = ZSTD_createCStream_advanced(memory);
        ZSTD_DStream* decompressor = ZSTD_createDStream_advanced(memory);
        const bool rejected = !compressor && !decompressor;
        ZSTD_freeCStream(compressor);
        ZSTD_freeDStream(decompressor);
        return Check(rejected && stats.failures == 2 && stats.live == 0,
                     "advanced constructors report real allocator failure");
    }
}

int main(int argc, char** argv)
{
    if (argc != 2 || !Check(ZSTD_versionNumber() == 10305 && std::strcmp(ZSTD_versionString(), "1.3.5") == 0,
                            "linked library version is exactly 1.3.5"))
    {
        return 2;
    }
    bool passed = false;
    if (std::strcmp(argv[1], "basic") == 0)
    {
        passed = Basic();
    }
    else if (std::strcmp(argv[1], "errors") == 0)
    {
        passed = Errors();
    }
    else if (std::strcmp(argv[1], "streams") == 0)
    {
        passed = Streams();
    }
    else if (std::strcmp(argv[1], "allocator") == 0)
    {
        passed = AllocatorFailure();
    }
    return passed ? 0 : 1;
}
