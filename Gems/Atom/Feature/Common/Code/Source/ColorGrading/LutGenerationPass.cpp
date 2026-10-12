/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ColorGrading/LutGenerationPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {

        RPI::Ptr<LutGenerationPass> LutGenerationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LutGenerationPass> pass = aznew LutGenerationPass(descriptor);
            return pass;
        }

         LutGenerationPass::LutGenerationPass(const RPI::PassDescriptor& descriptor)
            : HDRColorGradingPass(descriptor)
        {
        }

        void LutGenerationPass::InitializeInternal()
        {
            HDRColorGradingPass::InitializeInternal();

            m_lutResolutionIndex.Reset();
            m_lutShaperTypeIndex.Reset();
            m_lutShaperScaleIndex.Reset();
        }

        void LutGenerationPass::FrameBeginInternal(FramePrepareParams params)
        {
            const auto* colorGradingSettings = GetHDRColorGradingSettings();
            m_frameEncodingValid = colorGradingSettings != nullptr;
            if (m_frameEncodingValid)
            {
                m_frameEncoding = JE::ColorGrading::LutGenerationCaptureMetadata::FromSettings(
                    *colorGradingSettings, InvalidFrameCaptureId);
                m_shaderResourceGroup->SetConstant(m_lutResolutionIndex, m_frameEncoding.m_resolution);

                auto shaperParams = AcesDisplayMapperFeatureProcessor::GetShaperParameters(
                    m_frameEncoding.m_shaper,
                    m_frameEncoding.m_minimumExposure,
                    m_frameEncoding.m_maximumExposure);
                m_shaderResourceGroup->SetConstant(m_lutShaperTypeIndex, shaperParams.m_type);
                m_shaderResourceGroup->SetConstant(m_lutShaperBiasIndex, shaperParams.m_bias);
                m_shaderResourceGroup->SetConstant(m_lutShaperScaleIndex, shaperParams.m_scale);
            }

            HDRColorGradingPass::FrameBeginInternal(params);

            // Pass::FrameBegin submits this output reservation immediately after returning.
            if (m_frameEncodingValid && m_attachmentReadback &&
                m_readbackOption == RPI::PassAttachmentReadbackOption::Output &&
                m_attachmentReadback->GetReadbackState() == RPI::AttachmentReadback::ReadbackState::AttachmentSet)
            {
                const auto captureId = m_attachmentReadback->GetUserIdentifier();
                if (captureId != InvalidFrameCaptureId)
                {
                    m_frameEncoding.m_captureId = captureId;
                    JE::ColorGrading::LutGenerationCaptureNotificationBus::Event(m_frameEncoding.m_captureId,
                        &JE::ColorGrading::LutGenerationCaptureNotifications::OnLutGenerationCapture, m_frameEncoding);
                }
            }
        }

        void LutGenerationPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            if (m_frameEncodingValid)
            {
                const uint32_t lutResolution = aznumeric_cast<uint32_t>(m_frameEncoding.m_resolution);
                RHI::Size lutSize{ lutResolution * lutResolution, lutResolution, 1 };

                RPI::Ptr<RPI::PassAttachment> attachment = FindOwnedAttachment(Name{ "ColorGradingLut" });
                RHI::ImageDescriptor& imageDescriptor = attachment->m_descriptor.m_image;
                imageDescriptor.m_size = lutSize;
                SetViewportScissorFromImageSize(lutSize);
            }
            HDRColorGradingPass::BuildCommandListInternal(context);
        }

        bool LutGenerationPass::IsEnabled() const
        {
            const auto* colorGradingSettings = GetHDRColorGradingSettings();
            return colorGradingSettings ? colorGradingSettings->GetGenerateLut() : false;
        }

        void LutGenerationPass::SetViewportScissorFromImageSize(const RHI::Size& imageSize)
        {
            const RHI::Viewport viewport(0.f, imageSize.m_width * 1.f, 0.f, imageSize.m_height * 1.f);
            const RHI::Scissor scissor(0, 0, imageSize.m_width, imageSize.m_height);
            m_viewportState = viewport;
            m_scissorState = scissor;
        }

    } // namespace Render
} // namespace AZ
