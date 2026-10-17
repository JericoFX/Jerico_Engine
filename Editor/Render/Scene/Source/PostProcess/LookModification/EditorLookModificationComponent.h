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
#include <PostProcess/LookModification/LookModificationComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorLookModificationComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::LookModificationComponentController, JE::Render::Scene::LookModificationComponent, JE::Render::Scene::LookModificationComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::LookModificationComponentController, JE::Render::Scene::LookModificationComponent, JE::Render::Scene::LookModificationComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorLookModificationComponent, "AZ::Render::EditorLookModificationComponent", JE::Render::Scene::EditorLookModificationComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorLookModificationComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorLookModificationComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorLookModificationComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorLookModificationComponent() = default;
            EditorLookModificationComponent(const JE::Render::Scene::LookModificationComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
