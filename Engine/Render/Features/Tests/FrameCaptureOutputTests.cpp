/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/Utils/FrameCaptureBus.h>
#include <Atom/Utils/PngFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/Utils.h>

namespace JE::Render::Features
{
    // The existing 8-bit writer remains in the private object library linked by this test target.
    FrameCaptureOutputResult PngFrameCaptureOutput(
        const AZStd::string& outputFilePath, const JE::Render::RPI::AttachmentReadback::ReadbackResult& readbackResult);
}

namespace UnitTest
{
    class FrameCaptureOutputTests : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_previousFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            m_temp = AZStd::make_unique<AZ::Test::ScopedAutoTempDirectory>();
        }

        void TearDown() override
        {
            m_temp.reset();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            m_fileIO.reset();
            AZ::IO::FileIOBase::SetInstance(m_previousFileIO);
            LeakDetectionFixture::TearDown();
        }

        AZStd::string Path(const char* name) const
        {
            return m_temp->Resolve(name).Native();
        }

        JE::Render::RPI::AttachmentReadback::ReadbackResult Readback(
            const AZStd::vector<uint8_t>& bytes, AZ::RHI::Size size = { 1, 1, 1 }) const
        {
            JE::Render::RPI::AttachmentReadback::ReadbackResult result;
            result.m_imageDescriptor.m_size = size;
            result.m_imageDescriptor.m_format = AZ::RHI::Format::R10G10B10A2_UNORM;
            result.m_dataBuffer = AZStd::make_shared<AZStd::vector<uint8_t>>(bytes);
            return result;
        }

        AZStd::vector<uint8_t> ReadFile(const AZStd::string& path) const
        {
            AZ::IO::SystemFile file;
            EXPECT_TRUE(file.Open(path.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY));
            if (!file.IsOpen())
            {
                return {};
            }
            AZStd::vector<uint8_t> bytes(static_cast<size_t>(file.Length()));
            EXPECT_EQ(file.Read(bytes.size(), bytes.data()), bytes.size());
            return bytes;
        }

        // Read the DDS little-endian fields without relying on host alignment or private writer structs.
        static uint32_t Word(const AZStd::vector<uint8_t>& bytes, size_t offset)
        {
            return uint32_t{ bytes[offset] } | (uint32_t{ bytes[offset + 1] } << 8) |
                (uint32_t{ bytes[offset + 2] } << 16) | (uint32_t{ bytes[offset + 3] } << 24);
        }

        AZ::IO::FileIOBase* m_previousFileIO = nullptr;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
        AZStd::unique_ptr<AZ::Test::ScopedAutoTempDirectory> m_temp;
    };

    TEST_F(FrameCaptureOutputTests, TenBitPngQuantizesChannelsAndRetainsExactSameReadbackInDds)
    {
        // Literal packed RGB10/A2 pixels: black, white, red, green, blue, and asymmetric midpoints.
        // The final pixel has RGB=(1, 511, 512), A=2; rounded normalized RGBA8 is (0, 127, 128, 170).
        const AZStd::vector<uint8_t> packed = {
            0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
            0xff, 0x03, 0x00, 0x40, 0x00, 0xfc, 0x0f, 0x80,
            0x00, 0x00, 0xf0, 0xff, 0x01, 0xfc, 0x07, 0xa0
        };
        const AZStd::vector<uint8_t> expected = {
            0, 0, 0, 0, 255, 255, 255, 255,
            255, 0, 0, 85, 0, 255, 0, 170,
            0, 0, 255, 255, 0, 127, 128, 170
        };
        const auto readback = Readback(packed, { 3, 2, 1 });
        const auto path = Path("capture.png");
        const auto output = JE::Render::Features::R10G10B10A2PngFrameCaptureOutput(path, readback);
        ASSERT_EQ(output.m_result, JE::Render::Features::FrameCaptureResult::Success);
        EXPECT_FALSE(output.m_errorMessage.has_value());
        EXPECT_EQ(*readback.m_dataBuffer, packed);

        const auto image = AZ::Utils::PngFile::Load(path.c_str());
        ASSERT_TRUE(image);
        EXPECT_EQ(image.GetWidth(), 3);
        EXPECT_EQ(image.GetHeight(), 2);
        EXPECT_EQ(image.GetBufferFormat(), AZ::Utils::PngFile::Format::RGBA);
        EXPECT_EQ(image.GetBuffer(), expected);

        const auto dds = ReadFile(path + ".dds");
        ASSERT_EQ(dds.size(), 148 + packed.size());
        EXPECT_EQ(Word(dds, 0), 0x20534444u); // DDS magic
        EXPECT_EQ(Word(dds, 4), 124u);
        EXPECT_EQ(Word(dds, 12), 2u);
        EXPECT_EQ(Word(dds, 16), 3u);
        EXPECT_EQ(Word(dds, 84), 0x30315844u); // DX10 extension
        EXPECT_EQ(Word(dds, 128), 24u); // DXGI_FORMAT_R10G10B10A2_UNORM
        EXPECT_EQ(AZStd::vector<uint8_t>(dds.begin() + 148, dds.end()), packed);
    }

    TEST_F(FrameCaptureOutputTests, TenBitInvalidBuffersDoNotWriteEitherOutput)
    {
        auto check = [this](const char* name, const JE::Render::RPI::AttachmentReadback::ReadbackResult& readback)
        {
            const auto path = Path(name);
            const auto output = JE::Render::Features::R10G10B10A2PngFrameCaptureOutput(path, readback);
            EXPECT_EQ(output.m_result, JE::Render::Features::FrameCaptureResult::InvalidArgument);
            EXPECT_TRUE(output.m_errorMessage.has_value());
            EXPECT_FALSE(AZ::IO::SystemFile::Exists(path.c_str()));
            EXPECT_FALSE(AZ::IO::SystemFile::Exists((path + ".dds").c_str()));
        };
        auto nullBuffer = Readback({});
        nullBuffer.m_dataBuffer.reset();
        check("null.png", nullBuffer);
        check("empty.png", Readback({}));
        check("truncated.png", Readback({ 0, 0, 0 }));
        check("oversized.png", Readback({ 0, 0, 0, 0, 0 }));
        check("zero.png", Readback({ 0, 0, 0, 0 }, { 0, 1, 1 }));
        check("volume.png", Readback({ 0, 0, 0, 0 }, { 1, 1, 2 }));
        check("overflow.png", Readback({ 0, 0, 0, 0 }, { 0xffffffffu, 0xffffffffu, 1 }));
    }

    TEST_F(FrameCaptureOutputTests, TenBitDoesNotOverwriteAnExistingOriginalDds)
    {
        const auto path = Path("capture.png");
        const auto original = AZ::Test::CreateTestFile(*m_temp, "capture.png.dds", "preserved original");
        ASSERT_TRUE(original.has_value());
        const auto before = ReadFile(path + ".dds");
        const auto output = JE::Render::Features::R10G10B10A2PngFrameCaptureOutput(path, Readback({ 0, 0, 0, 0 }));
        EXPECT_EQ(output.m_result, JE::Render::Features::FrameCaptureResult::FileWriteError);
        EXPECT_EQ(ReadFile(path + ".dds"), before);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(path.c_str()));
    }

    TEST_F(FrameCaptureOutputTests, TenBitDoesNotOverwriteAnExistingPng)
    {
        const auto path = Path("capture.png");
        ASSERT_TRUE(AZ::Test::CreateTestFile(*m_temp, "capture.png", "preserved preview").has_value());
        const auto before = ReadFile(path);
        const auto output = JE::Render::Features::R10G10B10A2PngFrameCaptureOutput(path, Readback({ 0, 0, 0, 0 }));
        EXPECT_EQ(output.m_result, JE::Render::Features::FrameCaptureResult::FileWriteError);
        EXPECT_EQ(ReadFile(path), before);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists((path + ".dds").c_str()));
    }

    TEST_F(FrameCaptureOutputTests, TenBitDdsWriteFailureDoesNotPublishPngSuccess)
    {
        ASSERT_TRUE(AZ::Test::CreateTestFile(*m_temp, "blocked", "not a folder").has_value());
        const auto path = Path("blocked/capture.png");
        const auto output = JE::Render::Features::R10G10B10A2PngFrameCaptureOutput(path, Readback({ 0, 0, 0, 0 }));
        EXPECT_NE(output.m_result, JE::Render::Features::FrameCaptureResult::Success);
        EXPECT_TRUE(output.m_errorMessage.has_value());
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(path.c_str()));
    }

    TEST_F(FrameCaptureOutputTests, TenBitWriterRejectsOtherFormatsWithoutFiles)
    {
        auto readback = Readback({ 0, 0, 0, 0 });
        readback.m_imageDescriptor.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        const auto path = Path("capture.png");
        EXPECT_EQ(JE::Render::Features::R10G10B10A2PngFrameCaptureOutput(path, readback).m_result,
            JE::Render::Features::FrameCaptureResult::UnsupportedFormat);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(path.c_str()));
        EXPECT_FALSE(AZ::IO::SystemFile::Exists((path + ".dds").c_str()));
    }

    TEST_F(FrameCaptureOutputTests, ExistingRgba8PngRetainsPixelsWithoutDdsSidecar)
    {
        const AZStd::vector<uint8_t> pixels = { 1, 2, 3, 4, 255, 127, 0, 85 };
        auto readback = Readback(pixels, { 2, 1, 1 });
        readback.m_imageDescriptor.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        const auto path = Path("capture.png");
        ASSERT_EQ(JE::Render::Features::PngFrameCaptureOutput(path, readback).m_result, JE::Render::Features::FrameCaptureResult::Success);
        const auto image = AZ::Utils::PngFile::Load(path.c_str());
        ASSERT_TRUE(image);
        EXPECT_EQ(image.GetBuffer(), pixels);
        EXPECT_EQ(*readback.m_dataBuffer, pixels);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists((path + ".dds").c_str()));
    }
}
