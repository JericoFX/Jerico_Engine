/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzFramework/Windowing/WindowBus.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <JE/Render/RPI/Reflect/Shader/ShaderVariantKey.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

#include <JE/Render/Features/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace JE::Render
{
    namespace Features
    {
        static const char* const LookModificationTransformPassTemplateName{ "LookModificationTransformTemplate" };

        /**
         *  The look modification composite pass. If color grading LUTs are enabled, this pass will apply the blended LUT.
         */
        class LookModificationCompositePass
            : public JE::Render::RPI::FullscreenTrianglePass
            , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(LookModificationCompositePass, "{D7DF3E8A-B642-4D51-ABC2-ADB2B60FCE1D}", JE::Render::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(LookModificationCompositePass, AZ::SystemAllocator);

            enum class SampleQuality : uint8_t
            {
                Linear = 0,
                BSpline7Tap = 1,
                BSpline19Tap = 2,
            };

            virtual ~LookModificationCompositePass() = default;

            //! Creates a LookModificationPass
            static JE::Render::RPI::Ptr<LookModificationCompositePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            //! Set whether exposure control is enabled
            void SetExposureControlEnabled(bool enabled);

            //! Set shaper parameters
            void SetShaperParameters(const AZ::Render::ShaperParams& shaperParams);

            void SetSampleQuality(SampleQuality sampleQuality);

        protected:
            LookModificationCompositePass(const JE::Render::RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) final;

        private:
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            // Scope producer functions...
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            void UpdateExposureFeatureState();
            void UpdateLookModificationFeatureState();

            void SetColorGradingLutEnabled(bool enabled);

            bool m_exposureControlEnabled = false;
            bool m_colorGradingLutEnabled = false;
            SampleQuality m_sampleQuality = SampleQuality::Linear;

            JE::Render::Features::DisplayMapperLut m_blendedColorGradingLut;
            AZ::Render::ShaperParams m_colorGradingShaperParams;

            const AZ::Name m_exposureShaderVariantOptionName{ "o_enableExposureControlFeature" };
            const AZ::Name m_colorGradingShaderVariantOptionName{ "o_enableColorGradingLut" };
            const AZ::Name m_lutSampleQualityShaderVariantOptionName{ "o_lutSampleQuality" };

            bool m_needToUpdateShaderVariant = true;

            AZ::RHI::ShaderInputNameIndex m_shaderColorGradingLutImageIndex = "m_gradingLut";
            AZ::RHI::ShaderInputNameIndex m_shaderColorGradingShaperTypeIndex = "m_shaperType";
            AZ::RHI::ShaderInputNameIndex m_shaderColorGradingShaperBiasIndex = "m_shaperBias";
            AZ::RHI::ShaderInputNameIndex m_shaderColorGradingShaperScaleIndex = "m_shaperScale";
        };
    } // namespace Features
} // namespace JE::Render
