/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Integration/RenderIntegrationModule.h>
#include <Bootstrap/BootstrapSystemComponent.h>

namespace JE::Render::Bootstrap
{
    class BootstrapModule final
        : public AZ::Module
    {
    public:
        AZ_RTTI(BootstrapModule, "{ADDE20F4-03E6-4692-A736-E56B87952727}", AZ::Module);
        AZ_CLASS_ALLOCATOR(BootstrapModule, AZ::SystemAllocator);

        explicit BootstrapModule(bool requireRendererSystem)
            : m_requireRendererSystem(requireRendererSystem)
        {
            m_descriptors.push_back(BootstrapSystemComponent::CreateDescriptor());
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            if (!m_requireRendererSystem)
            {
                return {};
            }
            return { azrtti_typeid<BootstrapSystemComponent>() };
        }

    private:
        bool m_requireRendererSystem;
    };
}

namespace JE::Render::Integration
{
    AZ::Module* CreateBootstrapModule(bool requireRendererSystem)
    {
        return aznew Bootstrap::BootstrapModule(requireRendererSystem);
    }
}
