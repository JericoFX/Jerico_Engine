/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Scene/Debug/RayTracingDebugComponentConfig.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Debug/RayTracingDebugComponent.h>
#include <Debug/RayTracingDebugComponentController.h>

namespace JE::Editor::Render::Scene
{
    class RayTracingDebugEditorComponent final
        : public AzToolsFramework::Components::
              EditorComponentAdapter<JE::Render::Scene::RayTracingDebugComponentController, JE::Render::Scene::RayTracingDebugComponent, JE::Render::Scene::RayTracingDebugComponentConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::
            EditorComponentAdapter<JE::Render::Scene::RayTracingDebugComponentController, JE::Render::Scene::RayTracingDebugComponent, JE::Render::Scene::RayTracingDebugComponentConfig>;
        AZ_EDITOR_COMPONENT(RayTracingDebugEditorComponent, "{352A6033-4127-4A8D-BE8A-3FA1267B02EB}", BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        // EditorComponentAdapter overrides
        AZ::u32 OnConfigurationChanged() override;
    };
} // namespace AZ::Render
