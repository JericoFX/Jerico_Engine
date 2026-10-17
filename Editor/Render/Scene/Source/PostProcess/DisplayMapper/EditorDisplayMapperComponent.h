/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/DisplayMapper/DisplayMapperComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorDisplayMapperComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::DisplayMapperComponentController, JE::Render::Scene::DisplayMapperComponent, JE::Render::Scene::DisplayMapperComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::DisplayMapperComponentController, JE::Render::Scene::DisplayMapperComponent, JE::Render::Scene::DisplayMapperComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorDisplayMapperComponent, "AZ::Render::EditorDisplayMapperComponent", JE::Render::Scene::EditorDisplayMapperComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorDisplayMapperComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorDisplayMapperComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorDisplayMapperComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorDisplayMapperComponent() = default;
            EditorDisplayMapperComponent(const JE::Render::Scene::DisplayMapperComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
