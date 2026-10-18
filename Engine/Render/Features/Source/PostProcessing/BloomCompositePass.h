/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>

#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>

#include <JE/Render/RPI/Reflect/Pass/DownsampleMipChainPassData.h>
#include <JE/Render/Features/PostProcess/Bloom/BloomConstants.h>

namespace JE::Render
{
    namespace Features
    {
        class BloomCompositeChildPass;

        // Pass for combining contributions of all downsampled images to the render target
        class BloomCompositePass
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(BloomCompositePass);

        public:
            AZ_RTTI(BloomCompositePass, "{02E41C48-5CC7-4277-B66E-009E4D6D32BA}", ParentPass);
            AZ_CLASS_ALLOCATOR(BloomCompositePass, AZ::SystemAllocator);
            virtual ~BloomCompositePass() = default;

            //! Creates a BloomCompositePass
            static JE::Render::RPI::Ptr<BloomCompositePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);
        protected:
            BloomCompositePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behaviour overrides...
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void GetAttachmentInfo();
            void UpdateParameters();
            void BuildChildPasses();
            void UpdateChildren();
            void CreateBinding(BloomCompositeChildPass* pass, uint32_t mipLevel);

            JE::Render::RPI::DownsampleMipChainPassData m_passData;

            uint32_t m_inputWidth = 0;
            uint32_t m_inputHeight = 0;

            uint32_t m_outputWidth = 0;
            uint32_t m_outputHeight = 0;

            AZStd::vector<AZ::Vector3> m_tintData = {
                JE::Render::Features::Bloom::DefaultTint,
                JE::Render::Features::Bloom::DefaultTint,
                JE::Render::Features::Bloom::DefaultTint,
                JE::Render::Features::Bloom::DefaultTint,
                JE::Render::Features::Bloom::DefaultTint
            };

            float m_intensity = JE::Render::Features::Bloom::DefaultIntensity;

            bool m_enableBicubic = JE::Render::Features::Bloom::DefaultEnableBicubicFilter;
        };

        // Pass spawned by parent composite pass for additive upsampling
        class BloomCompositeChildPass
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(BloomCompositeChildPass);

            friend class BloomBlurPass;

        public:
            AZ_RTTI(BloomCompositeChildPass, "{85D3FE9B-D347-40D6-B666-B4DF855F5B80}", RenderPass);
            AZ_CLASS_ALLOCATOR(BloomCompositeChildPass, AZ::SystemAllocator);
            virtual ~BloomCompositeChildPass() = default;

            //! Creates a BloomCompositeChildPass
            static JE::Render::RPI::Ptr<BloomCompositeChildPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void UpdateParameters(uint32_t sourceMip, uint32_t sourceImageWidth, uint32_t sourceImageHeight, uint32_t targetImageWidth, uint32_t targetImageHeight, bool enableBicubic, AZ::Vector3 tint, float intensity);

        protected:
            BloomCompositeChildPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            AZ::RHI::ShaderInputNameIndex m_intensityInputIndex = "m_intensity";
            AZ::RHI::ShaderInputNameIndex m_sourceImageSizeInputIndex = "m_sourceImageSize";
            AZ::RHI::ShaderInputNameIndex m_sourceImageTexelSizeInputIndex = "m_sourceImageTexelSize";
            AZ::RHI::ShaderInputNameIndex m_targetImageSizeInputIndex = "m_targetImageSize";
            AZ::RHI::ShaderInputNameIndex m_sourceMipLevelInputIndex = "m_sourceMipLevel";
            AZ::RHI::ShaderInputNameIndex m_enableBicubicInputIndex = "m_enableBicubic";
            AZ::RHI::ShaderInputNameIndex m_tintInputIndex = "m_tint";

            uint32_t m_targetImageWidth;
            uint32_t m_targetImageHeight;
        };
    } // namespace Features
} // namespace JE::Render
