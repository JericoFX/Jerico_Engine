/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CoreLights/DirectionalLightComponent.h>
#include <Atom/Feature/CoreLights/DirectionalLightShadowNotificationBus.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <JE/Render/Scene/CoreLights/DirectionalLightComponentConfig.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorDirectionalLightComponent final
            : public AZ::Render::EditorRenderComponentAdapter<JE::Render::Scene::DirectionalLightComponentController, JE::Render::Scene::DirectionalLightComponent, JE::Render::Scene::DirectionalLightComponentConfig>
            , public AZ::Render::ShadowingDirectionalLightNotificationsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:

            using BaseClass = AZ::Render::EditorRenderComponentAdapter<JE::Render::Scene::DirectionalLightComponentController, JE::Render::Scene::DirectionalLightComponent, JE::Render::Scene::DirectionalLightComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorDirectionalLightComponent, "AZ::Render::EditorDirectionalLightComponent", JE::Render::Scene::EditorDirectionalLightComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorDirectionalLightComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorDirectionalLightComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorDirectionalLightComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorDirectionalLightComponent() = default;
            EditorDirectionalLightComponent(const JE::Render::Scene::DirectionalLightComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;

            // ShadowingDirectionalLightNotificationsBus overrides
            void OnShadowingDirectionalLightChanged(const AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle& handle) override;

            //! AzFramework::EntityDebugDisplayEventBus::Handler overrides...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;
        };

    } // namespace Render
} // namespace AZ
