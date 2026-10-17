/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <JE/Render/RPI/Public/View.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace UnitTest
{
    class LookModificationSettingsTests
        : public LeakDetectionFixture
        , private AZ::Debug::TraceMessageBus::Handler
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
            m_view = JE::Render::RPI::View::CreateView(AZ::Name("Look LUT stack regression"), JE::Render::RPI::View::UsageCamera);
            BusConnect();
        }

        void TearDown() override
        {
            BusDisconnect();
            m_processor->Deactivate();
            m_processor.reset();
            m_view.reset();
            if (m_ownsNameDictionary)
            {
                AZ::NameDictionary::Destroy();
            }
            LeakDetectionFixture::TearDown();
        }

        bool OnPreWarning(const char* window, const char*, int, const char*, const char*) override
        {
            if (AZStd::string_view(window) == "LookModificationSettings")
            {
                ++m_lutWarnings;
            }
            return false; // Keep the existing test environment's trace handling.
        }

        static AZ::Data::Asset<JE::Render::RPI::AnyAsset> Lut(AZ::u32 subId)
        {
            // Valid identity only: aggregation must not need an AssetManager, GPU or loaded LUT data.
            return {AZ::Data::AssetId(AZ::Uuid::CreateString("{82B2A28F-A53D-4E44-AB95-39FF771FA5E7}"), subId),
                azrtti_typeid<JE::Render::RPI::AnyAsset>()};
        }

        AZ::Render::PostProcessSettingsInterface* AddLayer(AZ::u64 entityId, AZ::u32 lutId, int priority = 0)
        {
            auto* source = m_processor->GetOrCreateSettingsInterface(AZ::EntityId(entityId));
            source->CopyViewToBlendWeightSettings({{m_view.get(), 1.0f}});
            source->SetOverrideFactor(1.0f);
            source->SetPriority(priority);
            auto* look = source->GetOrCreateLookModificationSettingsInterface();
            look->SetEnabled(true);
            look->SetColorGradingLut(Lut(lutId));
            source->OnConfigChanged();
            return source;
        }

        AZ::Render::LookModificationSettings* Aggregate()
        {
            m_processor->Simulate({});
            auto* settings = m_processor->GetLevelSettingsFromView(m_view);
            auto* look = settings ? settings->GetLookModificationSettings() : nullptr;
            // Match the render consumer, which prepares the stack before reading its entries.
            if (look)
            {
                look->PrepareLutBlending();
            }
            return look;
        }

        void ExpectOnlyLut(AZ::Render::LookModificationSettings* look, AZ::u32 lutId)
        {
            ASSERT_NE(look, nullptr);
            ASSERT_EQ(look->GetLutBlendStackSize(), 1u);
            EXPECT_EQ(look->GetLutBlendItem(0).m_asset.GetId(), Lut(lutId).GetId());
            EXPECT_FLOAT_EQ(look->GetLutBlendItem(0).m_overrideStrength, 1.0f);
        }

        void ExpectEmpty(AZ::Render::LookModificationSettings* look)
        {
            EXPECT_EQ(look ? look->GetLutBlendStackSize() : 0u, 0u);
        }

        bool m_ownsNameDictionary = false;
        size_t m_lutWarnings = 0;
        AZStd::unique_ptr<AZ::Render::PostProcessFeatureProcessor> m_processor;
        JE::Render::RPI::ViewPtr m_view;
    };

    TEST_F(LookModificationSettingsTests, OneLutRemainsOneAcrossTenViewUpdates)
    {
        auto* source = AddLayer(1, 1);
        for (int frame = 0; frame < 10; ++frame)
        {
            SCOPED_TRACE(frame);
            // View/layer changes in the editor cause a new aggregation on the same view.
            source->OnConfigChanged();
            ExpectOnlyLut(Aggregate(), 1);
        }
        EXPECT_EQ(m_lutWarnings, 0u);
    }

    TEST_F(LookModificationSettingsTests, ReplacingLutRemovesPreviousAssetFromViewStack)
    {
        auto* source = AddLayer(1, 1);
        ExpectOnlyLut(Aggregate(), 1);
        source->GetOrCreateLookModificationSettingsInterface()->SetColorGradingLut(Lut(2));
        source->OnConfigChanged();
        ExpectOnlyLut(Aggregate(), 2);
        EXPECT_EQ(m_lutWarnings, 0u);
    }

    TEST_F(LookModificationSettingsTests, DisablingLookRemovesLutFromExistingViewStack)
    {
        auto* source = AddLayer(1, 1);
        ExpectOnlyLut(Aggregate(), 1);
        source->GetOrCreateLookModificationSettingsInterface()->SetEnabled(false);
        source->OnConfigChanged();
        ExpectEmpty(Aggregate());
        EXPECT_EQ(m_lutWarnings, 0u);
    }

    TEST_F(LookModificationSettingsTests, RemovingLookSettingsRemovesLutFromExistingViewStack)
    {
        auto* source = AddLayer(1, 1);
        ExpectOnlyLut(Aggregate(), 1);
        source->RemoveLookModificationSettingsInterface();
        ExpectEmpty(Aggregate());
        EXPECT_EQ(m_lutWarnings, 0u);
    }

    TEST_F(LookModificationSettingsTests, FourDistinctLayersStayFourWithoutOverflowWarnings)
    {
        for (AZ::u32 layer = 1; layer <= 4; ++layer)
        {
            AddLayer(layer, layer, static_cast<int>(layer));
        }
        for (int frame = 0; frame < 10; ++frame)
        {
            SCOPED_TRACE(frame);
            m_processor->OnPostProcessSettingsChanged();
            auto* look = Aggregate();
            ASSERT_NE(look, nullptr);
            ASSERT_EQ(look->GetLutBlendStackSize(), 4u);
            for (size_t index = 0; index < 4; ++index)
            {
                // Existing aggregation order is descending layer priority.
                EXPECT_EQ(look->GetLutBlendItem(index).m_asset.GetId(), Lut(static_cast<AZ::u32>(4 - index)).GetId());
            }
        }
        EXPECT_EQ(m_lutWarnings, 0u);
    }
}
