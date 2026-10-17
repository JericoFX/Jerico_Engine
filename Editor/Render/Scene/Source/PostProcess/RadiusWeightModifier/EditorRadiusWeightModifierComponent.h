/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/RadiusWeightModifier/RadiusWeightModifierComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorRadiusWeightModifierComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::RadiusWeightModifierComponentController, JE::Render::Scene::RadiusWeightModifierComponent, JE::Render::Scene::RadiusWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::RadiusWeightModifierComponentController, JE::Render::Scene::RadiusWeightModifierComponent, JE::Render::Scene::RadiusWeightModifierComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorRadiusWeightModifierComponent, "AZ::Render::EditorRadiusWeightModifierComponent", JE::Render::Scene::EditorRadiusWeightModifierComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorRadiusWeightModifierComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorRadiusWeightModifierComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorRadiusWeightModifierComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorRadiusWeightModifierComponent() = default;
            EditorRadiusWeightModifierComponent(const JE::Render::Scene::RadiusWeightModifierComponentConfig& config);

            AZ::u32 OnConfigurationChanged() override;
        };
    }
}
