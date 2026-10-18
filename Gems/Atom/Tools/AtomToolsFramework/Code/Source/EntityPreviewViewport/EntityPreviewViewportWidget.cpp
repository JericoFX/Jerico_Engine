/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <JE/Render/Features/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>
#include <JE/Render/Features/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <JE/Render/Features/PostProcessing/PostProcessingConstants.h>
#include <JE/Render/Features/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <JE/Render/Features/Utils/ModelPreset.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportWidget.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AtomToolsFramework
{
    EntityPreviewViewportWidget::EntityPreviewViewportWidget(const AZ::Crc32& toolId, QWidget* parent)
        : RenderViewportWidget(parent)
        , m_toolId(toolId)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        AZ::TickBus::Handler::BusConnect();
        EntityPreviewViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
    }

    EntityPreviewViewportWidget::~EntityPreviewViewportWidget()
    {
        EntityPreviewViewportSettingsNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        GetControllerList()->Remove(m_viewportController);

        m_viewportController.reset();
        m_viewportContent.reset();
        m_viewportScene.reset();
        m_entityContext.reset();
    }

    void EntityPreviewViewportWidget::Init(
        AZStd::shared_ptr<AzFramework::EntityContext> entityContext,
        AZStd::shared_ptr<EntityPreviewViewportScene> viewportScene,
        AZStd::shared_ptr<EntityPreviewViewportContent> viewportContent,
        AZStd::shared_ptr<EntityPreviewViewportInputController> viewportController)
    {
        m_entityContext = entityContext;
        m_viewportScene = viewportScene;
        m_viewportContent = viewportContent;
        m_viewportController = viewportController;

        GetControllerList()->Add(m_viewportController);

        // Connect camera to pipeline's default view after camera entity activated
        m_viewportScene->GetPipeline()->SetDefaultViewFromEntity(m_viewportContent->GetCameraEntityId());

        OnViewportSettingsChanged();
    }

    void EntityPreviewViewportWidget::OnViewportSettingsChanged()
    {
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequestBus::Events* viewportRequests)
            {
                m_viewportController->SetFieldOfView(viewportRequests->GetFieldOfView());

                // Update lighting preset, skybox, and shadows
                auto scene = m_viewportScene->GetScene();
                auto imageBasedLightFP = scene->GetFeatureProcessor<JE::Render::Features::ImageBasedLightFeatureProcessorInterface>();
                auto postProcessFP = scene->GetFeatureProcessor<JE::Render::Features::PostProcessFeatureProcessorInterface>();
                auto postProcessSettingInterface = postProcessFP->GetOrCreateSettingsInterface(m_viewportContent->GetPostFxEntityId());
                auto exposureControlSettingInterface = postProcessSettingInterface->GetOrCreateExposureControlSettingsInterface();
                auto directionalLightFP = scene->GetFeatureProcessor<JE::Render::Features::DirectionalLightFeatureProcessorInterface>();
                auto skyboxFP = scene->GetFeatureProcessor<JE::Render::Features::SkyBoxFeatureProcessorInterface>();

                Camera::Configuration cameraConfig;
                Camera::CameraRequestBus::EventResult(
                    cameraConfig, m_viewportContent->GetCameraEntityId(), &Camera::CameraRequestBus::Events::GetCameraConfiguration);

                const bool enableAlternateSkybox = viewportRequests->GetAlternateSkyboxEnabled();
                viewportRequests->GetLightingPreset().ApplyLightingPreset(
                    imageBasedLightFP, skyboxFP, exposureControlSettingInterface, directionalLightFP, cameraConfig, m_lightHandles,
                    enableAlternateSkybox);

                const bool pipelineActivated = m_viewportScene->ActivateRenderPipeline(viewportRequests->GetLastRenderPipelineAssetId());
                if (!pipelineActivated)
                {
                    // Since it failed to switch to the requested pipeline, tell the settings system to switch
                    // back to the previous active pipeline. This should trigger UI updates to correctly show
                    // which pipeline is active.
                    viewportRequests->LoadRenderPipelineByAssetId(m_viewportScene->GetPipelineAssetId());
                }
            });
    }

    void EntityPreviewViewportWidget::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        RenderViewportWidget::OnTick(deltaTime, time);

        const auto objectLocalBounds = m_viewportContent->GetObjectLocalBounds();
        if (m_objectLocalBoundsOld != objectLocalBounds)
        {
            m_objectLocalBoundsOld = objectLocalBounds;
            m_viewportController->Reset();
        }

        auto cameraTransform = m_cameraTransformOld;
        AZ::TransformBus::EventResult(cameraTransform, m_viewportContent->GetCameraEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        if (m_cameraTransformOld != cameraTransform)
        {
            m_cameraTransformOld = cameraTransform;
            auto scene = m_viewportScene->GetScene();
            auto directionalLightFP = scene->GetFeatureProcessor<JE::Render::Features::DirectionalLightFeatureProcessorInterface>();
            for (const DirectionalLightHandle& id : m_lightHandles)
            {
                directionalLightFP->SetCameraTransform(id, cameraTransform);
            }
        }

        m_viewportScene->GetPipeline()->AddToRenderTickOnce();
    }
} // namespace AtomToolsFramework
