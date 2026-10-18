/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>

namespace JE::Render
{
    namespace Features
    {
        //! Child pass for downsampling DepthLinear mips
        class ReflectionScreenSpaceDownsampleDepthLinearChildPass
            : public JE::Render::RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceDownsampleDepthLinearChildPass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ReflectionScreenSpaceDownsampleDepthLinearChildPass, "Render::ReflectionScreenSpaceDownsampleDepthLinearChildPass", "{3863028B-3CA9-4F45-A7CC-EA2885593F83}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(JE::Render::Features::ReflectionScreenSpaceDownsampleDepthLinearChildPass, AZ::SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionScreenSpaceDownsampleDepthLinearChildPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetMipLevel(uint32_t mipLevel) { m_mipLevel = mipLevel; }

        private:
            explicit ReflectionScreenSpaceDownsampleDepthLinearChildPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            AZ::RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            AZ::RHI::ShaderInputNameIndex m_mipLevelNameIndex = "m_mipLevel";
            AZ::RHI::ShaderInputNameIndex m_halfResolutionNameIndex = "m_halfResolution";
            AZ::RHI::ShaderInputNameIndex m_previousMipLevelNameIndex = "m_previousMipLevel";
            AZ::RHI::ShaderInputNameIndex m_previousMipWidthNameIndex = "m_previousMipWidth";
            AZ::RHI::ShaderInputNameIndex m_previousMipHeightNameIndex = "m_previousMipHeight";

            bool m_updateSrg = false;
            float m_invOutputScale = 1.0f;
            uint32_t m_mipLevel = 0;
            AZ::RHI::Size m_imageSize;
            bool m_halfResolution = false;
        };
    } // namespace Features
} // namespace JE::Render
