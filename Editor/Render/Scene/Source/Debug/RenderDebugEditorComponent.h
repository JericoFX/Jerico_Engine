/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Atom/Feature/Debug/RenderDebugConstants.h>
#include <Debug/RenderDebugComponent.h>

namespace JE::Editor::Render::Scene
{
    namespace RenderDebug
    {
        inline constexpr AZ::TypeId RenderDebugEditorComponentTypeId{ "{235031F8-2AAD-442D-AB4D-F9B5B8337DCD}" };
    }

    class RenderDebugEditorComponent final
        : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::RenderDebugComponentController, JE::Render::Scene::RenderDebugComponent, JE::Render::Scene::RenderDebugComponentConfig>
    {
    public:

        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::RenderDebugComponentController, JE::Render::Scene::RenderDebugComponent, JE::Render::Scene::RenderDebugComponentConfig>;
        AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::RenderDebugEditorComponent, "AZ::Render::RenderDebugEditorComponent", JE::Editor::Render::Scene::RenderDebug::RenderDebugEditorComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
        AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::RenderDebugEditorComponent)
        AZ_COMPONENT_BASE(JE::Editor::Render::Scene::RenderDebugEditorComponent)
        AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::RenderDebugEditorComponent, AZ::ComponentAllocator);

        static void Reflect(AZ::ReflectContext* context);

        RenderDebugEditorComponent() = default;
        RenderDebugEditorComponent(const JE::Render::Scene::RenderDebugComponentConfig& config);

        //! EditorRenderComponentAdapter overrides...
        AZ::u32 OnConfigurationChanged() override;
    };

}
