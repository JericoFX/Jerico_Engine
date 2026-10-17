/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/PaniniProjection/PaniniProjectionComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace PaniniProjection
        {
            static constexpr const char* const EditorPaniniProjectionComponentTypeId = "{213CFFC8-E9E6-46EA-9DBE-B779F0B2A823}";
        }

        class EditorPaniniProjectionComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<JE::Render::Scene::PaniniProjectionComponentController, JE::Render::Scene::PaniniProjectionComponent, JE::Render::Scene::PaniniProjectionComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<JE::Render::Scene::PaniniProjectionComponentController, JE::Render::Scene::PaniniProjectionComponent, JE::Render::Scene::PaniniProjectionComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorPaniniProjectionComponent, "AZ::Render::EditorPaniniProjectionComponent", JE::Editor::Render::Scene::PaniniProjection::EditorPaniniProjectionComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorPaniniProjectionComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorPaniniProjectionComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorPaniniProjectionComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorPaniniProjectionComponent() = default;
            EditorPaniniProjectionComponent(const JE::Render::Scene::PaniniProjectionComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
