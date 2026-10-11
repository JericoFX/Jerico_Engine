/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <lz4.h>
#include <lz4hc.h>
#include <lz4frame.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

static_assert(LZ4_VERSION_NUMBER == 10904, "The package header must be LZ4 1.9.4");

namespace
{
    bool Check(bool condition, const char* description)
    {
        std::printf("%s: %s\n", condition ? "PASS" : "FAIL", description);
        return condition;
    }

    std::vector<char> Payload()
    {
        std::vector<char> input(262145);
        for (size_t i = 0; i < input.size(); ++i)
        {
            input[i] = static_cast<char>((i * 29 + i / 23) & 255);
        }
        return input;
    }

    bool Blocks()
    {
        const auto input = Payload();
        const int sourceSize = static_cast<int>(input.size());
        const int bound = LZ4_compressBound(sourceSize);
        std::vector<char> compressed(static_cast<size_t>(bound));
        std::vector<char> restored(input.size());
        for (bool highCompression : { false, true })
        {
            const int size = highCompression
                ? LZ4_compress_HC(input.data(), compressed.data(), sourceSize, bound, LZ4HC_CLEVEL_DEFAULT)
                : LZ4_compress_default(input.data(), compressed.data(), sourceSize, bound);
            if (!Check(size > 0 && size <= bound, highCompression ? "HC block compression" : "default block compression")
                || !Check(LZ4_decompress_safe(compressed.data(), restored.data(), size, sourceSize) == sourceSize
                              && restored == input, "safe block decode is byte exact"))
            {
                return false;
            }
        }
        return true;
    }

    std::vector<char> Frame(const std::vector<char>& input)
    {
        // These are the same default frame APIs used by AzFramework ZipDir.
        std::vector<char> frame(LZ4F_compressFrameBound(input.size(), nullptr));
        const size_t size = LZ4F_compressFrame(frame.data(), frame.size(), input.data(), input.size(), nullptr);
        if (LZ4F_isError(size))
        {
            return {};
        }
        frame.resize(size);
        return frame;
    }

    bool Frames(bool fragmented)
    {
        const auto input = Payload();
        const auto frame = Frame(input);
        LZ4F_decompressionContext_t context = nullptr;
        if (!Check(!frame.empty() && !LZ4F_isError(LZ4F_createDecompressionContext(&context, LZ4F_VERSION))
                       && context, "default frame and decompression context"))
        {
            LZ4F_freeDecompressionContext(context);
            return false;
        }
        std::vector<char> decoded(input.size());
        size_t sourceCursor = 0;
        size_t destinationCursor = 0;
        bool complete = false;
        for (size_t attempts = 0; attempts < 200000; ++attempts)
        {
            size_t sourceBytes = fragmented ? std::min(size_t{ 11 }, frame.size() - sourceCursor) : frame.size();
            size_t destinationBytes = fragmented ? std::min(size_t{ 17 }, decoded.size() - destinationCursor) : decoded.size();
            const size_t remaining = LZ4F_decompress(context, decoded.data() + destinationCursor, &destinationBytes,
                                                   frame.data() + sourceCursor, &sourceBytes, nullptr);
            if (LZ4F_isError(remaining))
            {
                break;
            }
            sourceCursor += sourceBytes;
            destinationCursor += destinationBytes;
            if (remaining == 0)
            {
                complete = sourceCursor == frame.size() && destinationCursor == input.size() && decoded == input;
                break;
            }
            if (!fragmented || (sourceBytes == 0 && destinationBytes == 0))
            {
                break;
            }
        }
        const size_t freed = LZ4F_freeDecompressionContext(context);
        return Check(complete, fragmented ? "fragmented frame input/output roundtrip" : "ZipDir single-call frame roundtrip")
            && Check(!LZ4F_isError(freed), "frame context released");
    }

    bool Errors()
    {
        const auto input = Payload();
        const int sourceSize = static_cast<int>(input.size());
        std::array<char, 18> guarded{};
        guarded.fill('Z');
        const int compressedSmall = LZ4_compress_default(input.data(), guarded.data() + 1, sourceSize, 1);
        if (!Check(compressedSmall == 0 && guarded.front() == 'Z'
                       && std::all_of(guarded.begin() + 2, guarded.end(), [](char c) { return c == 'Z'; }),
                   "block compression rejects short destination without overrun"))
        {
            return false;
        }
        std::vector<char> compressed(static_cast<size_t>(LZ4_compressBound(sourceSize)));
        const int size = LZ4_compress_default(input.data(), compressed.data(), sourceSize, static_cast<int>(compressed.size()));
        if (size <= 1)
        {
            return false;
        }
        guarded.fill('Z');
        const int decodeSmall = LZ4_decompress_safe(compressed.data(), guarded.data() + 1, size, 1);
        const size_t frameSmall = LZ4F_compressFrame(guarded.data() + 1, 1, input.data(), input.size(), nullptr);
        if (!Check(decodeSmall < 0 && LZ4F_isError(frameSmall) && LZ4F_getErrorName(frameSmall)[0] != '\0'
                       && guarded.front() == 'Z'
                       && std::all_of(guarded.begin() + 2, guarded.end(), [](char c) { return c == 'Z'; }),
                   "safe block decode and frame compression reject short destination"))
        {
            return false;
        }
        std::vector<char> decoded(input.size());
        auto frame = Frame(input);
        if (frame.size() < 5)
        {
            return false;
        }
        LZ4F_decompressionContext_t context = nullptr;
        if (LZ4F_isError(LZ4F_createDecompressionContext(&context, LZ4F_VERSION)))
        {
            return false;
        }
        size_t sourceBytes = frame.size() - 1;
        size_t destinationBytes = decoded.size();
        const size_t incomplete = LZ4F_decompress(context, decoded.data(), &destinationBytes, frame.data(), &sourceBytes, nullptr);
        LZ4F_freeDecompressionContext(context);
        // Frame decoding is incremental: a truncated frame needs more input, it is not complete.
        bool passed = Check(!LZ4F_isError(incomplete) && incomplete != 0, "truncated frame cannot report complete");
        context = nullptr;
        if (LZ4F_isError(LZ4F_createDecompressionContext(&context, LZ4F_VERSION)))
        {
            return false;
        }
        frame[0] ^= 0x7f;
        sourceBytes = frame.size();
        destinationBytes = decoded.size();
        const size_t invalid = LZ4F_decompress(context, decoded.data(), &destinationBytes, frame.data(), &sourceBytes, nullptr);
        const size_t freed = LZ4F_freeDecompressionContext(context);
        return Check(LZ4F_isError(invalid) && LZ4F_getErrorName(invalid)[0] != '\0', "invalid frame magic reports an error")
            && Check(!LZ4F_isError(freed), "failed frame context released") && passed;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2 || !Check(LZ4_versionNumber() == 10904 && std::strcmp(LZ4_versionString(), "1.9.4") == 0,
                            "linked library version is exactly 1.9.4"))
    {
        return 2;
    }
    bool passed = false;
    if (std::strcmp(argv[1], "blocks") == 0) { passed = Blocks(); }
    else if (std::strcmp(argv[1], "frames") == 0) { passed = Frames(false); }
    else if (std::strcmp(argv[1], "fragmented") == 0) { passed = Frames(true); }
    else if (std::strcmp(argv[1], "errors") == 0) { passed = Errors(); }
    return passed ? 0 : 1;
}
