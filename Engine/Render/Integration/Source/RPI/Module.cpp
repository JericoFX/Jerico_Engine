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

#include <Atom/RPI.Public/Image/ImageTagSystemComponent.h>
#include <Atom/RPI.Public/Model/ModelTagSystemComponent.h>
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
        m_descriptors.push_back(AZ::RPI::ImageTagSystemComponent::CreateDescriptor());
        m_descriptors.push_back(AZ::RPI::ModelTagSystemComponent::CreateDescriptor());
        m_descriptors.push_back(PassTemplatesAutoLoader::CreateDescriptor());
    }

    AZ::ComponentTypeList Module::GetRequiredSystemComponents() const
    {
        return
        {
            azrtti_typeid<Runtime::RPISystemComponent>(),
            azrtti_typeid<AZ::RPI::ImageTagSystemComponent>(),
            azrtti_typeid<AZ::RPI::ModelTagSystemComponent>(),
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
