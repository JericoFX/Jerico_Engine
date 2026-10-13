// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <OpenImageIO/imageio.h>
#include <OpenColorIO/OpenColorIO.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace OCIO = OCIO_NAMESPACE;
namespace
{
    void Require(bool value, const char* message)
    {
        if (!value) { throw std::runtime_error(message); }
    }

    bool Progress(void* context, float fraction)
    {
        auto& calls = *static_cast<unsigned*>(context);
        Require(std::isfinite(fraction) && fraction >= 0.0f && fraction <= 1.0f, "bounded write progress");
        ++calls;
        return false;
    }

    template<class Pixel>
    void RoundTrip(const std::filesystem::path& root, const char* stem, const char* extension,
        OIIO::TypeDesc type, int channels)
    {
        constexpr int width = 19;
        constexpr int height = 17;
        const auto temporary = root / (std::string(stem) + ".temporary." + extension);
        const auto final = root / (std::string(stem) + "." + extension);
        Require(!std::filesystem::exists(temporary) && !std::filesystem::exists(final), "fresh owned image outputs");
        std::vector<Pixel> pixels(width * height * channels);
        for (std::size_t index = 0; index < pixels.size(); ++index)
        {
            if constexpr (std::is_same_v<Pixel, float>)
            {
                pixels[index] = static_cast<float>(index % 97) / 32.0f - 0.75f;
            }
            else
            {
                pixels[index] = static_cast<Pixel>((index * 37 + 11) % (sizeof(Pixel) == 1 ? 256 : 65536));
            }
        }
        auto output = OIIO::ImageOutput::create(temporary.string());
        Require(output != nullptr, "ImageOutput plugin available");
        OIIO::ImageSpec spec(width, height, channels, type);
        // The RGBA fixture contains straight alpha; avoid lossy premultiplication conversions.
        if (channels == 4) { spec.attribute("oiio:UnassociatedAlpha", 1); }
        Require(output->open(temporary.string(), spec), "ImageOutput open");
        unsigned callbacks = 0;
        Require(output->write_image(type, pixels.data(), OIIO::AutoStride, OIIO::AutoStride, OIIO::AutoStride,
            Progress, &callbacks), "caller write_image with progress");
        Require(output->close(), "ImageOutput close");
        output.reset();
        std::filesystem::rename(temporary, final);
        auto input = OIIO::ImageInput::open(final.string(), channels == 4 ? &spec : nullptr);
        Require(input != nullptr, "published image readable");
        Require(input->spec().width == width && input->spec().height == height && input->spec().nchannels == channels,
            "image dimensions and channels preserved");
        std::vector<Pixel> decoded(pixels.size());
        Require(input->read_image(0, 0, 0, channels, type, decoded.data()), "ImageInput decode");
        Require(input->close(), "ImageInput close");
        Require(decoded == pixels, "lossless native codec pixels match");
        Require(!std::filesystem::exists(temporary), "temporary publication path retired");
        std::printf("CASE %s pixels=%zu progress=%u\n", stem, pixels.size(), callbacks);
    }

    void Jpeg(const std::filesystem::path& root)
    {
        constexpr int width = 32, height = 24, channels = 3;
        std::vector<std::uint8_t> pixels(width * height * channels);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int i = (y * width + x) * channels;
                pixels[i] = static_cast<std::uint8_t>(32 + x * 4);
                pixels[i + 1] = static_cast<std::uint8_t>(32 + y * 5);
                pixels[i + 2] = static_cast<std::uint8_t>(64 + x + y);
            }
        }
        const auto path = root / "smooth.jpg";
        auto output = OIIO::ImageOutput::create(path.string());
        Require(output != nullptr, "JPEG encoder plugin");
        OIIO::ImageSpec spec(width, height, channels, OIIO::TypeDesc::UINT8);
        spec.attribute("CompressionQuality", 100);
        spec.attribute("jpeg:subsampling", "4:4:4");
        Require(output->open(path.string(), spec) && output->write_image(OIIO::TypeDesc::UINT8, pixels.data())
            && output->close(), "JPEG encode/close");
        auto input = OIIO::ImageInput::open(path.string());
        Require(input && input->spec().width == width && input->spec().height == height
            && input->spec().nchannels == channels, "JPEG decoded geometry");
        std::vector<std::uint8_t> decoded(pixels.size());
        Require(input->read_image(0, 0, 0, channels, OIIO::TypeDesc::UINT8, decoded.data()) && input->close(), "JPEG decode/close");
        unsigned maximum = 0;
        double error = 0;
        for (std::size_t i = 0; i < pixels.size(); ++i)
        {
            const unsigned delta = static_cast<unsigned>(std::abs(int(pixels[i]) - int(decoded[i])));
            maximum = std::max(maximum, delta);
            error += delta;
        }
        Require(maximum <= 4 && error / pixels.size() <= 1.0, "JPEG quality100 smooth RGB bounded loss");
        std::printf("JPEG maximum=%u mean=%g\n", maximum, error / pixels.size());
    }

    void VerifyTools(const std::filesystem::path& root)
    {
        std::vector<std::uint8_t> reference;
        for (const char* name : {"checker.tif", "checker.png", "checker.tx"})
        {
            auto input = OIIO::ImageInput::open((root / name).string());
            Require(input && input->spec().width == 32 && input->spec().height == 32
                && input->spec().nchannels == 3, "native tools decoded dimensions/channels");
            std::vector<std::uint8_t> pixels(32 * 32 * 3);
            Require(input->read_image(0, 0, 0, 3, OIIO::TypeDesc::UINT8, pixels.data()), "native tool pixels readable");
            if (reference.empty())
            {
                reference = pixels;
                Require(*std::min_element(pixels.begin(), pixels.end()) == 0
                    && *std::max_element(pixels.begin(), pixels.end()) == 255, "native checker has black and white pixels");
            }
            else { Require(pixels == reference, "iconvert/maketx preserve source base pixels"); }
            if (std::string(name) == "checker.tx")
            {
                OIIO::ImageSpec mip;
                Require(input->seek_subimage(0, 1, mip) && mip.width == 16 && mip.height == 16, "maketx created real mip chain");
            }
            Require(input->close(), "native tool image closed");
        }
    }
    void Color(const std::filesystem::path& root)
    {
        auto config = OCIO::Config::CreateRaw()->createEditableCopy();
        config->setRole(OCIO::ROLE_SCENE_LINEAR, "raw");
        config->validate();
        auto matrix = OCIO::MatrixTransform::Create();
        const double values[16] = {2, 0, 0, 0, 0, 0.5, 0, 0, 0, 0, 1.5, 0, 0, 0, 0, 1};
        const double offset[4] = {0.1, -0.1, 0.25, 0};
        matrix->setMatrix(values);
        matrix->setOffset(offset);
        matrix->validate();
        std::array<float, 4> pixel{0.25f, 0.5f, 0.75f, 0.6f};
        const auto original = pixel;
        config->getProcessor(matrix)->getDefaultCPUProcessor()->applyRGBA(pixel.data());
        Require(std::abs(pixel[0] - 0.6f) < 0.00001f && std::abs(pixel[1] - 0.15f) < 0.00001f
            && std::abs(pixel[2] - 1.375f) < 0.00001f && pixel[3] == original[3], "OCIO matrix and alpha");
        config->getProcessor(matrix, OCIO::TRANSFORM_DIR_INVERSE)->getDefaultCPUProcessor()->applyRGBA(pixel.data());
        for (std::size_t index = 0; index < pixel.size(); ++index)
        {
            Require(std::abs(pixel[index] - original[index]) < 0.00001f, "OCIO inverse roundtrip");
        }
        std::ostringstream serialized;
        config->serialize(serialized);
        std::istringstream stream(serialized.str());
        auto reopened = OCIO::Config::CreateFromStream(stream);
        reopened->validate();
        Require(reopened->getNumColorSpaces() == config->getNumColorSpaces(), "OCIO config serialize/reopen");
        std::ofstream file(root / "raw.ocio", std::ios::binary);
        file << serialized.str();
        Require(file.good(), "owned OCIO CLI config written");
    }

    void Invalid(const std::filesystem::path& root)
    {
        const auto malformed = root / "malformed.png";
        const auto destination = root / "invalid-published.tif";
        std::ofstream file(malformed, std::ios::binary);
        file << "not a PNG";
        file.close();
        OIIO::geterror();
        auto input = OIIO::ImageInput::open(malformed.string());
        Require(input == nullptr && !OIIO::geterror().empty(), "malformed image rejected with diagnostic");
        Require(!std::filesystem::exists(destination), "invalid image never publishes output");
        std::array<float, 4> pixel{0.2f, 0.4f, 0.6f, 0.8f};
        const auto before = pixel;
        bool rejected = false;
        try
        {
            auto config = OCIO::Config::CreateRaw();
            config->getProcessor("JE_missing_color_space", "raw")->getDefaultCPUProcessor()->applyRGBA(pixel.data());
        }
        catch (const OCIO::Exception& error)
        {
            rejected = error.what() != nullptr && error.what()[0] != '\0';
        }
        Require(rejected && pixel == before, "invalid OCIO request rejects without modifying pixels");
    }
}

int main(int argc, char** argv)
{
    try
    {
        Require(argc == 3, "case and owned output directory required");
        Require(OIIO::openimageio_version() == 20317, "linked OIIO exactly 2.3.17");
        Require(std::string(OCIO::GetVersion()) == "2.1.1", "linked OCIO exactly 2.1.1");
        const std::string mode = argv[1];
        const std::filesystem::path root = argv[2];
        if (mode == "tools-verify")
        {
            VerifyTools(root);
            std::printf("CASE tools-verify PASS\n");
            return 0;
        }
        Require(!std::filesystem::exists(root), "consumer output directory must be new");
        std::filesystem::create_directories(root);
        if (mode == "r8") { RoundTrip<std::uint8_t>(root, "r8", "tif", OIIO::TypeDesc::UINT8, 1); }
        else if (mode == "r16") { RoundTrip<std::uint16_t>(root, "r16", "png", OIIO::TypeDesc::UINT16, 1); }
        else if (mode == "r32") { RoundTrip<float>(root, "r32", "exr", OIIO::TypeDesc::FLOAT, 1); }
        else if (mode == "rgba") { RoundTrip<std::uint8_t>(root, "rgba", "png", OIIO::TypeDesc::UINT8, 4); }
        else if (mode == "jpeg") { Jpeg(root); }
        else if (mode == "color") { Color(root); }
        else if (mode == "invalid") { Invalid(root); }
        else { throw std::runtime_error("unknown consumer case"); }
        std::printf("CASE %s PASS OIIO=%d OCIO=%s\n", mode.c_str(), OIIO::openimageio_version(), OCIO::GetVersion());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL %s\n", error.what());
        return 1;
    }
}
