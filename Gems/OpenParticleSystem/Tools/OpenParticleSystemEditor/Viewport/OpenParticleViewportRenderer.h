/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <OpenParticleSystem/ParticleEditorRequestBus.h>
#include <JE/Render/Features/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <JE/Render/Features/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <JE/Render/RPI/Public/AuxGeom/AuxGeomDraw.h>
#include <JE/Render/RPI/Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <JE/Render/RPI/Public/Base.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Instance/Instance.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <Document/ParticleDocumentBus.h>
#include <InputController/OpenParticleEditorViewportInputController.h>
#include <OpenParticleViewportNotificationBus.h>
#include <PerformanceMonitorComponent.h>
namespace JE::Render::Features
{
    class DisplayMapperFeatureProcessorInterface;
}

namespace AZ
{
    namespace Render
    {

    }

    class Entity;
    class Component;

    }

namespace JE::Render::RPI
{
        class SwapChainPass;
        class WindowContext;
    }

namespace AZ
{
}

namespace OpenParticleSystemEditor
{
    //! Provides backend logic for OpenParticleViewport
    //! Sets up a scene, camera, loads the model, and applies texture
    class OpenParticleViewportRenderer
        : public AZ::TickBus::Handler
        , public ParticleDocumentNotifyBus::Handler
        , public OpenParticleViewportNotificationBus::Handler
        , public AZ::TransformNotificationBus::MultiHandler
        , public OpenParticleSystem::ParticleEditorRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(OpenParticleViewportRenderer, AZ::SystemAllocator, 0);

        explicit OpenParticleViewportRenderer(AZStd::shared_ptr<JE::Render::RPI::WindowContext> windowContext);
        ~OpenParticleViewportRenderer();

        AZStd::shared_ptr<OpenParticleEditorViewportInputController> GetController();
        JE::Render::RPI::ScenePtr GetScene()
        {
            return m_scene;
        }

        void ActiveView();
        void DeactiveView();
    private:
        constexpr static AZStd::string_view SCENE_NAME = "OpenParticleViewportRenderer";
        constexpr static AZStd::string_view VIEWPOINT_SETTING_PATH = "/O3DE/Editor/Viewport/OpenParticle/Scene";

        void CreateRenderScene();
        void CreateAssetRenderPipeline();
        void CreateBRDFTexturePipeline() const;
        void CreateCamera(AzFramework::EntityContextId& entityContextId);
        void CreateParticleEntity(AzFramework::EntityContextId& entityContextId);
        void CreatePostProcessEntity(AzFramework::EntityContextId& entityContextId);
        void InitSkybox();
        void CreateGrid(AzFramework::EntityContextId& entityContextId);
        void ConnectEBus();

        const OpenParticleSystem::CameraTransform& GetParticleEditorCameraTransform() override;

        // OpenParticleSystemEditor::ParticleDocumentNotifyBus::Handler interface overrides...
        void OnDocumentOpened(AZ::Data::Asset<OpenParticle::ParticleAsset> particleAsset, [[maybe_unused]] AZStd::string particleAssetPath) override;
        void OnDocumentInvisible() override;

        // OpenParticleViewportNotificationBus::Handler interface overrides...
        void OnLightingPresetSelected(AZStd::string preset) override;
        void OnGridEnabledChanged(bool enable) override;
        void OnAlternateSkyboxEnabledChanged(bool enable) override;

        // AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AZ::TransformNotificationBus::MultiHandler overrides...
        void OnTransformChanged(const AZ::Transform&, const AZ::Transform&) override;

        void DrawAxisGizmo() const;

        void DrawPerformanceMonitorInfo();

        void DrawLine2d(JE::Render::RPI::AuxGeomDrawPtr auxGeomPtr, const AZ::Vector2& p1, const AZ::Vector2& p2, const AZ::Color& color, float z) const;
        void Draw2dTextLabel(float x, float y, float size, const char* text, bool center, const AZ::Color& color) const;

        AZStd::shared_ptr<JE::Render::RPI::WindowContext> m_windowContext;
        JE::Render::RPI::RenderPipelinePtr m_renderPipeline;
        JE::Render::RPI::ScenePtr m_scene;
        JE::Render::Features::DirectionalLightFeatureProcessorInterface* m_directionalLightFeatureProcessor = nullptr;

        AZStd::unique_ptr<AzFramework::EntityContext> m_entityContext;

        AZ::Entity* m_cameraEntity = nullptr;
        AZ::Component* m_cameraComponent = nullptr;

        AZ::Entity* m_postProcessEntity = nullptr;
        AZ::Entity* m_gridEntity = nullptr;

        using DirectionalLightHandle = JE::Render::Features::DirectionalLightFeatureProcessorInterface::LightHandle;
        AZStd::vector<DirectionalLightHandle> m_lightHandles;

        JE::Render::Features::SkyBoxFeatureProcessorInterface* m_skyboxFeatureProcessor = nullptr;

        AZStd::shared_ptr<OpenParticleEditorViewportInputController> m_viewportController;
        AZ::Entity* m_particleEntity = nullptr;

        PerformanceMonitorComponent m_performanceMonitor;
        bool m_initialized = false;
        OpenParticleSystem::CameraTransform m_cameraTransform;
        bool m_active = false;
    };
} // namespace OpenParticleSystemEditor
