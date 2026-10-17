/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/AoConstants.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoConstants.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoConstants.h>
#include <PostProcess/AmbientOcclusion/AoComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace Ao
        {
            inline constexpr AZ::TypeId EditorAoComponentTypeId{ "{5A807489-4FB2-4421-A4D2-9D9E523ABF83}" };
        }

        class EditorAoComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::AoComponentController, JE::Render::Scene::AoComponent, JE::Render::Scene::AoComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::AoComponentController, JE::Render::Scene::AoComponent, JE::Render::Scene::AoComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorAoComponent, "AZ::Render::EditorAoComponent", JE::Editor::Render::Scene::Ao::EditorAoComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorAoComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorAoComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorAoComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorAoComponent() = default;
            EditorAoComponent(const JE::Render::Scene::AoComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
