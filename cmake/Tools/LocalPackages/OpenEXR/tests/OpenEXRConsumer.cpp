/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfIO.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfTestFile.h>
#include <OpenEXR/openexr_base.h>
#include <Imath/ImathConfig.h>
#include <Imath/half.h>
#include <zlib.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(OPENEXR_DLL) || defined(IMATH_DLL) || defined(IEX_DLL) || defined(ILMTHREAD_DLL)
#error The original Windows SDK must be static
#endif
namespace Exr = OPENEXR_IMF_INTERNAL_NAMESPACE;
namespace Math = IMATH_INTERNAL_NAMESPACE;
namespace
{
constexpr int Width = 4;
constexpr int Height = 3;
constexpr size_t Values = static_cast<size_t>(Width * Height * 4);
constexpr size_t MaxStream = 1024 * 1024;
const std::array<const char*, 4> Channels{{"R", "G", "B", "A"}};

void Require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}

class MemoryOutput final : public Exr::OStream
{
public:
    MemoryOutput() : Exr::OStream("caller-memory.exr") {}
    void write(const char data[], int count) override
    {
        if (count < 0 || static_cast<size_t>(count) > MaxStream - m_position)
        { throw std::runtime_error("bounded output exceeded"); }
        const size_t end = m_position + static_cast<size_t>(count);
        if (m_bytes.size() < end) { m_bytes.resize(end); }
        if (count > 0) { std::memcpy(m_bytes.data() + m_position, data, static_cast<size_t>(count)); }
        m_position = end;
    }
    uint64_t tellp() override { return static_cast<uint64_t>(m_position); }
    void seekp(uint64_t position) override
    {
        if (position > MaxStream) { throw std::runtime_error("bounded output seek exceeded"); }
        m_position = static_cast<size_t>(position);
    }
    const std::vector<char>& bytes() const { return m_bytes; }
private:
    std::vector<char> m_bytes;
    size_t m_position = 0;
};

class MemoryInput final : public Exr::IStream
{
public:
    explicit MemoryInput(const std::vector<char>& bytes) : Exr::IStream("caller-memory.exr"), m_bytes(bytes) {}
    bool read(char data[], int count) override
    {
        if (count < 0 || static_cast<size_t>(count) > m_bytes.size() - m_position)
        { throw std::runtime_error("truncated EXR stream"); }
        if (count > 0) { std::memcpy(data, m_bytes.data() + m_position, static_cast<size_t>(count)); }
        m_position += static_cast<size_t>(count);
        return m_position < m_bytes.size();
    }
    uint64_t tellg() override { return static_cast<uint64_t>(m_position); }
    void seekg(uint64_t position) override
    {
        if (position > m_bytes.size()) { throw std::runtime_error("bounded input seek exceeded"); }
        m_position = static_cast<size_t>(position);
    }
private:
    const std::vector<char>& m_bytes;
    size_t m_position = 0;
};

template<class T> using Pixels = std::array<T, Values>;

template<class T> Pixels<T> Fixture()
{
    const std::array<float, 8> hdr{{-16.0f, -1.25f, 0.0f, 0.125f, 1.0f, 32.0f, 2048.0f, 65504.0f}};
    Pixels<T> pixels{};
    for (size_t index = 0; index < pixels.size(); ++index)
    { pixels[index] = T(hdr[index % hdr.size()]); }
    return pixels;
}

Exr::Header Header(Exr::PixelType type, Exr::Compression compression, bool alpha)
{
    Exr::Header header(Width, Height);
    header.compression() = compression;
    for (size_t index = 0; index < (alpha ? 4U : 3U); ++index)
    { header.channels().insert(Channels[index], Exr::Channel(type)); }
    return header;
}

template<class T> Exr::FrameBuffer Frame(Pixels<T>& pixels, Exr::PixelType type)
{
    Exr::FrameBuffer frame;
    char* base = reinterpret_cast<char*>(pixels.data());
    for (size_t index = 0; index < Channels.size(); ++index)
    {
        frame.insert(Channels[index], Exr::Slice(type, base + index * sizeof(T),
            4 * sizeof(T), Width * 4 * sizeof(T), 1, 1, index == 3 ? 1.0 : 0.0));
    }
    return frame;
}

template<class T> std::vector<char> Encode(Pixels<T>& pixels, Exr::PixelType type, Exr::Compression compression, bool alpha = true)
{
    MemoryOutput stream;
    {
        Exr::OutputFile file(stream, Header(type, compression, alpha));
        file.setFrameBuffer(Frame(pixels, type));
        file.writePixels(Height);
    }
    return stream.bytes();
}

template<class T> void Read(Exr::InputFile& file, Pixels<T>& pixels, Exr::PixelType type)
{
    Require(file.isComplete(), "incomplete EXR chunks");
    const auto& window = file.header().dataWindow();
    Require(window.min.x == 0 && window.min.y == 0 && window.max.x == Width - 1 && window.max.y == Height - 1,
        "unexpected data window");
    for (size_t index = 0; index < 3; ++index)
    {
        const auto* channel = file.header().channels().findChannel(Channels[index]);
        Require(channel && channel->type == type, "unexpected HDR channel type");
    }
    file.setFrameBuffer(Frame(pixels, type));
    file.readPixels(0, Height - 1);
}

template<class T> bool Decode(const std::vector<char>& bytes, Pixels<T>& destination, Exr::PixelType type)
{
    try
    {
        MemoryInput stream(bytes);
        Exr::InputFile file(stream);
        Pixels<T> candidate{};
        Read(file, candidate, type);
        destination = candidate;
        return true;
    }
    catch (const std::exception&) { return false; }
}

template<class T> void Equal(const Pixels<T>& expected, const Pixels<T>& actual)
{
    for (size_t index = 0; index < expected.size(); ++index)
    { Require(expected[index] == actual[index], "HDR pixel differs"); }
}

void Identity()
{
    Require(std::strcmp(OPENEXR_VERSION_STRING, "3.1.3") == 0, "OpenEXR header version differs");
    Require(std::strcmp(IMATH_VERSION_STRING, "3.1.3") == 0, "Imath header version differs");
    Require(std::strcmp(zlibVersion(), "1.2.11") == 0, "linked zlib version differs");
    int major = 0, minor = 0, patch = 0;
    const char* extra = nullptr;
    exr_get_library_version(&major, &minor, &patch, &extra);
    Require(major == 3 && minor == 1 && patch == 3, "linked OpenEXRCore version differs");
    Require(extra && *extra == '\0', "unexpected OpenEXR release suffix");
    Math::half half(1.25f);
    Require(half.bits() == 0x3d00 && static_cast<float>(half) == 1.25f, "Imath half encoding differs");
    const Math::half smallest(Math::half::FromBits, 1);
    Require(static_cast<float>(smallest) == 0x1p-24f, "Imath half lookup/denormal differs");
}

template<class T> void Roundtrip(Exr::PixelType type)
{
    auto source = Fixture<T>();
    for (const auto compression : {Exr::NO_COMPRESSION, Exr::RLE_COMPRESSION, Exr::ZIPS_COMPRESSION, Exr::ZIP_COMPRESSION, Exr::PIZ_COMPRESSION})
    {
        const auto encoded = Encode(source, type, compression);
        Pixels<T> decoded{};
        Require(Decode(encoded, decoded, type), "lossless EXR decode failed");
        Equal(source, decoded);
    }
}

void FillAlpha()
{
    auto source = Fixture<Math::half>();
    const auto encoded = Encode(source, Exr::HALF, Exr::ZIP_COMPRESSION, false);
    Pixels<Math::half> decoded{};
    Require(Decode(encoded, decoded, Exr::HALF), "RGB-only EXR failed");
    for (size_t index = 0; index < source.size(); ++index)
    { Require(decoded[index] == (index % 4 == 3 ? Math::half(1.0f) : source[index]), "alpha fill differs from caller contract"); }
}

void Errors()
{
    auto source = Fixture<float>();
    const auto valid = Encode(source, Exr::FLOAT, Exr::ZIP_COMPRESSION);
    Pixels<float> sentinel{};
    sentinel.fill(-9876.5f);
    for (size_t length : {size_t(0), size_t(4), size_t(16), valid.size() / 2, valid.size() - 1})
    {
        const std::vector<char> truncated(valid.begin(), valid.begin() + static_cast<std::ptrdiff_t>(length));
        auto output = sentinel;
        Require(!Decode(truncated, output, Exr::FLOAT), "truncated EXR unexpectedly decoded");
        Equal(sentinel, output);
    }
    auto corrupt = valid;
    corrupt[0] = static_cast<char>(static_cast<unsigned char>(corrupt[0]) ^ 0x7fU);
    auto output = sentinel;
    Require(!Decode(corrupt, output, Exr::FLOAT), "invalid magic unexpectedly decoded");
    Equal(sentinel, output);
    Require(Decode(valid, output, Exr::FLOAT), "valid EXR failed after errors");
    Equal(source, output);
}

void NativeFile(const char* path)
{
    auto source = Fixture<float>();
    source[0] = 1.0e12f;
    source[1] = -1.0e12f;
    {
        Exr::OutputFile file(path, Header(Exr::FLOAT, Exr::ZIP_COMPRESSION, true));
        file.setFrameBuffer(Frame(source, Exr::FLOAT));
        file.writePixels(Height);
    }
    bool tiled = true, deep = true, multipart = true;
    Require(Exr::isOpenExrFile(path, tiled, deep, multipart), "native file signature failed");
    Require(!tiled && !deep && !multipart, "native file is outside selected loader format");
    Pixels<float> decoded{};
    { Exr::InputFile file(path); Read(file, decoded, Exr::FLOAT); }
    Equal(source, decoded);
}
}

int main(int argc, char* argv[])
{
    try
    {
        Require(argc >= 2, "test mode is required");
        const std::string mode = argv[1];
        if (mode == "Identity") { Identity(); }
        else if (mode == "Half") { Roundtrip<Math::half>(Exr::HALF); }
        else if (mode == "Float") { Roundtrip<float>(Exr::FLOAT); }
        else if (mode == "FillAlpha") { FillAlpha(); }
        else if (mode == "Errors") { Errors(); }
        else if (mode == "NativeFile") { Require(argc == 3, "native file output is required"); NativeFile(argv[2]); }
        else { throw std::runtime_error("unknown test mode"); }
        std::cout << "PASS " << mode << '\n';
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
