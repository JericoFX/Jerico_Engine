/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PostProcessing/HDRColorGradingPass.h>
#include <JE/Render/Features/ColorGrading/LutGenerationCaptureBus.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderVariantKey.h>

#include <PostProcess/ColorGrading/HDRColorGradingSettings.h>
#include <JE/Render/Features/DisplayMapper/DisplayMapperFeatureProcessorInterface.h>

namespace JE::Render
{
    namespace Features
    {
        // Performs color grading on an identity LUT strip
        class LutGenerationPass
            : public JE::Render::Features::HDRColorGradingPass
        {
        public:
            AZ_RTTI(LutGenerationPass, "{C21DABA8-B538-4C80-BA18-5B97CC9259E5}", JE::Render::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(LutGenerationPass, AZ::SystemAllocator);

            virtual ~LutGenerationPass() = default;

            //! Creates a ColorGradingPass
            static JE::Render::RPI::Ptr<LutGenerationPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        protected:
            LutGenerationPass(const JE::Render::RPI::PassDescriptor& descriptor);

            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;
            bool IsEnabled() const override;
        private:
            // Set viewport scissor based on output LUT resolution
            void SetViewportScissorFromImageSize(const AZ::RHI::Size& imageSize);

            JE::Render::Features::LutGenerationCaptureMetadata m_frameEncoding;
            bool m_frameEncodingValid = false;

            AZ::RHI::ShaderInputNameIndex m_lutResolutionIndex = "m_lutResolution";
            AZ::RHI::ShaderInputNameIndex m_lutShaperTypeIndex = "m_shaperType";
            AZ::RHI::ShaderInputNameIndex m_lutShaperBiasIndex = "m_shaperBias";
            AZ::RHI::ShaderInputNameIndex m_lutShaperScaleIndex = "m_shaperScale";
        };

    } // namespace Features
} // namespace JE::Render
