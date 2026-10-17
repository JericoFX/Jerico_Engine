/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/WhiteBalance/WhiteBalanceComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace WhiteBalance
        {
            static constexpr const char* const EditorWhiteBalanceComponentTypeId = "{32C7C44E-AABD-4575-A485-C820BF1F0873}";
        }

        class EditorWhiteBalanceComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<JE::Render::Scene::WhiteBalanceComponentController, JE::Render::Scene::WhiteBalanceComponent, JE::Render::Scene::WhiteBalanceComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<JE::Render::Scene::WhiteBalanceComponentController, JE::Render::Scene::WhiteBalanceComponent, JE::Render::Scene::WhiteBalanceComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorWhiteBalanceComponent, "AZ::Render::EditorWhiteBalanceComponent", JE::Editor::Render::Scene::WhiteBalance::EditorWhiteBalanceComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorWhiteBalanceComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorWhiteBalanceComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorWhiteBalanceComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorWhiteBalanceComponent() = default;
            EditorWhiteBalanceComponent(const JE::Render::Scene::WhiteBalanceComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
