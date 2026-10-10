/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <ft2build.h>
#include FT_FREETYPE_H
#include <zlib.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{
    struct FreeTypeState
    {
        FT_Library m_library = nullptr;
        FT_Face m_face = nullptr;
        ~FreeTypeState()
        {
            if (m_face)
            {
                FT_Done_Face(m_face);
            }
            if (m_library)
            {
                FT_Done_FreeType(m_library);
            }
        }
    };
}

int main(const int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Supply the explicit local font fixture path\n";
        return 1;
    }
    FreeTypeState state;
    if (FT_Init_FreeType(&state.m_library))
    {
        std::cerr << "FT_Init_FreeType failed\n";
        return 1;
    }
    FT_Int major = 0;
    FT_Int minor = 0;
    FT_Int patch = 0;
    FT_Library_Version(state.m_library, &major, &minor, &patch);
    if (major != 2 || minor != 11 || patch != 1 || std::strcmp(zlibVersion(), "1.2.11") != 0)
    {
        std::cerr << "The linked Freetype/zlib versions differ from the audited pins\n";
        return 1;
    }
    if (FT_New_Face(state.m_library, argv[1], 0, &state.m_face) ||
        FT_Select_Charmap(state.m_face, FT_ENCODING_UNICODE) ||
        FT_Get_Char_Index(state.m_face, 'A') == 0 ||
        FT_Set_Pixel_Sizes(state.m_face, 0, 48) ||
        FT_Load_Char(state.m_face, 'A', FT_LOAD_DEFAULT) ||
        FT_Render_Glyph(state.m_face->glyph, FT_RENDER_MODE_NORMAL))
    {
        std::cerr << "Loading or rendering the local Unicode A glyph failed\n";
        return 1;
    }
    const FT_Bitmap& bitmap = state.m_face->glyph->bitmap;
    if (!bitmap.buffer || bitmap.width == 0 || bitmap.rows == 0 ||
        bitmap.pixel_mode != FT_PIXEL_MODE_GRAY || bitmap.num_grays != 256 ||
        state.m_face->glyph->advance.x <= 0)
    {
        std::cerr << "The rendered glyph lacks a grayscale bitmap or positive advance\n";
        return 1;
    }
    const auto pitch = static_cast<std::int64_t>(bitmap.pitch);
    const auto stride = static_cast<std::uint64_t>(pitch < 0 ? -pitch : pitch);
    if (stride < bitmap.width)
    {
        std::cerr << "The rendered bitmap stride is smaller than its width\n";
        return 1;
    }
    std::uint64_t coverage = 0;
    std::uint64_t nonzero = 0;
    std::uint64_t pixelHash = 14695981039346656037ULL;
    unsigned char minimum = 255;
    unsigned char maximum = 0;
    for (unsigned int y = 0; y < bitmap.rows; ++y)
    {
        const auto rowIndex = bitmap.pitch >= 0 ? y : bitmap.rows - 1 - y;
        const unsigned char* row = bitmap.buffer + static_cast<std::size_t>(rowIndex * stride);
        for (unsigned int x = 0; x < bitmap.width; ++x)
        {
            const unsigned char pixel = row[x];
            coverage += pixel;
            nonzero += pixel != 0 ? 1 : 0;
            minimum = std::min(minimum, pixel);
            maximum = std::max(maximum, pixel);
            pixelHash = (pixelHash ^ pixel) * 1099511628211ULL;
        }
    }
    if (coverage == 0 || minimum == maximum)
    {
        std::cerr << "The glyph bitmap has no ink or no coverage variation\n";
        return 1;
    }
    std::cout << "PASS Freetype 2.11.1 / zlib 1.2.11; Unicode A at 48px"
              << "; width=" << bitmap.width << "; rows=" << bitmap.rows
              << "; advance26.6=" << state.m_face->glyph->advance.x
              << "; nonzero=" << nonzero << "; coverage=" << coverage
              << "; pixelFNV1a64=" << std::hex << pixelHash << '\n';
    return 0;
}
