/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <PostProcess/Bloom/BloomComponent.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        namespace Bloom
        {
            static constexpr const char* const EditorBloomComponentTypeId =
                "{33789179-AB9C-4891-9DA3-1972EAED6719}";
        }

        class EditorBloomComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::BloomComponentController, JE::Render::Scene::BloomComponent, JE::Render::Scene::BloomComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Scene::BloomComponentController, JE::Render::Scene::BloomComponent, JE::Render::Scene::BloomComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorBloomComponent, "AZ::Render::EditorBloomComponent", JE::Editor::Render::Scene::Bloom::EditorBloomComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorBloomComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorBloomComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorBloomComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorBloomComponent() = default;
            EditorBloomComponent(const JE::Render::Scene::BloomComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
