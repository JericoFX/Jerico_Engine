/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <SkyBox/PhysicalSkyComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        class EditorPhysicalSkyComponent final
            : public AZ::Render::EditorRenderComponentAdapter<JE::Render::Scene::PhysicalSkyComponentController, JE::Render::Scene::PhysicalSkyComponent, JE::Render::Scene::PhysicalSkyComponentConfig>
        {
        public:

            using BaseClass = AZ::Render::EditorRenderComponentAdapter<JE::Render::Scene::PhysicalSkyComponentController, JE::Render::Scene::PhysicalSkyComponent, JE::Render::Scene::PhysicalSkyComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorPhysicalSkyComponent, "AZ::Render::EditorPhysicalSkyComponent", AZ::Render::EditorPhysicalSkyComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorPhysicalSkyComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorPhysicalSkyComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorPhysicalSkyComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorPhysicalSkyComponent() = default;
            EditorPhysicalSkyComponent(const JE::Render::Scene::PhysicalSkyComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
