/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/World/Camera/CameraModule.h>
#include <JE/World/Camera/CameraComponent.h>
#include <JE/World/Camera/CameraSystemComponent.h>
#include <JE/World/Camera/CameraRigComponent.h>

namespace JE::World::Camera
{
    CameraModule::CameraModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            CameraComponent::CreateDescriptor(),
            CameraSystemComponent::CreateDescriptor(),
            CameraRigComponent::CreateDescriptor() });
    }
    CameraModule::~CameraModule() = default;
    AZ::ComponentTypeList CameraModule::GetRequiredSystemComponents() const
    {
        return { azrtti_typeid<CameraSystemComponent>() };
    }
    AZ::Module* CreateRuntimeModule()
    {
        return aznew CameraModule;
    }
}
