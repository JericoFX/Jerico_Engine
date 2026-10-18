/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <PostProcess/ColorGrading/JEEditorLutWriter.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

#include <cstring>
#include <limits>

namespace UnitTest
{
    class JEEditorLutWriterTests : public ::testing::Test
    {
    protected:
        AZStd::string Base(const char* name = "lut") const
        {
            return m_temp.Resolve(name).Native();
        }

        static JE::Render::RPI::AttachmentReadback::ReadbackResult Readback(uint32_t size = 2)
        {
            JE::Render::RPI::AttachmentReadback::ReadbackResult result;
            result.m_state = JE::Render::RPI::AttachmentReadback::ReadbackState::Success;
            result.m_attachmentType = AZ::RHI::AttachmentType::Image;
            result.m_imageDescriptor.m_format = AZ::RHI::Format::R32G32B32A32_FLOAT;
            result.m_imageDescriptor.m_size = { size * size, size, 1 };
            result.m_dataBuffer = AZStd::make_shared<AZStd::vector<uint8_t>>(size * size * size * sizeof(float) * 4);
            for (uint32_t b = 0; b < size; ++b)
            {
                for (uint32_t g = 0; g < size; ++g)
                {
                    for (uint32_t r = 0; r < size; ++r)
                    {
                        const float pixel[4] = {
                            float(r) / float(size - 1), float(g) / float(size - 1), float(b) / float(size - 1), 1.0f
                        };
                        SetPixel(result, (b * size * size) + (g * size) + r, pixel);
                    }
                }
            }
            return result;
        }

        static void SetPixel(JE::Render::RPI::AttachmentReadback::ReadbackResult& result, size_t index, const float (&pixel)[4])
        {
            std::memcpy(result.m_dataBuffer->data() + index * sizeof(pixel), pixel, sizeof(pixel));
        }

        static AZStd::string Read(const AZStd::string& path)
        {
            AZ::IO::SystemFile file;
            EXPECT_TRUE(file.Open(path.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY));
            if (!file.IsOpen())
            {
                return {};
            }
            AZStd::string text(static_cast<size_t>(file.Length()), '\0');
            EXPECT_EQ(file.Read(text.size(), text.data()), text.size());
            return text;
        }

        size_t FileCount() const
        {
            size_t count = 0;
            AZ::IO::SystemFile::FindFiles(m_temp.Resolve("*").c_str(), [&count](const char*, bool isFile)
            {
                count += isFile ? 1 : 0;
                return true;
            });
            return count;
        }

        void ExpectRejected(const JE::Render::RPI::AttachmentReadback::ReadbackResult& result) const
        {
            EXPECT_FALSE(JE::Editor::Render::Scene::WriteLutSource(result, Base()).IsSuccess());
            EXPECT_FALSE(AZ::IO::SystemFile::Exists((Base() + ".3dl").c_str()));
            EXPECT_FALSE(AZ::IO::SystemFile::Exists((Base() + ".azasset").c_str()));
            EXPECT_EQ(FileCount(), 0);
        }

        AZ::Test::ScopedAutoTempDirectory m_temp;
    };

    TEST_F(JEEditorLutWriterTests, WritesScriptSchemaAndBlueFirstColorOrderingToBothFormats)
    {
        const auto result = JE::Editor::Render::Scene::WriteLutSource(Readback(), Base());
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue(), Base() + ".azasset");
        EXPECT_EQ(FileCount(), 2);
        EXPECT_EQ(Read(Base() + ".3dl"),
            "0 1023 \n0 0 0\n0 0 4095\n0 4095 0\n0 4095 4095\n"
            "4095 0 0\n4095 0 4095\n4095 4095 0\n4095 4095 4095\n");

        const auto jsonText = Read(result.GetValue());
        auto json = AZ::JsonSerializationUtils::ReadJsonString(jsonText);
        ASSERT_TRUE(json.IsSuccess());
        const auto& document = json.GetValue();
        ASSERT_TRUE(document.IsObject());
        ASSERT_TRUE(document.HasMember("Type"));
        EXPECT_STREQ(document["Type"].GetString(), "JsonSerialization");
        EXPECT_EQ(document["Version"].GetUint(), 1);
        EXPECT_STREQ(document["ClassName"].GetString(), "LookupTableAsset");
        EXPECT_STREQ(document["ClassData"]["Name"].GetString(), "LookupTable");

        // Load the real reflected type, not a second test-only LUT model.
        AZ::SerializeContext context;
        JE::Render::Features::LookupTableAsset::Reflect(&context);
        AZ::JsonDeserializerSettings settings;
        settings.m_serializeContext = &context;
        JE::Render::Features::LookupTableAsset loaded;
        const auto load = AZ::JsonSerializationUtils::LoadObjectFromString(loaded, jsonText, &settings);
        ASSERT_TRUE(load.IsSuccess()) << load.GetError().c_str();
        EXPECT_EQ(loaded.m_name, "LookupTable");
        EXPECT_EQ(loaded.m_intervals, (AZStd::vector<AZ::u64>{ 0, 1023 }));
        EXPECT_EQ(loaded.m_values, (AZStd::vector<AZ::u64>{
            0, 0, 0, 0, 0, 4095, 0, 4095, 0, 0, 4095, 4095,
            4095, 0, 0, 4095, 0, 4095, 4095, 4095, 0, 4095, 4095, 4095
        }));
        EXPECT_TRUE(JE::Editor::Render::Scene::ValidateLookupTable(loaded).IsSuccess());
    }

    TEST_F(JEEditorLutWriterTests, FloorsIntervalsAndClampedFloatChannelsWithoutRounding)
    {
        auto readback = Readback(3);
        const float pixel[4] = { -0.5f, 0.5f, 1.5f, 0.25f };
        SetPixel(readback, 0, pixel);
        ASSERT_TRUE(JE::Editor::Render::Scene::WriteLutSource(readback, Base()).IsSuccess());
        const auto text = Read(Base() + ".3dl");
        EXPECT_EQ(text.find("0 511 1023 \n0 2047 4095\n"), 0);
    }

    TEST_F(JEEditorLutWriterTests, RejectsFailedReadbackAndWrongAttachmentOrFormat)
    {
        auto readback = Readback();
        readback.m_state = JE::Render::RPI::AttachmentReadback::ReadbackState::Failed;
        ExpectRejected(readback);
        readback.m_state = JE::Render::RPI::AttachmentReadback::ReadbackState::Success;
        readback.m_attachmentType = AZ::RHI::AttachmentType::Buffer;
        ExpectRejected(readback);
        readback.m_attachmentType = AZ::RHI::AttachmentType::Image;
        readback.m_imageDescriptor.m_format = AZ::RHI::Format::R16G16B16A16_FLOAT;
        ExpectRejected(readback);
    }

    TEST_F(JEEditorLutWriterTests, RejectsDimensionsAndOverflowBeforeIndexing)
    {
        auto readback = Readback();
        readback.m_imageDescriptor.m_size = { 1, 1, 1 };
        ExpectRejected(readback);
        readback.m_imageDescriptor.m_size = { 3, 2, 1 };
        ExpectRejected(readback);
        readback.m_imageDescriptor.m_size = { 4, 2, 2 };
        ExpectRejected(readback);
        readback.m_imageDescriptor.m_size = { 0, std::numeric_limits<uint32_t>::max(), 1 };
        ExpectRejected(readback);
    }

    TEST_F(JEEditorLutWriterTests, RequiresExactTightBuffer)
    {
        auto readback = Readback();
        readback.m_dataBuffer->pop_back();
        ExpectRejected(readback);
        readback.m_dataBuffer->resize(129);
        ExpectRejected(readback);
        readback.m_dataBuffer.reset();
        ExpectRejected(readback);
    }

    TEST_F(JEEditorLutWriterTests, RejectsNonFiniteRgbAndAlphaWithoutCreatingOutputs)
    {
        for (size_t channel = 0; channel < 4; ++channel)
        {
            for (float value : { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
            {
                auto readback = Readback();
                float pixel[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                pixel[channel] = value;
                SetPixel(readback, 7, pixel);
                ExpectRejected(readback);
            }
        }
    }

    TEST_F(JEEditorLutWriterTests, ExistingEitherOutputIsPreservedAndNoPartialSuccessRemains)
    {
        for (const char* extension : { ".3dl", ".azasset" })
        {
            const auto base = Base(extension);
            const auto path = base + extension;
            AZ::IO::SystemFile existing;
            ASSERT_TRUE(existing.Open(path.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE_NEW | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY));
            ASSERT_EQ(existing.Write("original", 8), 8);
            existing.Close();
            EXPECT_FALSE(JE::Editor::Render::Scene::WriteLutSource(Readback(), base).IsSuccess());
            EXPECT_EQ(Read(path), "original");
            const auto other = base + (AZStd::string_view(extension) == ".3dl" ? ".azasset" : ".3dl");
            EXPECT_FALSE(AZ::IO::SystemFile::Exists(other.c_str()));
        }
        EXPECT_EQ(FileCount(), 2);
    }

    TEST_F(JEEditorLutWriterTests, DirectoryAtSecondOutputIsPreservedWithoutPartialOutputs)
    {
        ASSERT_TRUE(AZ::IO::SystemFile::CreateDir((Base() + ".azasset").c_str()));
        EXPECT_FALSE(JE::Editor::Render::Scene::WriteLutSource(Readback(), Base()).IsSuccess());
        EXPECT_FALSE(AZ::IO::SystemFile::Exists((Base() + ".3dl").c_str()));
        EXPECT_TRUE(AZ::IO::SystemFile::IsDirectory((Base() + ".azasset").c_str()));
        EXPECT_EQ(FileCount(), 0);
    }

    TEST_F(JEEditorLutWriterTests, MissingParentAndRelativeBaseFailWithoutCreatingDirectories)
    {
        const auto base = Base("missing/lut");
        EXPECT_FALSE(JE::Editor::Render::Scene::WriteLutSource(Readback(), base).IsSuccess());
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(Base("missing").c_str()));
        EXPECT_FALSE(JE::Editor::Render::Scene::WriteLutSource(Readback(), "relative/lut").IsSuccess());
        EXPECT_EQ(FileCount(), 0);
    }

    TEST_F(JEEditorLutWriterTests, ValidatesLoadedCardinalityIntervalsAndTwelveBitValues)
    {
        JE::Render::Features::LookupTableAsset loaded;
        loaded.m_intervals = { 0, 1023 };
        loaded.m_values.resize(24, 4095);
        ASSERT_TRUE(JE::Editor::Render::Scene::ValidateLookupTable(loaded).IsSuccess());
        loaded.m_values.pop_back();
        EXPECT_FALSE(JE::Editor::Render::Scene::ValidateLookupTable(loaded).IsSuccess());
        loaded.m_values.resize(24, 0);
        loaded.m_values[23] = 4096;
        EXPECT_FALSE(JE::Editor::Render::Scene::ValidateLookupTable(loaded).IsSuccess());
        loaded.m_values[23] = 0;
        loaded.m_intervals = { 0, 1024 };
        EXPECT_FALSE(JE::Editor::Render::Scene::ValidateLookupTable(loaded).IsSuccess());
        loaded.m_intervals = { 0, 1023, 512, 1023 };
        loaded.m_values.resize(192, 0);
        EXPECT_FALSE(JE::Editor::Render::Scene::ValidateLookupTable(loaded).IsSuccess());
        loaded.m_intervals = { 0 };
        loaded.m_values.resize(3);
        EXPECT_FALSE(JE::Editor::Render::Scene::ValidateLookupTable(loaded).IsSuccess());
    }
}
