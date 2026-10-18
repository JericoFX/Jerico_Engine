/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>

#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>

#include <JE/Render/RPI/Reflect/Pass/DownsampleMipChainPassData.h>

#include <Atom/RHI/ConstantsData.h>
#include <JE/Render/Features/PostProcess/Bloom/BloomConstants.h>

namespace JE::Render
{
    namespace Features
    {
        class BloomBlurChildPass;

        // Pass for blurring downsampled images
        class BloomBlurPass
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(BloomBlurPass);

        public:
            AZ_RTTI(BloomBlurPass, "{02E41C48-5CC7-4277-B66E-009E4D6D32BA}", ParentPass);
            AZ_CLASS_ALLOCATOR(BloomBlurPass, AZ::SystemAllocator);
            virtual ~BloomBlurPass() = default;

            //! Creates a BloomBlurPass
            static JE::Render::RPI::Ptr<BloomBlurPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);
        protected:
            BloomBlurPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behaviour overrides...
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void GetInputInfo();
            void UpdateParameters();
            void BuildKernelData();
            void BuildChildPasses();
            void UpdateChildren();
            void CreateBinding(BloomBlurChildPass* pass, uint32_t mipLevel, bool isHorizontalPass);

            void GenerateWeightOffset(float sigma, uint32_t kernelRadius);
            void PrepareBuffer(uint32_t blurStageIndex);

            float KernelRadiusClamp(float radius);

            float Gaussian1D(float x, float sigma);

            JE::Render::RPI::DownsampleMipChainPassData m_passData;

            AZStd::vector<AZStd::vector<float>> m_weightData;
            AZStd::vector<AZStd::vector<float>> m_offsetData;
            AZStd::vector<uint32_t> m_kernelRadiusData;

            AZStd::vector<AZ::Data::Instance<JE::Render::RPI::Buffer>> m_weightBuffer;
            AZStd::vector<AZ::Data::Instance<JE::Render::RPI::Buffer>> m_offsetBuffer;

            AZStd::vector<float> m_kernelScreenPercents = {
                JE::Render::Features::Bloom::DefaultScreenPercentStage0,
                JE::Render::Features::Bloom::DefaultScreenPercentStage1,
                JE::Render::Features::Bloom::DefaultScreenPercentStage2,
                JE::Render::Features::Bloom::DefaultScreenPercentStage3,
                JE::Render::Features::Bloom::DefaultScreenPercentStage4
            };

            float m_kernelSizeScale = JE::Render::Features::Bloom::DefaultKernelSizeScale;

            uint32_t m_inputWidth = 0;
            uint32_t m_inputHeight = 0;

            uint8_t m_stageCount = JE::Render::Features::Bloom::MaxStageCount;

            bool m_paramsUpdated = true;
        };

        // Child pass spawned by parent blur pass, one child do gaussian blur on single downsampled level in one direction
        class BloomBlurChildPass
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(BloomBlurChildPass);

            friend class BloomBlurPass;

        public:
            AZ_RTTI(BloomBlurChildPass, "{85D3FE9B-D347-40D6-B666-B4DF855F5B80}", RenderPass);
            AZ_CLASS_ALLOCATOR(BloomBlurChildPass, AZ::SystemAllocator);
            virtual ~BloomBlurChildPass() = default;

            //! Creates a BloomBlurChildPass
            static JE::Render::RPI::Ptr<BloomBlurChildPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void UpdateParameters(
                AZ::Data::Instance<JE::Render::RPI::Buffer> offsetBuffer,
                AZ::Data::Instance<JE::Render::RPI::Buffer> weightBuffer,
                uint32_t radius,
                bool direction,
                uint32_t mipLevel,
                uint32_t imageWidth,
                uint32_t imageHeight);

        protected:
            BloomBlurChildPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            // output texture vertical dimension required by compute shader
            AZ::RHI::ShaderInputNameIndex m_offsetsInputIndex = "m_offsets";
            AZ::RHI::ShaderInputNameIndex m_weightsInputIndex = "m_weights";

            AZ::RHI::ShaderInputNameIndex m_kernelRadiusInputIndex = "m_kernelRadius";
            AZ::RHI::ShaderInputNameIndex m_directionInputIndex = "m_direction";
            AZ::RHI::ShaderInputNameIndex m_sourceImageSizeInputIndex = "m_sourceImageSize";
            AZ::RHI::ShaderInputNameIndex m_sourceImageTexelSizeInputIndex = "m_sourceImageTexelSize";
            AZ::RHI::ShaderInputNameIndex m_mipLevelInputIndex = "m_mipLevel";

            AZ::Data::Instance<JE::Render::RPI::Buffer> m_offsetBuffer;
            AZ::Data::Instance<JE::Render::RPI::Buffer> m_weightBuffer;

            uint32_t m_sourceImageWidth;
            uint32_t m_sourceImageHeight;
        };
    } // namespace Features
} // namespace JE::Render
