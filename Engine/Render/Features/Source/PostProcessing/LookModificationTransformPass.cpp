/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Pass/Specific/SwapChainPass.h>

#include <PostProcessing/LookModificationTransformPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcessing/BlendColorGradingLutsPass.h>
#include <PostProcessing/LookModificationCompositePass.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<LookModificationPass> LookModificationPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<LookModificationPass> pass = aznew LookModificationPass(descriptor);
            return pass;
        }

        LookModificationPass::LookModificationPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ParentPass(descriptor)
        {
            AzFramework::NativeWindowHandle windowHandle = nullptr;
            AzFramework::WindowSystemRequestBus::BroadcastResult(
                windowHandle,
                &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);
        }

        void LookModificationPass::BuildInternal()
        {
            m_pipelineOutput = FindAttachmentBinding(AZ::Name("PipelineOutput"));
            ParentPass::BuildInternal();
        }

        void LookModificationPass::FrameBeginInternal([[maybe_unused]] FramePrepareParams params)
        {
            // Get swap chain format
            AZ::RHI::Format swapChainFormat = AZ::RHI::Format::Unknown;
            if (m_pipelineOutput && m_pipelineOutput->GetAttachment())
            {
                swapChainFormat = m_pipelineOutput->GetAttachment()->m_descriptor.m_image.m_format;
            }

            // Update the children passes
            JE::Render::RPI::Ptr<BlendColorGradingLutsPass> blendPass = FindChildPass<BlendColorGradingLutsPass>();
            if (blendPass)
            {
                auto commonShaperParams = blendPass->GetCommonShaperParams();
                if (commonShaperParams)
                {
                    m_shaperParams = *commonShaperParams;
                }
                else
                {
                    // Mix of shapers used, so shape them based on the output transform type.
                    m_displayBufferFormat = swapChainFormat;
                    m_outputDeviceTransformType = AcesDisplayMapperFeatureProcessor::GetOutputDeviceTransformType(m_displayBufferFormat);
                    m_shaperParams = AZ::Render::GetAcesShaperParameters(m_outputDeviceTransformType);
                }

                blendPass->SetShaperParameters(m_shaperParams);
                    
                JE::Render::RPI::Ptr<LookModificationCompositePass> compositePass = FindChildPass<LookModificationCompositePass>();
                if (compositePass)
                {
                    compositePass->SetShaperParameters(m_shaperParams);
                }
            }

            ParentPass::FrameBeginInternal(params);
        }
    } // namespace Features
} // namespace JE::Render
