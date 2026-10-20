/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PreviewRenderer/PreviewRenderer.h>
#include <PreviewRenderer/PreviewRendererCaptureState.h>

namespace JE::Editor::Tools
{
    PreviewRendererCaptureState::PreviewRendererCaptureState(PreviewRenderer* renderer)
        : PreviewRendererState(renderer)
    {
        m_renderer->PoseContent();
    }

    PreviewRendererCaptureState::~PreviewRendererCaptureState()
    {
        JE::Render::Features::FrameCaptureNotificationBus::Handler::BusDisconnect();
        m_renderer->EndCapture();
    }

    void PreviewRendererCaptureState::Update()
    {
        if (m_captureComplete)
        {
            JE::Render::Features::FrameCaptureNotificationBus::Handler::BusDisconnect();
            m_renderer->CompleteCaptureRequest();
            return;
        }

        if (AZStd::chrono::steady_clock::now() > m_abortTime)
        {
            JE::Render::Features::FrameCaptureNotificationBus::Handler::BusDisconnect();
            m_renderer->CancelCaptureRequest();
            return;
        }

        if (m_renderer->IsContentReadyToRender())
        {
            if (!JE::Render::Features::FrameCaptureNotificationBus::Handler::BusIsConnected())
            {
                // if the start capture call fails the capture will be retried next tick.
                const JE::Render::Features::FrameCaptureId frameCaptureId = m_renderer->StartCapture();
                if (frameCaptureId != JE::Render::Features::InvalidFrameCaptureId)
                {
                    JE::Render::Features::FrameCaptureNotificationBus::Handler::BusConnect(frameCaptureId);
                }
            }
        }
    }

    void PreviewRendererCaptureState::OnFrameCaptureFinished(
        [[maybe_unused]] JE::Render::Features::FrameCaptureResult result, [[maybe_unused]] const AZStd::string& info)
    {
        JE::Render::Features::FrameCaptureNotificationBus::Handler::BusDisconnect();
        m_captureComplete = true;
    }
} // namespace JE::Editor::Tools
