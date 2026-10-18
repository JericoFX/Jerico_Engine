/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/SubsurfaceScatteringPass.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>

#include <JE/Render/RPI/Reflect/Pass/PassTemplate.h>

#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/Pass/PassAttachment.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Public/RPISystemInterface.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<SubsurfaceScatteringPass> SubsurfaceScatteringPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<SubsurfaceScatteringPass> pass = aznew SubsurfaceScatteringPass(descriptor);
            return pass;
        }

        SubsurfaceScatteringPass::SubsurfaceScatteringPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : ComputePass(descriptor)
        { }

        void SubsurfaceScatteringPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            AZ::RHI::Size targetImageSize;
            if (m_fullscreenDispatch)
            {
                auto attachment = m_fullscreenSizeSourceBinding->GetAttachment();
                if (attachment)
                {
                    auto imageDescriptor = context.GetImageDescriptor(attachment->GetAttachmentId());
                    targetImageSize = imageDescriptor.m_size;
                }
            }

            // Update shader constant
            m_shaderResourceGroup->SetConstant(m_screenSizeInputIndex, AZ::Vector2(static_cast<float>(targetImageSize.m_width), static_cast<float>(targetImageSize.m_height)));

            ComputePass::CompileResources(context);
        }
    } // namespace Features
}   // namespace AZ
