/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceBlurChildPass.h"

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<ReflectionScreenSpaceBlurChildPass> ReflectionScreenSpaceBlurChildPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ReflectionScreenSpaceBlurChildPass> pass = aznew ReflectionScreenSpaceBlurChildPass(descriptor);
            return pass;
        }

        ReflectionScreenSpaceBlurChildPass::ReflectionScreenSpaceBlurChildPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceBlurChildPass::FrameBeginInternal(FramePrepareParams params)
        {
            // get attachment size
            JE::Render::RPI::PassAttachment* inputAttachment = GetInputBinding(0).GetAttachment().get();
            AZ_Assert(inputAttachment, "ReflectionScreenSpaceBlurChildPass: Input binding has no attachment!");

            AZ::RHI::Size size = inputAttachment->m_descriptor.m_image.m_size;

            if (m_imageSize != size)
            {
                m_imageSize = size;
                m_invOutputScale = (m_passType == PassType::Vertical) ? static_cast<float>(pow(2.0f, m_mipLevel)) : 1.0f;
                m_updateSrg = true;
            }

            float outputScale = 1.0f / m_invOutputScale;
            uint32_t outputWidth = static_cast<uint32_t>(m_imageSize.m_width * outputScale);
            uint32_t outputHeight = static_cast<uint32_t>(m_imageSize.m_height * outputScale);

            params.m_viewportState = AZ::RHI::Viewport(0, static_cast<float>(outputWidth), 0, static_cast<float>(outputHeight));
            params.m_scissorState = AZ::RHI::Scissor(0, 0, outputWidth, outputHeight);

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void ReflectionScreenSpaceBlurChildPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            if (m_updateSrg)
            {
                m_shaderResourceGroup->SetConstant(m_invOutputScaleNameIndex, m_invOutputScale);
                m_shaderResourceGroup->SetConstant(m_mipLevelNameIndex, m_mipLevel);

                m_updateSrg = false;
            }

            FullscreenTrianglePass::CompileResources(context);
        }

    } // namespace Features
} // namespace JE::Render
