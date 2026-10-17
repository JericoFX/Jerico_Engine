/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <JE/Render/Scene/PostProcess/PostFxLayerComponentConstants.h>
#include <PostProcess/PostFxLayerComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorPostFxLayerComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::PostFxLayerComponentController, JE::Render::Scene::PostFxLayerComponent, JE::Render::Scene::PostFxLayerComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::PostFxLayerComponentController, JE::Render::Scene::PostFxLayerComponent, JE::Render::Scene::PostFxLayerComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorPostFxLayerComponent, "AZ::Render::EditorPostFxLayerComponent", JE::Render::Scene::EditorPostFxLayerComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorPostFxLayerComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorPostFxLayerComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorPostFxLayerComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorPostFxLayerComponent() = default;
            EditorPostFxLayerComponent(const JE::Render::Scene::PostFxLayerComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
