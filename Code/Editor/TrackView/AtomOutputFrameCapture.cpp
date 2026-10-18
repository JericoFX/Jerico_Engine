/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomOutputFrameCapture.h"

#include <JE/Render/RPI/Public/Pass/Specific/RenderToTexturePass.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPISystemInterface.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Reflect/System/RenderPipelineDescriptor.h>
#include <JE/Render/Features/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Name/Name.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

namespace TrackView
{
    void AtomOutputFrameCapture::CreatePipeline(
        JE::Render::RPI::Scene& scene, const AZStd::string& pipelineName, const uint32_t width, const uint32_t height)
    {
        JE::Render::RPI::RenderPipelineDescriptor pipelineDesc;
        pipelineDesc.m_mainViewTagName = "MainCamera"; // must be "MainCamera"
        pipelineDesc.m_name = pipelineName;
        pipelineDesc.m_rootPassTemplate = "MainPipelineRenderToTexture";
        pipelineDesc.m_renderSettings.m_multisampleState = JE::Render::RPI::RPISystemInterface::Get()->GetApplicationMultisampleState();
        m_renderPipeline = JE::Render::RPI::RenderPipeline::CreateRenderPipeline(pipelineDesc);

        if (auto renderToTexturePass = azrtti_cast<JE::Render::RPI::RenderToTexturePass*>(m_renderPipeline->GetRootPass().get()))
        {
            renderToTexturePass->ResizeOutput(width, height);
        }

        scene.AddRenderPipeline(m_renderPipeline);

        // rendering pipeline has a tree structure
        m_passHierarchy.push_back(pipelineName);
        m_passHierarchy.push_back("CopyToSwapChain");

        // retrieve View from the camera that's animating
        AZ::Name viewName = AZ::Name("MainCamera");
        m_view = JE::Render::RPI::View::CreateView(viewName, JE::Render::RPI::View::UsageCamera);
        m_renderPipeline->SetDefaultView(m_view);
        m_targetView = scene.GetDefaultRenderPipeline()->GetDefaultView();
        if (auto* fp = scene.GetFeatureProcessor<JE::Render::Features::PostProcessFeatureProcessorInterface>())
        {
            // This will be set again to mimic the active camera in UpdateView
            fp->SetViewAlias(m_view, m_targetView);
        }
        m_pipelineCreated = true;
    }

    void AtomOutputFrameCapture::DestroyPipeline(JE::Render::RPI::Scene& scene)
    {
        if (!m_pipelineCreated)
        {
            return;
        }

        if (auto* fp = scene.GetFeatureProcessor<JE::Render::Features::PostProcessFeatureProcessorInterface>())
        {
            // Remove view alias introduced in CreatePipeline and UpdateView
            fp->RemoveViewAlias(m_view);
        }
        scene.RemoveRenderPipeline(m_renderPipeline->GetId());
        m_passHierarchy.clear();
        m_renderPipeline.reset();
        m_view.reset();
        m_targetView.reset();

        m_pipelineCreated = false;
    }

    void AtomOutputFrameCapture::UpdateView(const AZ::Matrix3x4& cameraTransform, const AZ::Matrix4x4& cameraProjection, const JE::Render::RPI::ViewPtr targetView)
    {
        if (targetView && targetView != m_targetView)
        {
            if (JE::Render::RPI::Scene* scene = SceneFromGameEntityContext())
            {
                if (auto* fp = scene->GetFeatureProcessor<JE::Render::Features::PostProcessFeatureProcessorInterface>())
                {
                    fp->SetViewAlias(m_view, targetView);
                    m_targetView = targetView;
                }
            }
        }

        m_view->SetCameraTransform(cameraTransform);
        m_view->SetViewToClipMatrix(cameraProjection);
    }

    bool AtomOutputFrameCapture::BeginCapture(
        const JE::Render::RPI::AttachmentReadback::CallbackFunction& attachmentReadbackCallback, CaptureFinishedCallback captureFinishedCallback)
    {
        m_captureFinishedCallback = AZStd::move(captureFinishedCallback);

        // note: "Output" (slot name) maps to MainPipeline.pass CopyToSwapChain
        JE::Render::Features::FrameCaptureOutcome captureOutcome;
        JE::Render::Features::FrameCaptureRequestBus::BroadcastResult(
            captureOutcome,
            &JE::Render::Features::FrameCaptureRequestBus::Events::CapturePassAttachmentWithCallback,
            attachmentReadbackCallback,
            m_passHierarchy,
            AZStd::string("Output"),
            JE::Render::RPI::PassAttachmentReadbackOption::Output);

        if (captureOutcome.IsSuccess())
        {
            JE::Render::Features::FrameCaptureNotificationBus::Handler::BusConnect(captureOutcome.GetValue());
            return true;
        }

        AZ_Error("AtomOutputFrameCapture", captureOutcome.IsSuccess(),
            "Frame capture initialization failed. %s", captureOutcome.GetError().m_errorMessage.c_str());

        return false;
    }

    void AtomOutputFrameCapture::OnFrameCaptureFinished(
        [[maybe_unused]] JE::Render::Features::FrameCaptureResult result, [[maybe_unused]] const AZStd::string& info)
    {
        JE::Render::Features::FrameCaptureNotificationBus::Handler::BusDisconnect();
        m_captureFinishedCallback();
    }

    AZ::Matrix3x4 TransformFromEntityId(const AZ::EntityId entityId)
    {
        AZ::Transform cameraTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(cameraTransform, entityId, &AZ::TransformBus::Events::GetWorldTM);
        return AZ::Matrix3x4::CreateFromTransform(cameraTransform);
    }

    AZ::Matrix4x4 ProjectionFromCameraEntityId(const AZ::EntityId entityId, const float outputWidth, const float outputHeight)
    {
        float nearDist = 0.0f;
        Camera::CameraRequestBus::EventResult(nearDist, entityId, &Camera::CameraRequestBus::Events::GetNearClipDistance);
        float farDist = 0.0f;
        Camera::CameraRequestBus::EventResult(farDist, entityId, &Camera::CameraRequestBus::Events::GetFarClipDistance);
        float fovRad = 0.0f;
        Camera::CameraRequestBus::EventResult(fovRad, entityId, &Camera::CameraRequestBus::Events::GetFovRadians);

        const float aspectRatio = outputWidth / outputHeight;

        AZ::Matrix4x4 viewToClipMatrix;
        AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix, fovRad, aspectRatio, nearDist, farDist, /*reverseDepth=*/true);
        return viewToClipMatrix;
    }

    JE::Render::RPI::Scene* SceneFromGameEntityContext()
    {
        AzFramework::EntityContextId entityContextId;
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            entityContextId, &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);

        return JE::Render::RPI::Scene::GetSceneForEntityContextId(entityContextId);
    }
} // namespace TrackView
