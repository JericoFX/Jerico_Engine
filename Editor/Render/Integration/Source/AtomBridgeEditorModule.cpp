/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <JE/Render/Integration/RenderIntegrationModule.h>

#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>
#include <JE/Editor/RenderIntegration/AssetProcessingModule.h>
#include "AssetCollectionAsyncLoaderTestComponent.h"

namespace JE::Editor::RenderIntegration
{
        class EditorModule
            : public JE::Render::Integration::Module
        {
        public:
            AZ_RTTI(EditorModule, "{7B330394-BE9C-4BDA-9345-1A0859815982}", JE::Render::Integration::Module);
            AZ_CLASS_ALLOCATOR(EditorModule, AZ::SystemAllocator);

            explicit EditorModule(bool requireRendererSystem = true)
                : JE::Render::Integration::Module()
                , m_requireRendererSystem(requireRendererSystem)
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                m_descriptors.insert(m_descriptors.end(), {
                    AssetCollectionAsyncLoaderTestComponent::CreateDescriptor(),
                    });
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                if (!m_requireRendererSystem)
                {
                    return {};
                }
                return JE::Render::Integration::Module::GetRequiredSystemComponents();
            }

        private:
            bool m_requireRendererSystem;
        };
    AZ::Module* CreateAuthoringModule() { return aznew EditorModule(); }
    AZ::Module* CreateAssetProcessingModule() { return aznew EditorModule(false); }
} // namespace JE::Editor::RenderIntegration
