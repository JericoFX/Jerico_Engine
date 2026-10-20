/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Utils/FrameCaptureBus.h>
#include <PreviewRenderer/PreviewRendererState.h>

namespace JE::Editor::Tools
{
    //! PreviewRendererCaptureState renders a preview to an image
    class PreviewRendererCaptureState final
        : public PreviewRendererState
        , public JE::Render::Features::FrameCaptureNotificationBus::Handler
    {
    public:
        PreviewRendererCaptureState(PreviewRenderer* renderer);
        ~PreviewRendererCaptureState();
        void Update() override;

    private:
        //! AZ::Render::FrameCaptureNotificationBus::Handler overrides...
        void OnFrameCaptureFinished(JE::Render::Features::FrameCaptureResult result, const AZStd::string& info) override;

        //! Track the amount of time since the capture request was initiated
        AZStd::chrono::steady_clock::time_point m_startTime = AZStd::chrono::steady_clock::now();
        AZStd::chrono::steady_clock::time_point m_abortTime = AZStd::chrono::steady_clock::now() + AZStd::chrono::milliseconds(5000);
        bool m_captureComplete = false;
    };
} // namespace JE::Editor::Tools
