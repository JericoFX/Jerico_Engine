/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurSettingsInterface.h>

namespace JE::Render
{
    namespace Features
    {
        // Class for controlling MotionBlur effect
        class MotionBlurPass final : public JE::Render::RPI::ComputePass,
                                     public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(MotionBlurPass);

        public:
            AZ_RTTI(MotionBlurPass, "{EA58C10C-F2D9-431B-A4A6-EB63A3118690}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(MotionBlurPass, AZ::SystemAllocator, 0);

            ~MotionBlurPass() = default;
            static JE::Render::RPI::Ptr<MotionBlurPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

            void SetSampleQuality(MotionBlur::SampleQuality sampleQuality);

        protected:
            // Behavior functions override...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            MotionBlurPass(const JE::Render::RPI::PassDescriptor& descriptor);
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            // Scope producer functions...
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            MotionBlur::SampleQuality m_sampleQuality = MotionBlur::SampleQuality::High;
            const AZ::Name m_sampleQualityShaderVariantOptionName{ "o_sampleQuality" };
            bool m_needToUpdateShaderVariant = true;

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Features
} // namespace JE::Render
