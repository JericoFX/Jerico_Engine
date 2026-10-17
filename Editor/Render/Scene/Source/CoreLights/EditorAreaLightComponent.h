/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformDef.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <CoreLights/AreaLightComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorAreaLightComponent final
            : public AZ::Render::EditorRenderComponentAdapter<JE::Render::Scene::AreaLightComponentController, JE::Render::Scene::AreaLightComponent, JE::Render::Scene::AreaLightComponentConfig>
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
        {
        public:
            using BaseClass = AZ::Render::EditorRenderComponentAdapter<JE::Render::Scene::AreaLightComponentController, JE::Render::Scene::AreaLightComponent, JE::Render::Scene::AreaLightComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorAreaLightComponent, "AZ::Render::EditorAreaLightComponent", JE::Render::Scene::EditorAreaLightComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorAreaLightComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorAreaLightComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorAreaLightComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorAreaLightComponent() = default;
            EditorAreaLightComponent(const JE::Render::Scene::AreaLightComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            // BoundsRequestBus overrides ...
            AZ::Aabb GetWorldBounds() const override;
            AZ::Aabb GetLocalBounds() const override;

        private:
            // AzFramework::EntityDebugDisplayEventBus::Handler overrides...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            // AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler overrides (inherited from EditorRenderComponentAdapter)
            void OnEntityVisibilityChanged(bool visibility) override;

            // EditorRenderComponentAdapter overrides...
            bool ShouldActivateController() const override;

            bool HandleLightTypeChange();

            AZ::u32 OnConfigurationChanged() override;

            JE::Render::Scene::AreaLightComponentConfig::LightType m_lightType {JE::Render::Scene::AreaLightComponentConfig::LightType::Unknown}; // Used to detect when the configuration's light type changes.
        };

    } // namespace Render
} // namespace AZ
