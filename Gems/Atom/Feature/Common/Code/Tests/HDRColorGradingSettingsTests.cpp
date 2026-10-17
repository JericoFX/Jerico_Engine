/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <JE/Render/RPI/Public/View.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace UnitTest
{
    class HDRColorGradingSettingsTests : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_ownsNameDictionary = !AZ::NameDictionary::IsReady();
            if (m_ownsNameDictionary)
            {
                AZ::NameDictionary::Create();
            }
            m_processor = AZStd::make_unique<AZ::Render::PostProcessFeatureProcessor>();
            m_processor->Activate();
            m_view = JE::Render::RPI::View::CreateView(AZ::Name("HDR generation regression"), JE::Render::RPI::View::UsageCamera);
            m_source = m_processor->GetOrCreateSettingsInterface(AZ::EntityId(1));
            m_source->CopyViewToBlendWeightSettings({{m_view.get(), 1.0f}});
            m_source->SetOverrideFactor(1.0f);
            m_hdr = m_source->GetOrCreateHDRColorGradingSettingsInterface();
            m_hdr->SetEnabled(true);
        }

        void TearDown() override
        {
            m_hdr = nullptr;
            m_source = nullptr;
            m_processor->Deactivate();
            m_processor.reset();
            m_view.reset();
            if (m_ownsNameDictionary)
            {
                AZ::NameDictionary::Destroy();
            }
            LeakDetectionFixture::TearDown();
        }

        void SetGeneration(
            bool generate, AZ::Render::LutResolution resolution, AZ::Render::ShaperPresetType shaper,
            float minimumExposure, float maximumExposure)
        {
            m_hdr->SetGenerateLut(generate);
            m_hdr->SetLutResolution(resolution);
            m_hdr->SetShaperPresetType(shaper);
            m_hdr->SetCustomMinExposure(minimumExposure);
            m_hdr->SetCustomMaxExposure(maximumExposure);
            m_source->OnConfigChanged();
        }

        AZ::Render::HDRColorGradingSettings* Aggregate()
        {
            m_processor->Simulate({});
            const auto aggregate = m_processor->GetLevelSettingsFromView(m_view);
            return aggregate ? aggregate->GetHDRColorGradingSettings() : nullptr;
        }

        void ExpectGeneration(
            const AZ::Render::HDRColorGradingSettings& aggregate, bool generate,
            AZ::Render::LutResolution resolution, AZ::Render::ShaperPresetType shaper,
            float minimumExposure, float maximumExposure)
        {
            EXPECT_TRUE(aggregate.GetEnabled());
            EXPECT_EQ(aggregate.GetGenerateLut(), generate);
            EXPECT_EQ(aggregate.GetLutResolution(), resolution);
            EXPECT_EQ(aggregate.GetShaperPresetType(), shaper);
            EXPECT_FLOAT_EQ(aggregate.GetCustomMinExposure(), minimumExposure);
            EXPECT_FLOAT_EQ(aggregate.GetCustomMaxExposure(), maximumExposure);
        }

        bool m_ownsNameDictionary = false;
        AZStd::unique_ptr<AZ::Render::PostProcessFeatureProcessor> m_processor;
        JE::Render::RPI::ViewPtr m_view;
        AZ::Render::PostProcessSettingsInterface* m_source = nullptr;
        AZ::Render::HDRColorGradingSettingsInterface* m_hdr = nullptr;
    };

    TEST_F(HDRColorGradingSettingsTests, FreshViewCopiesCompleteGenerationSettings)
    {
        SetGeneration(true, AZ::Render::LutResolution::Lut32x32x32, AZ::Render::ShaperPresetType::LinearCustomRange, -4.0f, 8.0f);
        const auto aggregate = Aggregate();
        ASSERT_NE(aggregate, nullptr);
        ExpectGeneration(*aggregate, true, AZ::Render::LutResolution::Lut32x32x32,
            AZ::Render::ShaperPresetType::LinearCustomRange, -4.0f, 8.0f);
    }

    TEST_F(HDRColorGradingSettingsTests, ExistingViewUpdatesFalseToTrueAndCoherentGenerationSettings)
    {
        SetGeneration(false, AZ::Render::LutResolution::Lut16x16x16, AZ::Render::ShaperPresetType::None, -6.5f, 6.5f);
        const auto initial = Aggregate();
        ASSERT_NE(initial, nullptr);
        ExpectGeneration(*initial, false, AZ::Render::LutResolution::Lut16x16x16, AZ::Render::ShaperPresetType::None, -6.5f, 6.5f);

        SetGeneration(true, AZ::Render::LutResolution::Lut32x32x32, AZ::Render::ShaperPresetType::LinearCustomRange, -4.0f, 8.0f);
        const auto updated = Aggregate();
        ASSERT_EQ(updated, initial); // Reuse the populated aggregate, exercising ApplySettingsTo rather than the first copy.
        ExpectGeneration(*updated, true, AZ::Render::LutResolution::Lut32x32x32,
            AZ::Render::ShaperPresetType::LinearCustomRange, -4.0f, 8.0f);
    }

    TEST_F(HDRColorGradingSettingsTests, ExistingViewResetsTrueToFalseAndCoherentGenerationSettings)
    {
        SetGeneration(true, AZ::Render::LutResolution::Lut64x64x64, AZ::Render::ShaperPresetType::Log2CustomRange, -8.0f, 10.0f);
        const auto initial = Aggregate();
        ASSERT_NE(initial, nullptr);
        ExpectGeneration(*initial, true, AZ::Render::LutResolution::Lut64x64x64,
            AZ::Render::ShaperPresetType::Log2CustomRange, -8.0f, 10.0f);

        SetGeneration(false, AZ::Render::LutResolution::Lut16x16x16, AZ::Render::ShaperPresetType::None, -6.5f, 6.5f);
        const auto updated = Aggregate();
        ASSERT_EQ(updated, initial);
        ExpectGeneration(*updated, false, AZ::Render::LutResolution::Lut16x16x16, AZ::Render::ShaperPresetType::None, -6.5f, 6.5f);
    }

    TEST_F(HDRColorGradingSettingsTests, ExistingViewSelectsCompleteGenerationTupleAtEffectiveAlphaThreshold)
    {
        struct BlendCase
        {
            float m_viewWeight;
            float m_overrideFactor;
            bool m_selectSource;
        };
        constexpr BlendCase cases[] = {{0.0f, 1.0f, false}, {1.0f, 0.49f, false}, {0.5f, 1.0f, true}, {1.0f, 1.0f, true}};

        SetGeneration(true, AZ::Render::LutResolution::Lut64x64x64, AZ::Render::ShaperPresetType::Log2CustomRange, -8.0f, 10.0f);
        const auto initial = Aggregate();
        ASSERT_NE(initial, nullptr);
        for (const auto& blend : cases)
        {
            SCOPED_TRACE(blend.m_viewWeight * blend.m_overrideFactor);
            m_source->CopyViewToBlendWeightSettings({{m_view.get(), 1.0f}});
            m_source->SetOverrideFactor(1.0f);
            SetGeneration(true, AZ::Render::LutResolution::Lut64x64x64, AZ::Render::ShaperPresetType::Log2CustomRange, -8.0f, 10.0f);
            ASSERT_EQ(Aggregate(), initial);
            ExpectGeneration(*initial, true, AZ::Render::LutResolution::Lut64x64x64,
                AZ::Render::ShaperPresetType::Log2CustomRange, -8.0f, 10.0f);

            m_source->CopyViewToBlendWeightSettings({{m_view.get(), blend.m_viewWeight}});
            m_source->SetOverrideFactor(blend.m_overrideFactor);
            SetGeneration(false, AZ::Render::LutResolution::Lut32x32x32, AZ::Render::ShaperPresetType::LinearCustomRange, -4.0f, 8.0f);
            const auto updated = Aggregate();
            ASSERT_EQ(updated, initial);
            if (blend.m_selectSource)
            {
                ExpectGeneration(*updated, false, AZ::Render::LutResolution::Lut32x32x32,
                    AZ::Render::ShaperPresetType::LinearCustomRange, -4.0f, 8.0f);
            }
            else
            {
                ExpectGeneration(*updated, true, AZ::Render::LutResolution::Lut64x64x64,
                    AZ::Render::ShaperPresetType::Log2CustomRange, -8.0f, 10.0f);
            }
        }
    }

    TEST_F(HDRColorGradingSettingsTests, ExistingViewLayerPriorityPreservesOneCoherentGenerationTuple)
    {
        m_source->SetPriority(100);
        SetGeneration(true, AZ::Render::LutResolution::Lut64x64x64, AZ::Render::ShaperPresetType::Log2CustomRange, -8.0f, 10.0f);
        const auto initial = Aggregate();
        ASSERT_NE(initial, nullptr);

        auto* otherSource = m_processor->GetOrCreateSettingsInterface(AZ::EntityId(2));
        otherSource->SetPriority(0);
        otherSource->SetOverrideFactor(1.0f);
        otherSource->CopyViewToBlendWeightSettings({{m_view.get(), 1.0f}});
        auto* otherHdr = otherSource->GetOrCreateHDRColorGradingSettingsInterface();
        otherHdr->SetEnabled(true);
        otherHdr->SetGenerateLut(false);
        otherHdr->SetLutResolution(AZ::Render::LutResolution::Lut32x32x32);
        otherHdr->SetShaperPresetType(AZ::Render::ShaperPresetType::LinearCustomRange);
        otherHdr->SetCustomMinExposure(-4.0f);
        otherHdr->SetCustomMaxExposure(8.0f);
        otherSource->OnConfigChanged();
        ASSERT_EQ(Aggregate(), initial);
        // Existing aggregation sorts descending priority, so priority 0 applies after priority 100.
        ExpectGeneration(*initial, false, AZ::Render::LutResolution::Lut32x32x32,
            AZ::Render::ShaperPresetType::LinearCustomRange, -4.0f, 8.0f);

        otherSource->CopyViewToBlendWeightSettings({{m_view.get(), 0.49f}});
        otherSource->OnConfigChanged();
        ASSERT_EQ(Aggregate(), initial);
        ExpectGeneration(*initial, true, AZ::Render::LutResolution::Lut64x64x64,
            AZ::Render::ShaperPresetType::Log2CustomRange, -8.0f, 10.0f);
    }
}
