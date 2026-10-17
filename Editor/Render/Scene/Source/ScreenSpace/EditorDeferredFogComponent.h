/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <ScreenSpace/DeferredFogComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace DeferredFog
        {
            static constexpr const char* const EditorDeferredFogComponentTypeId =
                "{6459274F-54C8-4C22-9448-B2B13B69182C}";
        }

        class EditorDeferredFogComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::DeferredFogComponentController, JE::Render::Scene::DeferredFogComponent, JE::Render::Scene::DeferredFogComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::DeferredFogComponentController, JE::Render::Scene::DeferredFogComponent, JE::Render::Scene::DeferredFogComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorDeferredFogComponent, "AZ::Render::EditorDeferredFogComponent", JE::Editor::Render::Scene::DeferredFog::EditorDeferredFogComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorDeferredFogComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorDeferredFogComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorDeferredFogComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorDeferredFogComponent() = default;
            EditorDeferredFogComponent(const JE::Render::Scene::DeferredFogComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
