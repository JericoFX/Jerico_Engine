/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/ChromaticAberration/ChromaticAberrationComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace ChromaticAberration
        {
            inline constexpr AZ::TypeId EditorChromaticAberrationComponentTypeId{ "{2F6115F3-C6C4-47ED-8F06-11006F190A68}" };
        }

        class EditorChromaticAberrationComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<JE::Render::Scene::ChromaticAberrationComponentController, JE::Render::Scene::ChromaticAberrationComponent, JE::Render::Scene::ChromaticAberrationComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<JE::Render::Scene::ChromaticAberrationComponentController, JE::Render::Scene::ChromaticAberrationComponent, JE::Render::Scene::ChromaticAberrationComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorChromaticAberrationComponent, "AZ::Render::EditorChromaticAberrationComponent", JE::Editor::Render::Scene::ChromaticAberration::EditorChromaticAberrationComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorChromaticAberrationComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorChromaticAberrationComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorChromaticAberrationComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorChromaticAberrationComponent() = default;
            EditorChromaticAberrationComponent(const JE::Render::Scene::ChromaticAberrationComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
