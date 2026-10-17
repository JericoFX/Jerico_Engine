/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <SpecularReflections/SpecularReflectionsComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorSpecularReflectionsComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::SpecularReflectionsComponentController, JE::Render::Scene::SpecularReflectionsComponent, JE::Render::Scene::SpecularReflectionsComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::SpecularReflectionsComponentController, JE::Render::Scene::SpecularReflectionsComponent, JE::Render::Scene::SpecularReflectionsComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorSpecularReflectionsComponent, "AZ::Render::EditorSpecularReflectionsComponent", JE::Render::Scene::EditorSpecularReflectionsComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorSpecularReflectionsComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorSpecularReflectionsComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorSpecularReflectionsComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorSpecularReflectionsComponent() = default;
            EditorSpecularReflectionsComponent(const JE::Render::Scene::SpecularReflectionsComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;

        private:
            AZ::u32 GetSSRVisibilitySetting();
        };

    } // namespace Render
} // namespace AZ
