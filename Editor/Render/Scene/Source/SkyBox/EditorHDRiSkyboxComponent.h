/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Render/Features/Utils/EditorRenderComponentAdapter.h>
#include <JE/Render/Features/SkyBox/SkyboxConstants.h>
#include <SkyBox/HDRiSkyboxComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorHDRiSkyboxComponent final
            : public JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::HDRiSkyboxComponentController, JE::Render::Scene::HDRiSkyboxComponent, JE::Render::Scene::HDRiSkyboxComponentConfig>
        {
        public:

            using BaseClass = JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::HDRiSkyboxComponentController, JE::Render::Scene::HDRiSkyboxComponent, JE::Render::Scene::HDRiSkyboxComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorHDRiSkyboxComponent, "AZ::Render::EditorHDRiSkyboxComponent", JE::Render::Features::EditorHDRiSkyboxComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorHDRiSkyboxComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorHDRiSkyboxComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorHDRiSkyboxComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorHDRiSkyboxComponent() = default;
            EditorHDRiSkyboxComponent(const JE::Render::Scene::HDRiSkyboxComponentConfig& config);
        };

    } // namespace Render
} // namespace AZ
