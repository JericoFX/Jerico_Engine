/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Module.h"
#include "RPISystemComponent.h"
#include "PassTemplatesAutoLoader.h"

#include <JE/Render/RPI/Public/Image/ImageTagSystemComponent.h>
#include <JE/Render/RPI/Public/Model/ModelTagSystemComponent.h>
#include <JE/Render/Integration/RenderIntegrationModule.h>

namespace JE::Render::RPI
{
    Module::Module()
        : Module(Runtime::RPISystemComponent::CreateDescriptor())
    {
    }

    Module::Module(AZ::ComponentDescriptor* rpiSystemDescriptor)
    {
        m_descriptors.push_back(rpiSystemDescriptor);
        m_descriptors.push_back(JE::Render::RPI::ImageTagSystemComponent::CreateDescriptor());
        m_descriptors.push_back(JE::Render::RPI::ModelTagSystemComponent::CreateDescriptor());
        m_descriptors.push_back(PassTemplatesAutoLoader::CreateDescriptor());
    }

    AZ::ComponentTypeList Module::GetRequiredSystemComponents() const
    {
        return
        {
            azrtti_typeid<Runtime::RPISystemComponent>(),
            azrtti_typeid<JE::Render::RPI::ImageTagSystemComponent>(),
            azrtti_typeid<JE::Render::RPI::ModelTagSystemComponent>(),
            azrtti_typeid<PassTemplatesAutoLoader>()
        };
    }
} // namespace JE::Render::RPI

namespace JE::Render::Integration
{
    AZ::Module* CreateRpiRuntimeModule()
    {
        return aznew JE::Render::RPI::Module;
    }
} // namespace JE::Render::Integration
