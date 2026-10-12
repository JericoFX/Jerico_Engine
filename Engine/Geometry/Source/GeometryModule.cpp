/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Components/WhiteBoxColliderComponent.h"
#include "WhiteBoxComponent.h"
#include <JE/Geometry/GeometryModule.h>
#include "WhiteBoxSystemComponent.h"

namespace JE::Geometry
{
    AZ_CLASS_ALLOCATOR_IMPL(GeometryModule, AZ::SystemAllocator)

    GeometryModule::GeometryModule()
    {
        // push results of [MyComponent]::CreateDescriptor() into m_descriptors here
        m_descriptors.insert(
            m_descriptors.end(),
            {
                WhiteBoxSystemComponent::CreateDescriptor(),
                WhiteBoxColliderComponent::CreateDescriptor(),
                WhiteBoxComponent::CreateDescriptor(),
            });
    }

    GeometryModule::~GeometryModule()
    {
    }

    AZ::ComponentTypeList GeometryModule::GetRequiredSystemComponents() const
    {
        // add required SystemComponents to the SystemEntity
        return AZ::ComponentTypeList{azrtti_typeid<WhiteBoxSystemComponent>()};
    }
    AZ::Module* CreateRuntimeModule()
    {
        return aznew GeometryModule;
    }
} // namespace JE::Geometry
