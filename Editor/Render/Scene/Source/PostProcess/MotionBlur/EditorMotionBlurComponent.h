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
#include <PostProcess/MotionBlur/MotionBlurComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace MotionBlur
        {
            static constexpr const char* const EditorMotionBlurComponentTypeId = "{ACA14BD9-BB53-4FEB-9E07-FDC0C9FE64FE}";
        }

        class EditorMotionBlurComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<JE::Render::Scene::MotionBlurComponentController, JE::Render::Scene::MotionBlurComponent, JE::Render::Scene::MotionBlurComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<JE::Render::Scene::MotionBlurComponentController, JE::Render::Scene::MotionBlurComponent, JE::Render::Scene::MotionBlurComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorMotionBlurComponent, "AZ::Render::EditorMotionBlurComponent", JE::Editor::Render::Scene::MotionBlur::EditorMotionBlurComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorMotionBlurComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorMotionBlurComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorMotionBlurComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorMotionBlurComponent() = default;
            EditorMotionBlurComponent(const JE::Render::Scene::MotionBlurComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
