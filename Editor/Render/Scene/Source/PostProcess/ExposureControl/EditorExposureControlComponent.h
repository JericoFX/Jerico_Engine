/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <JE/Render/Scene/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <PostProcess/ExposureControl/ExposureControlComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorExposureControlComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::ExposureControlComponentController, JE::Render::Scene::ExposureControlComponent, JE::Render::Scene::ExposureControlComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::ExposureControlComponentController, JE::Render::Scene::ExposureControlComponent, JE::Render::Scene::ExposureControlComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorExposureControlComponent, "AZ::Render::EditorExposureControlComponent", JE::Render::Scene::EditorExposureControlComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorExposureControlComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorExposureControlComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorExposureControlComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorExposureControlComponent() = default;
            EditorExposureControlComponent(const JE::Render::Scene::ExposureControlComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
