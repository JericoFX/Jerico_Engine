// Copyright (c) Contributors to the Open 3D Engine Project.
// For complete copyright and license terms please see the LICENSE at the root of this distribution.
// SPDX-License-Identifier: Apache-2.0 OR MIT

#include <zlib.h>

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

int main()
{
    if (std::strcmp(ZLIB_VERSION, "1.2.11") != 0 || std::strcmp(zlibVersion(), ZLIB_VERSION) != 0)
    {
        std::cerr << "JE zlib: header/runtime version mismatch\n";
        return 1;
    }
    std::array<Bytef, 4096> input{};
    for (std::size_t index = 0; index < input.size(); ++index)
    {
        input[index] = static_cast<Bytef>(index % 256);
    }
    const uLong inputSize = static_cast<uLong>(input.size());
    uLongf compressedSize = compressBound(inputSize);
    std::vector<Bytef> compressed(compressedSize);
    if (compress2(compressed.data(), &compressedSize, input.data(), inputSize, Z_BEST_SPEED) != Z_OK
        || compressedSize >= inputSize)
    {
        std::cerr << "JE zlib: compression failed\n";
        return 1;
    }
    std::array<Bytef, 4096> output{};
    uLongf outputSize = static_cast<uLongf>(output.size());
    if (uncompress(output.data(), &outputSize, compressed.data(), compressedSize) != Z_OK
        || outputSize != inputSize || output != input)
    {
        std::cerr << "JE zlib: roundtrip differs\n";
        return 1;
    }
    outputSize = 8;
    if (uncompress(output.data(), &outputSize, compressed.data(), compressedSize) != Z_BUF_ERROR)
    {
        std::cerr << "JE zlib: short buffer was accepted\n";
        return 1;
    }
    compressed[0] = static_cast<Bytef>(compressed[0] ^ 0xffu);
    outputSize = static_cast<uLongf>(output.size());
    if (uncompress(output.data(), &outputSize, compressed.data(), compressedSize) != Z_DATA_ERROR)
    {
        std::cerr << "JE zlib: corrupt stream was accepted\n";
        return 1;
    }
    std::cout << "JE local zlib 1.2.11: compression, roundtrip, bounds and corrupt-stream checks passed\n";
    return 0;
}
