/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Grid/GridComponent.h>
#include <JE/Render/Scene/Grid/GridComponentConstants.h>
#include <JE/Render/Scene/Grid/GridComponentBus.h>
#include <JE/Editor/Render/Features/Utils/EditorRenderComponentAdapter.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        //! In-editor grid component
        class EditorGridComponent final
            : public JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::GridComponentController, JE::Render::Scene::GridComponent, JE::Render::Scene::GridComponentConfig>
        {
        public:
            using BaseClass = JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::GridComponentController, JE::Render::Scene::GridComponent, JE::Render::Scene::GridComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorGridComponent, "AZ::Render::EditorGridComponent", JE::Render::Scene::EditorGridComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorGridComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorGridComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorGridComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorGridComponent() = default;
            EditorGridComponent(const JE::Render::Scene::GridComponentConfig& config);
        };
    } // namespace Render
} // namespace AZ
