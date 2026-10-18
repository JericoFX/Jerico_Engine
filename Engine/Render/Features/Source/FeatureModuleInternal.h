// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once
#include <JE/Render/Features/RenderFeaturesExports.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Module/Module.h>

namespace JE::Render::Features
{
    template<class ComponentType>
    void AppendOwnedComponentDescriptor(AZStd::list<AZ::ComponentDescriptor*>& descriptors)
    {
        AZ::ComponentDescriptor* existing = nullptr;
        AZ::ComponentDescriptorBus::EventResult(
            existing, azrtti_typeid<ComponentType>(), &AZ::ComponentDescriptor::GetDescriptor);
        // Always use the original factory, including its UUID/type-name validation.
        AZ::ComponentDescriptor* descriptor = ComponentType::CreateDescriptor();
        if (!existing && descriptor)
        {
            descriptors.push_back(descriptor);
        }
    }

    JE_RENDER_FEATURES_API void AppendOwnedRuntimeComponentDescriptors(AZStd::list<AZ::ComponentDescriptor*>& descriptors);
    JE_RENDER_FEATURES_API AZ::ComponentTypeList GetRuntimeRequiredSystemComponents();
}
