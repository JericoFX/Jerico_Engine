/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionCopyFrameBufferPass.h"
#include "ReflectionScreenSpaceTracePass.h"
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<ReflectionCopyFrameBufferPass> ReflectionCopyFrameBufferPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ReflectionCopyFrameBufferPass> pass = aznew ReflectionCopyFrameBufferPass(descriptor);
            return pass;
        }

        ReflectionCopyFrameBufferPass::ReflectionCopyFrameBufferPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionCopyFrameBufferPass::BuildInternal()
        {
            JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceTracePass"), GetRenderPipeline());
            JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                {
                    JE::Render::Features::ReflectionScreenSpaceTracePass* tracePass = azrtti_cast<ReflectionScreenSpaceTracePass*>(pass);
                    AZ::Data::Instance<JE::Render::RPI::AttachmentImage>& frameBufferAttachment = tracePass->GetPreviousFrameImageAttachment();

                    JE::Render::RPI::PassAttachmentBinding& outputBinding = GetOutputBinding(0);
                    AttachImageToSlot(outputBinding.m_name, frameBufferAttachment);

                    return JE::Render::RPI::PassFilterExecutionFlow::StopVisitingPasses;
                });

            FullscreenTrianglePass::BuildInternal();
        }
    } // namespace Features
} // namespace JE::Render
