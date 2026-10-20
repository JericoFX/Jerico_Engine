/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Components/DiffuseGlobalIlluminationComponent.h>

namespace JE::Editor::Render
{
    namespace Features
    {
        class EditorDiffuseGlobalIlluminationComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Features::DiffuseGlobalIlluminationComponentController, JE::Render::Features::DiffuseGlobalIlluminationComponent, JE::Render::Features::DiffuseGlobalIlluminationComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<JE::Render::Features::DiffuseGlobalIlluminationComponentController, JE::Render::Features::DiffuseGlobalIlluminationComponent, JE::Render::Features::DiffuseGlobalIlluminationComponentConfig>;
            AZ_RTTI_WITH_NAME(EditorDiffuseGlobalIlluminationComponent, "AZ::Render::EditorDiffuseGlobalIlluminationComponent", JE::Render::Features::EditorDiffuseGlobalIlluminationComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase);
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(EditorDiffuseGlobalIlluminationComponent)
            AZ_COMPONENT_BASE(EditorDiffuseGlobalIlluminationComponent)
            AZ_CLASS_ALLOCATOR(EditorDiffuseGlobalIlluminationComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorDiffuseGlobalIlluminationComponent() = default;
            EditorDiffuseGlobalIlluminationComponent(const JE::Render::Features::DiffuseGlobalIlluminationComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Features
} // namespace JE::Editor::Render
