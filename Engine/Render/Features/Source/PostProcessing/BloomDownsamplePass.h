/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/Features/PostProcess/Bloom/BloomConstants.h>

namespace JE::Render
{
    namespace Features
    {
        class ShaderResourceGroup;

        class BloomDownsamplePass
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(BloomDownsamplePass);

        public:
            AZ_RTTI(BloomDownsamplePass, "{D1CA5F45-70DB-4130-B5FA-147EFB010B1F}", RenderPass);
            AZ_CLASS_ALLOCATOR(BloomDownsamplePass, AZ::SystemAllocator);
            virtual ~BloomDownsamplePass() = default;

            //! Creates a BloomDownsamplePass
            static JE::Render::RPI::Ptr<BloomDownsamplePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        protected:
            BloomDownsamplePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void BuildOutAttachmentBinding();
            AZ::Vector4 CalThresholdConstants();

            // output texture vertical dimension required by compute shader
            AZ::RHI::ShaderInputNameIndex m_sourceImageTexelSizeInputIndex = "m_sourceImageTexelSize";
            AZ::RHI::ShaderInputNameIndex m_thresholdConstantsInputIndex = "m_thresholdConstants";

            float m_threshold = JE::Render::Features::Bloom::DefaultThreshold;
            float m_knee = JE::Render::Features::Bloom::DefaultKnee;

            bool m_srgNeedsUpdate = true;
        };
    } // namespace Features
} // namespace JE::Render
