/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Reflect/Shader/ShaderVariantKey.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <PostProcess/ColorGrading/HDRColorGradingSettings.h>

namespace JE::Render
{
    namespace Features
    {
        /**
         *  The color grading pass.
         */
        class HDRColorGradingPass
            : public JE::Render::RPI::FullscreenTrianglePass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(HDRColorGradingPass, "{E68E31A1-DB24-4AFF-A029-456A8B74C03C}", JE::Render::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(HDRColorGradingPass, AZ::SystemAllocator);

            virtual ~HDRColorGradingPass() = default;

            //! Creates a ColorGradingPass
            static JE::Render::RPI::Ptr<HDRColorGradingPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        protected:
            HDRColorGradingPass(const JE::Render::RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

            const HDRColorGradingSettings* GetHDRColorGradingSettings() const;
            virtual void SetSrgConstants();
        private:
            AZ::RHI::ShaderInputNameIndex m_colorAdjustmentWeightIndex = "m_colorAdjustmentWeight";
            AZ::RHI::ShaderInputNameIndex m_colorGradingExposureIndex = "m_colorGradingExposure";
            AZ::RHI::ShaderInputNameIndex m_colorGradingContrastIndex = "m_colorGradingContrast";
            AZ::RHI::ShaderInputNameIndex m_colorGradingPreSaturationIndex = "m_colorGradingPreSaturation";
            AZ::RHI::ShaderInputNameIndex m_colorFilterIntensityIndex = "m_colorFilterIntensity";
            AZ::RHI::ShaderInputNameIndex m_colorFilterMultiplyIndex = "m_colorFilterMultiply";
            AZ::RHI::ShaderInputNameIndex m_colorFilterSwatchIndex = "m_colorFilterSwatch";

            AZ::RHI::ShaderInputNameIndex m_whiteBalanceWeightIndex = "m_whiteBalanceWeight";
            AZ::RHI::ShaderInputNameIndex m_whiteBalanceKelvinIndex = "m_whiteBalanceKelvin";
            AZ::RHI::ShaderInputNameIndex m_whiteBalanceTintIndex = "m_whiteBalanceTint";
            AZ::RHI::ShaderInputNameIndex m_whiteBalanceLuminancePreservationIndex = "m_whiteBalanceLuminancePreservation";

            AZ::RHI::ShaderInputNameIndex m_splitToneBalanceIndex = "m_splitToneBalance";
            AZ::RHI::ShaderInputNameIndex m_splitToneWeightIndex = "m_splitToneWeight";
            AZ::RHI::ShaderInputNameIndex m_splitToneShadowsColorIndex = "m_splitToneShadowsColor";
            AZ::RHI::ShaderInputNameIndex m_splitToneHighlightsColorIndex = "m_splitToneHighlightsColor";

            AZ::RHI::ShaderInputNameIndex m_smhShadowsStartIndex = "m_smhShadowsStart";
            AZ::RHI::ShaderInputNameIndex m_smhShadowsEndIndex = "m_smhShadowsEnd";
            AZ::RHI::ShaderInputNameIndex m_smhHighlightsStartIndex = "m_smhHighlightsStart";
            AZ::RHI::ShaderInputNameIndex m_smhHighlightsEndIndex = "m_smhHighlightsEnd";
            AZ::RHI::ShaderInputNameIndex m_smhWeightIndex = "m_smhWeight";
            AZ::RHI::ShaderInputNameIndex m_smhShadowsColorIndex = "m_smhShadowsColor";
            AZ::RHI::ShaderInputNameIndex m_smhMidtonesColorIndex = "m_smhMidtonesColor";
            AZ::RHI::ShaderInputNameIndex m_smhHighlightsColorIndex = "m_smhHighlightsColor";

            AZ::RHI::ShaderInputNameIndex m_channelMixingRedIndex = "m_channelMixingRed";
            AZ::RHI::ShaderInputNameIndex m_channelMixingGreenIndex = "m_channelMixingGreen";
            AZ::RHI::ShaderInputNameIndex m_channelMixingBlueIndex = "m_channelMixingBlue";

            AZ::RHI::ShaderInputNameIndex m_finalAdjustmentWeightIndex = "m_finalAdjustmentWeight";
            AZ::RHI::ShaderInputNameIndex m_colorGradingPostSaturationIndex = "m_colorGradingPostSaturation";
            AZ::RHI::ShaderInputNameIndex m_colorGradingHueShiftIndex = "m_colorGradingHueShift";
        };
    } // namespace Features
} // namespace JE::Render
