/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponent.h>
#include <OcclusionCullingPlane/OcclusionCullingPlaneComponentConstants.h>
#include <JE/Editor/Render/Features/Utils/EditorRenderComponentAdapter.h>

namespace JE::Editor::Render
{
    namespace Scene
    {        
        class EditorOcclusionCullingPlaneComponent final
            : public JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::OcclusionCullingPlaneComponentController, JE::Render::Scene::OcclusionCullingPlaneComponent, JE::Render::Scene::OcclusionCullingPlaneComponentConfig>
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:
            using BaseClass = JE::Editor::Render::Features::EditorRenderComponentAdapter<JE::Render::Scene::OcclusionCullingPlaneComponentController, JE::Render::Scene::OcclusionCullingPlaneComponent, JE::Render::Scene::OcclusionCullingPlaneComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorOcclusionCullingPlaneComponent, "AZ::Render::EditorOcclusionCullingPlaneComponent", JE::Render::Scene::EditorOcclusionCullingPlaneComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorOcclusionCullingPlaneComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorOcclusionCullingPlaneComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorOcclusionCullingPlaneComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorOcclusionCullingPlaneComponent();
            EditorOcclusionCullingPlaneComponent(const JE::Render::Scene::OcclusionCullingPlaneComponentConfig& config);

            // AZ::Component overrides
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace Render
} // namespace AZ
