/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/FilmGrain/FilmGrainComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace FilmGrain
        {
            static constexpr const char* const EditorFilmGrainComponentTypeId = "{61D39B81-DE19-482B-97FF-3761F2C25E4D}";
        }

        class EditorFilmGrainComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<JE::Render::Scene::FilmGrainComponentController, JE::Render::Scene::FilmGrainComponent, JE::Render::Scene::FilmGrainComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<JE::Render::Scene::FilmGrainComponentController, JE::Render::Scene::FilmGrainComponent, JE::Render::Scene::FilmGrainComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorFilmGrainComponent, "AZ::Render::EditorFilmGrainComponent", JE::Editor::Render::Scene::FilmGrain::EditorFilmGrainComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorFilmGrainComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorFilmGrainComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorFilmGrainComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorFilmGrainComponent() = default;
            EditorFilmGrainComponent(const JE::Render::Scene::FilmGrainComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
