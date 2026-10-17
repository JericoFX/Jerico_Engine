/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/Feature/ColorGrading/LutGenerationCaptureBus.h>
#include <JE/Render/RPI/Public/View.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace UnitTest
{
    class LutGenerationCaptureTests : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_ownsNames = !AZ::NameDictionary::IsReady();
            if (m_ownsNames)
            {
                AZ::NameDictionary::Create();
            }
            m_processor = AZStd::make_unique<AZ::Render::PostProcessFeatureProcessor>();
            m_processor->Activate();
            m_view = JE::Render::RPI::View::CreateView(AZ::Name("LUT frame metadata"), JE::Render::RPI::View::UsageCamera);
        }

        void TearDown() override
        {
            m_processor->Deactivate();
            m_processor.reset();
            m_view.reset();
            if (m_ownsNames)
            {
                AZ::NameDictionary::Destroy();
            }
            LeakDetectionFixture::TearDown();
        }

        AZ::Render::HDRColorGradingSettingsInterface* Layer(
            AZ::u64 entity, AZ::u32 priority, AZ::Render::LutResolution resolution,
            AZ::Render::ShaperPresetType shaper, float minimum, float maximum)
        {
            auto* source = m_processor->GetOrCreateSettingsInterface(AZ::EntityId(entity));
            source->SetPriority(priority);
            source->SetOverrideFactor(1.0f);
            source->CopyViewToBlendWeightSettings({{m_view.get(), 1.0f}});
            auto* hdr = source->GetOrCreateHDRColorGradingSettingsInterface();
            hdr->SetEnabled(true);
            hdr->SetGenerateLut(true);
            hdr->SetLutResolution(resolution);
            hdr->SetShaperPresetType(shaper);
            hdr->SetCustomMinExposure(minimum);
            hdr->SetCustomMaxExposure(maximum);
            source->OnConfigChanged();
            return hdr;
        }

        static JE::Render::RPI::AttachmentReadback::ReadbackResult Readback(AZ::Render::FrameCaptureId id, uint32_t resolution)
        {
            JE::Render::RPI::AttachmentReadback::ReadbackResult result{};
            result.m_state = JE::Render::RPI::AttachmentReadback::ReadbackState::Success;
            result.m_attachmentType = AZ::RHI::AttachmentType::Image;
            result.m_userIdentifier = id;
            result.m_imageDescriptor.m_format = AZ::RHI::Format::R32G32B32A32_FLOAT;
            result.m_imageDescriptor.m_size = {resolution * resolution, resolution, 1};
            return result;
        }

        bool m_ownsNames = false;
        AZStd::unique_ptr<AZ::Render::PostProcessFeatureProcessor> m_processor;
        JE::Render::RPI::ViewPtr m_view;
    };

    TEST_F(LutGenerationCaptureTests, SnapshotUsesEffectiveViewTupleRatherThanRequestingController)
    {
        Layer(1, 100, AZ::Render::LutResolution::Lut16x16x16, AZ::Render::ShaperPresetType::None, -6.5f, 6.5f);
        auto* winner = Layer(2, 0, AZ::Render::LutResolution::Lut32x32x32,
            AZ::Render::ShaperPresetType::Log2CustomRange, -4.0f, 8.0f);
        m_processor->Simulate({});
        auto* aggregate = m_processor->GetLevelSettingsFromView(m_view);
        ASSERT_NE(aggregate, nullptr);
        const auto* hdr = aggregate->GetHDRColorGradingSettings();
        ASSERT_NE(hdr, nullptr);
        const auto snapshot = JE::ColorGrading::LutGenerationCaptureMetadata::FromSettings(*hdr, 7);
        EXPECT_TRUE(snapshot.m_generateLut);
        EXPECT_EQ(snapshot.m_resolution, AZ::Render::LutResolution::Lut32x32x32);
        EXPECT_EQ(snapshot.m_shaper, AZ::Render::ShaperPresetType::Log2CustomRange);
        EXPECT_FLOAT_EQ(snapshot.m_minimumExposure, -4.0f);
        EXPECT_FLOAT_EQ(snapshot.m_maximumExposure, 8.0f);
        EXPECT_TRUE(snapshot.MatchesReadback(7, Readback(7, 32)));

        winner->SetLutResolution(AZ::Render::LutResolution::Lut64x64x64);
        winner->SetShaperPresetType(AZ::Render::ShaperPresetType::LinearCustomRange);
        winner->SetCustomMinExposure(-8.0f);
        winner->SetCustomMaxExposure(10.0f);
        winner->OnConfigChanged();
        m_processor->Simulate({});
        ASSERT_EQ(m_processor->GetLevelSettingsFromView(m_view), aggregate);
        EXPECT_EQ(aggregate->GetHDRColorGradingSettings()->GetLutResolution(), AZ::Render::LutResolution::Lut64x64x64);
        EXPECT_TRUE(snapshot.MatchesReadback(7, Readback(7, 32)));
        EXPECT_FALSE(snapshot.MatchesReadback(7, Readback(7, 64)));
        EXPECT_EQ(snapshot.m_resolution, AZ::Render::LutResolution::Lut32x32x32);
        EXPECT_EQ(snapshot.m_shaper, AZ::Render::ShaperPresetType::Log2CustomRange);
        EXPECT_FLOAT_EQ(snapshot.m_minimumExposure, -4.0f);
        EXPECT_FLOAT_EQ(snapshot.m_maximumExposure, 8.0f);
    }

    TEST_F(LutGenerationCaptureTests, CaptureRequiresExactIdentifierResolutionAndImageLayout)
    {
        auto* hdr = Layer(1, 0, AZ::Render::LutResolution::Lut16x16x16, AZ::Render::ShaperPresetType::None, 0.0f, 0.0f);
        const auto metadata = JE::ColorGrading::LutGenerationCaptureMetadata::FromSettings(*hdr, 9);
        auto readback = Readback(9, 16);
        EXPECT_TRUE(metadata.MatchesReadback(9, readback));
        EXPECT_FALSE(metadata.MatchesReadback(8, readback));
        EXPECT_FALSE(metadata.MatchesReadback(AZ::Render::InvalidFrameCaptureId, readback));
        readback.m_userIdentifier = 8;
        EXPECT_FALSE(metadata.MatchesReadback(9, readback));
        readback = Readback(9, 32);
        EXPECT_FALSE(metadata.MatchesReadback(9, readback));
        readback = Readback(9, 16);
        --readback.m_imageDescriptor.m_size.m_width;
        EXPECT_FALSE(metadata.MatchesReadback(9, readback));
        readback = Readback(9, 16);
        --readback.m_imageDescriptor.m_size.m_height;
        EXPECT_FALSE(metadata.MatchesReadback(9, readback));
        readback = Readback(9, 16);
        readback.m_imageDescriptor.m_size.m_depth = 2;
        EXPECT_FALSE(metadata.MatchesReadback(9, readback));
        readback = Readback(9, 16);
        readback.m_imageDescriptor.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        EXPECT_FALSE(metadata.MatchesReadback(9, readback));
        readback = Readback(9, 16);
        readback.m_attachmentType = AZ::RHI::AttachmentType::Buffer;
        EXPECT_FALSE(metadata.MatchesReadback(9, readback));
        readback = Readback(9, 16);
        readback.m_state = JE::Render::RPI::AttachmentReadback::ReadbackState::Failed;
        EXPECT_FALSE(metadata.MatchesReadback(9, readback));
        EXPECT_FALSE(JE::ColorGrading::LutGenerationCaptureMetadata{}.MatchesReadback(9, Readback(9, 16)));
    }

    TEST_F(LutGenerationCaptureTests, DisabledWinningGenerationTupleCannotAuthorizeReadback)
    {
        Layer(1, 100, AZ::Render::LutResolution::Lut16x16x16, AZ::Render::ShaperPresetType::None, 0.0f, 0.0f);
        auto* winner = Layer(2, 0, AZ::Render::LutResolution::Lut32x32x32,
            AZ::Render::ShaperPresetType::LinearCustomRange, -4.0f, 8.0f);
        winner->SetGenerateLut(false);
        winner->OnConfigChanged();
        m_processor->Simulate({});
        auto* aggregate = m_processor->GetLevelSettingsFromView(m_view);
        ASSERT_NE(aggregate, nullptr);
        const auto* hdr = aggregate->GetHDRColorGradingSettings();
        ASSERT_NE(hdr, nullptr);
        const auto metadata = JE::ColorGrading::LutGenerationCaptureMetadata::FromSettings(*hdr, 9);
        EXPECT_FALSE(metadata.m_generateLut);
        EXPECT_FALSE(metadata.MatchesReadback(9, Readback(9, 32)));
        // The real pass is disabled in this case; it must time out, never publish fabricated metadata.
    }

    TEST_F(LutGenerationCaptureTests, NotificationRoutesOnlyToTheReservedCaptureAndStopsAfterDisconnect)
    {
        struct Receiver final : JE::ColorGrading::LutGenerationCaptureNotificationBus::Handler
        {
            void OnLutGenerationCapture(const JE::ColorGrading::LutGenerationCaptureMetadata& metadata) override
            {
                ++m_calls;
                m_metadata = metadata;
            }
            int m_calls = 0;
            JE::ColorGrading::LutGenerationCaptureMetadata m_metadata;
        } receiver;
        auto* hdr = Layer(1, 0, AZ::Render::LutResolution::Lut16x16x16, AZ::Render::ShaperPresetType::None, 0.0f, 0.0f);
        const auto metadata = JE::ColorGrading::LutGenerationCaptureMetadata::FromSettings(*hdr, 11);
        receiver.BusConnect(11);
        JE::ColorGrading::LutGenerationCaptureNotificationBus::Event(10,
            &JE::ColorGrading::LutGenerationCaptureNotifications::OnLutGenerationCapture, metadata);
        EXPECT_EQ(receiver.m_calls, 0);
        JE::ColorGrading::LutGenerationCaptureNotificationBus::Event(11,
            &JE::ColorGrading::LutGenerationCaptureNotifications::OnLutGenerationCapture, metadata);
        EXPECT_EQ(receiver.m_calls, 1);
        EXPECT_EQ(receiver.m_metadata.m_captureId, 11);
        EXPECT_TRUE(receiver.m_metadata.MatchesReadback(11, Readback(11, 16)));
        receiver.BusDisconnect();
        JE::ColorGrading::LutGenerationCaptureNotificationBus::Event(11,
            &JE::ColorGrading::LutGenerationCaptureNotifications::OnLutGenerationCapture, metadata);
        EXPECT_EQ(receiver.m_calls, 1);
    }
}
