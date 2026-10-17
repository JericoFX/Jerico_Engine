/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Instance/Instance.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/ViewportContext.h>
#include <JE/Render/RPI/Public/WindowContext.h>
#include <JE/Render/RPI/Public/Image/AttachmentImage.h>
#include <JE/Render/RPI/Public/Pass/Specific/SwapChainPass.h>

#include <JE/Render/Bootstrap/DefaultWindowBus.h>
#include <JE/Render/Bootstrap/BootstrapRequestBus.h>

namespace JE
{
    namespace Render
    {
        namespace Bootstrap
        {
            class BootstrapSystemComponent
                : public AZ::Component
                , public AZ::TickBus::Handler
                , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
                , public AzFramework::WindowNotificationBus::Handler
                , public AzFramework::WindowSystemNotificationBus::Handler
                , public AzFramework::WindowSystemRequestBus::Handler
                , public JE::Render::Bootstrap::DefaultWindowBus::Handler
                , public JE::Render::Bootstrap::RequestBus::Handler
            {
            public:
                AZ_COMPONENT(BootstrapSystemComponent, "{1EAFD87D-A64A-4612-93D8-B3AFFA70F09B}");

                BootstrapSystemComponent();
                ~BootstrapSystemComponent() override;

                static void Reflect(AZ::ReflectContext* context);

                static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
                static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
                static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
                static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& required);

                // AzFramework::WindowSystemRequestBus::Handler overrides ...
                AzFramework::NativeWindowHandle GetDefaultWindowHandle() override;

                // JE::Render::Bootstrap::DefaultWindowBus::Handler overrides ...
                AZStd::shared_ptr<JE::Render::RPI::WindowContext> GetDefaultWindowContext() override;
                void SetCreateDefaultScene(bool create) override;

                // JE::Render::Bootstrap::RequestBus::Handler overrides ...
                JE::Render::RPI::ScenePtr GetOrCreateAtomSceneFromAzScene(AzFramework::Scene* scene) override;
                bool EnsureDefaultRenderPipelineInstalledForScene(JE::Render::RPI::ScenePtr scene, JE::Render::RPI::ViewportContextPtr viewportContext) override;
                void SwitchRenderPipeline(const JE::Render::RPI::RenderPipelineDescriptor& newRenderPipelineDesc, JE::Render::RPI::ViewportContextPtr viewportContext) override;
                void SwitchAntiAliasing(const AZStd::string& newAntiAliasing, JE::Render::RPI::ViewportContextPtr viewportContext) override;
                void SwitchMultiSample(const uint16_t newSampleCount, JE::Render::RPI::ViewportContextPtr viewportContext) override;
                void RefreshWindowResolution() override;

            protected:
                // AZ::Component overrides ...
                void Activate() override;
                void Deactivate() override;

                // WindowNotificationBus::Handler overrides ...
                void OnWindowClosed() override;
                void OnWindowResized(uint32_t width, uint32_t height) override;

                // AZ::TickBus::Handler overrides ...
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
                int GetTickOrder() override;

                // AzFramework::WindowSystemNotificationBus::Handler overrides ...
                void OnWindowCreated(AzFramework::NativeWindowHandle windowHandle) override;

                // AzFramework::ApplicationLifecycleEvents::Bus::Handler overrides ...
                void OnApplicationWindowCreated() override;
                void OnApplicationWindowDestroy() override;

            private:
                void Initialize();

                void CreateDefaultRenderPipeline();
                void CreateDefaultScene();
                void DestroyDefaultScene();
                void RemoveRenderPipeline();

                void CreateViewportContext();
                void SetWindowResolution();

                //! Run the BRDF pipeline to generate the BRDF texture
                void RunBRDFPipeline(JE::Render::RPI::ScenePtr scene, JE::Render::RPI::ViewportContextPtr viewportContext);

                //! Load a render pipeline from disk and add it to the scene
                JE::Render::RPI::RenderPipelinePtr LoadPipeline(
                    const JE::Render::RPI::ScenePtr scene,
                    const JE::Render::RPI::ViewportContextPtr viewportContext,
                    AZStd::string_view xrPipelineName,
                    JE::Render::RPI::ViewType viewType,
                    AZ::RHI::MultisampleState& multisampleState);

                AzFramework::Scene::RemovalEvent::Handler m_sceneRemovalHandler;

                AZStd::unique_ptr<AzFramework::NativeWindow> m_nativeWindow;
                AzFramework::NativeWindowHandle m_windowHandle = nullptr;
                JE::Render::RPI::ViewportContextPtr m_viewportContext;

                JE::Render::RPI::ScenePtr m_defaultScene = nullptr;
                AZStd::shared_ptr<AzFramework::Scene> m_defaultFrameworkScene = nullptr;

                bool m_isInitialized = false;

                // The id of the render pipeline created by this component
                JE::Render::RPI::RenderPipelineId m_renderPipelineId;

                // Save a reference to the image created by the BRDF pipeline so it doesn't get auto deleted if it's ref count goes to zero
                // For example, if we delete all the passes, we won't have to recreate the BRDF pipeline to recreate the BRDF texture
                AZ::Data::Instance<JE::Render::RPI::AttachmentImage> m_brdfTexture;

                bool m_createDefaultScene = true;
                bool m_defaultSceneReady = false;

                // Maps AZ scenes to RPI scene weak pointers to allow looking up a ScenePtr instead of a raw Scene*
                AZStd::unordered_map<AzFramework::Scene*, AZStd::weak_ptr<JE::Render::RPI::Scene>> m_azSceneToAtomSceneMap;

                AZ::SettingsRegistryInterface::NotifyEventHandler m_componentApplicationLifecycleHandler;
            };
        } // namespace Bootstrap
    } // namespace Render
} // namespace JE
