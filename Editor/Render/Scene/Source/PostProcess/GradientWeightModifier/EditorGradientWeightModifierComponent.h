/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/GradientWeightModifier/GradientWeightModifierComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorGradientWeightModifierComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::GradientWeightModifierComponentController, JE::Render::Scene::GradientWeightModifierComponent, JE::Render::Scene::GradientWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::GradientWeightModifierComponentController, JE::Render::Scene::GradientWeightModifierComponent, JE::Render::Scene::GradientWeightModifierComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorGradientWeightModifierComponent, "AZ::Render::EditorGradientWeightModifierComponent", JE::Render::Scene::EditorGradientWeightModifierComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorGradientWeightModifierComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorGradientWeightModifierComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorGradientWeightModifierComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorGradientWeightModifierComponent() = default;
            EditorGradientWeightModifierComponent(const JE::Render::Scene::GradientWeightModifierComponentConfig& config);

            AZ::u32 OnConfigurationChanged() override;
        };
    }
}
