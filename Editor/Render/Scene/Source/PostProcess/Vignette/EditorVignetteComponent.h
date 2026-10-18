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
#include <PostProcess/Vignette/VignetteComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace Vignette
        {
            static constexpr const char* const EditorVignetteComponentTypeId = "{8E362EA9-76D8-4EBC-B73D-400DF3DF8B4A}";
        }

        class EditorVignetteComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<JE::Render::Scene::VignetteComponentController, JE::Render::Scene::VignetteComponent, JE::Render::Scene::VignetteComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<JE::Render::Scene::VignetteComponentController, JE::Render::Scene::VignetteComponent, JE::Render::Scene::VignetteComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorVignetteComponent, "AZ::Render::EditorVignetteComponent", JE::Editor::Render::Scene::Vignette::EditorVignetteComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorVignetteComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorVignetteComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorVignetteComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorVignetteComponent() = default;
            EditorVignetteComponent(const JE::Render::Scene::VignetteComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
