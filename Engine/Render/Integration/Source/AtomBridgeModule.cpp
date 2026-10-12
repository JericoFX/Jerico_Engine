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
#include <FlyCameraInputComponent.h>
#include <AtomBridgeSystemComponent.h>

namespace JE::Render::Integration
{
        Module::Module()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                    FlyCameraInputComponent::CreateDescriptor(),
                    AtomBridgeSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList Module::GetRequiredSystemComponents() const
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AtomBridgeSystemComponent>(),
            };
        }
    AZ::Module* CreateRuntimeModule() { return aznew Module(); }
} // namespace JE::Render::Integration
