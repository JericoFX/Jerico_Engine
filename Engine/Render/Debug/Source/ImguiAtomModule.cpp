/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Debug/RenderDebugModule.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <ImguiAtomSystemComponent.h>

namespace JE::Render::Debug
{
    class ImguiAtomModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ImguiAtomModule, "{E3CE5991-30B5-4B04-BF79-516DDBD4D233}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ImguiAtomModule, AZ::SystemAllocator);

        ImguiAtomModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ImguiAtomSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ImguiAtomSystemComponent>(),
            };
        }
    };
    AZ::Module* CreateViewportModule() { return aznew ImguiAtomModule(); }
}
