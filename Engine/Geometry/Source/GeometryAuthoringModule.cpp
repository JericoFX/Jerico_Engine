/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Components/EditorWhiteBoxColliderComponent.h"
#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxSystemComponent.h"
#include "GeometryAuthoringModule.h"

namespace JE::Geometry
{
    AZ_CLASS_ALLOCATOR_IMPL(GeometryAuthoringModule, AZ::SystemAllocator)

    GeometryAuthoringModule::GeometryAuthoringModule()
        : GeometryModule()
    {
        // push results of [MyComponent]::CreateDescriptor() into m_descriptors here
        m_descriptors.insert(
            m_descriptors.end(),
            {
                EditorWhiteBoxSystemComponent::CreateDescriptor(),
                EditorWhiteBoxComponent::CreateDescriptor(),
                EditorWhiteBoxColliderComponent::CreateDescriptor(),
            });
    }

    GeometryAuthoringModule::~GeometryAuthoringModule() = default;

    AZ::ComponentTypeList GeometryAuthoringModule::GetRequiredSystemComponents() const
    {
        // add required SystemComponents to the SystemEntity
        return AZ::ComponentTypeList{azrtti_typeid<EditorWhiteBoxSystemComponent>()};
    }
    AZ::Module* CreateAuthoringModule()
    {
        return aznew GeometryAuthoringModule;
    }
} // namespace JE::Geometry
