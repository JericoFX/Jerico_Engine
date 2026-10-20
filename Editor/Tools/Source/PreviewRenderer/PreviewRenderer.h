/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Utils/FrameCaptureBus.h>
#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Pass/AttachmentReadback.h>
#include <JE/Editor/Tools/PreviewRenderer/PreviewContent.h>
#include <JE/Editor/Tools/PreviewRenderer/PreviewRendererCaptureRequest.h>
#include <JE/Editor/Tools/PreviewRenderer/PreviewRendererInterface.h>
#include <JE/Editor/Tools/PreviewRenderer/PreviewerFeatureProcessorProviderBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <PreviewRenderer/PreviewRendererState.h>

namespace JE::Editor::Tools
{
    //! Processes requests for setting up content that gets rendered to a texture and captured to an image
    class PreviewRenderer final
        : public PreviewRendererInterface
        , public PreviewerFeatureProcessorProviderBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PreviewRenderer, AZ::SystemAllocator);
        AZ_RTTI(PreviewRenderer, "{60FCB7AB-2A94-417A-8C5E-5B588D17F5D1}", PreviewRendererInterface);

        PreviewRenderer(const AZStd::string& sceneName, const AZStd::string& pipelineName);
        ~PreviewRenderer() override;

        void AddCaptureRequest(const PreviewRendererCaptureRequest& captureRequest) override;

        JE::Render::RPI::ScenePtr GetScene() const override;
        JE::Render::RPI::ViewPtr GetView() const override;
        AZ::Uuid GetEntityContextId() const override;

        void ProcessCaptureRequests();
        void CancelCaptureRequest();
        void CompleteCaptureRequest();

        void LoadContent();
        void UpdateLoadContent();
        void CancelLoadContent();

        void PoseContent();

        bool IsContentReadyToRender();
        JE::Render::Features::FrameCaptureId StartCapture();
        void EndCapture();

    private:
        //! AZ::SystemTickBus::Handler interface overrides...
        void OnSystemTick() override;

        //! AZ::Render::PreviewerFeatureProcessorProviderBus::Handler interface overrides...
        void GetRequiredFeatureProcessors(AZStd::vector<AZStd::string>& featureProcessors) const override;

        static constexpr float AspectRatio = 1.0f;
        static constexpr float NearDist = 0.001f;
        static constexpr float FarDist = 100.0f;
        static constexpr float FieldOfView = AZ::Constants::HalfPi;

        JE::Render::RPI::ScenePtr m_scene;
        AZStd::shared_ptr<AzFramework::Scene> m_frameworkScene;
        JE::Render::RPI::RenderPipelinePtr m_renderPipeline;
        JE::Render::RPI::ViewPtr m_view;
        AZStd::vector<AZStd::string> m_passHierarchy;
        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;

        //! Incoming requests are appended to this queue and processed one at a time in OnSystemTick function.
        AZStd::queue<PreviewRendererCaptureRequest> m_captureRequestQueue;
        PreviewRendererCaptureRequest m_currentCaptureRequest;

        AZStd::unique_ptr<PreviewRendererState> m_state;
    };
} // namespace JE::Editor::Tools
