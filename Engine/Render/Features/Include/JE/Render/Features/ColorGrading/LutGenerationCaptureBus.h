/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <JE/Render/Features/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h>
#include <JE/Render/Features/Utils/FrameCaptureBus.h>
#include <AzCore/std/parallel/mutex.h>

namespace JE::Render::Features
{
    // Immutable encoding used by LutGenerationPass for the frame reserved by this capture.
    struct LutGenerationCaptureMetadata
    {
        JE::Render::Features::FrameCaptureId m_captureId = JE::Render::Features::InvalidFrameCaptureId;
        bool m_generateLut = false;
        JE::Render::Features::LutResolution m_resolution = JE::Render::Features::LutResolution::Lut16x16x16;
        AZ::Render::ShaperPresetType m_shaper = AZ::Render::ShaperPresetType::None;
        float m_minimumExposure = 0.0f;
        float m_maximumExposure = 0.0f;

        static LutGenerationCaptureMetadata FromSettings(
            const JE::Render::Features::HDRColorGradingSettingsInterface& settings, JE::Render::Features::FrameCaptureId captureId)
        {
            return {captureId, settings.GetGenerateLut(), settings.GetLutResolution(), settings.GetShaperPresetType(),
                settings.GetCustomMinExposure(), settings.GetCustomMaxExposure()};
        }

        bool MatchesReadback(JE::Render::Features::FrameCaptureId expectedId,
            const JE::Render::RPI::AttachmentReadback::ReadbackResult& result) const
        {
            const auto resolution = static_cast<uint32_t>(m_resolution);
            return result.m_state == JE::Render::RPI::AttachmentReadback::ReadbackState::Success &&
                expectedId != JE::Render::Features::InvalidFrameCaptureId && m_captureId == expectedId &&
                result.m_userIdentifier == expectedId && m_generateLut &&
                (resolution == 16 || resolution == 32 || resolution == 64) &&
                result.m_attachmentType == AZ::RHI::AttachmentType::Image &&
                result.m_imageDescriptor.m_format == AZ::RHI::Format::R32G32B32A32_FLOAT &&
                result.m_imageDescriptor.m_size.m_width == resolution * resolution &&
                result.m_imageDescriptor.m_size.m_height == resolution && result.m_imageDescriptor.m_size.m_depth == 1;
        }
    };

    class LutGenerationCaptureNotifications : public AZ::EBusTraits
    {
    public:
        using BusIdType = JE::Render::Features::FrameCaptureId;
        using MutexType = AZStd::recursive_mutex;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        virtual ~LutGenerationCaptureNotifications() = default;
        virtual void OnLutGenerationCapture(const LutGenerationCaptureMetadata& metadata) = 0;
    };
    using LutGenerationCaptureNotificationBus = AZ::EBus<LutGenerationCaptureNotifications>;
}
