/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <JE/Render/Features/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <PostProcess/DepthOfField/DepthOfFieldComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace DepthOfField
        {
            inline constexpr AZ::TypeId EditorDepthOfFieldComponentTypeId{ "{E9B85017-18F3-4CD6-9EEC-221B0E6B0619}" };
        }

        class EditorDepthOfFieldComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::DepthOfFieldComponentController, JE::Render::Scene::DepthOfFieldComponent, JE::Render::Scene::DepthOfFieldComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::DepthOfFieldComponentController, JE::Render::Scene::DepthOfFieldComponent, JE::Render::Scene::DepthOfFieldComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorDepthOfFieldComponent, "AZ::Render::EditorDepthOfFieldComponent", JE::Editor::Render::Scene::DepthOfField::EditorDepthOfFieldComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorDepthOfFieldComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorDepthOfFieldComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorDepthOfFieldComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorDepthOfFieldComponent() = default;
            EditorDepthOfFieldComponent(const JE::Render::Scene::DepthOfFieldComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
