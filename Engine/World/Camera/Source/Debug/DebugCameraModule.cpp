/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/World/Camera/Debug/DebugCameraModule.h>
#include <JE/World/Camera/Debug/ArcBallControllerComponent.h>
#include <JE/World/Camera/Debug/CameraComponent.h>
#include <JE/World/Camera/Debug/NoClipControllerComponent.h>

namespace JE::World::Camera::Debug
{
    CameraModule::CameraModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            CameraControllerComponent::CreateDescriptor(),
            ArcBallControllerComponent::CreateDescriptor(),
            CameraComponent::CreateDescriptor(),
            NoClipControllerComponent::CreateDescriptor() });
    }

    CameraModule::~CameraModule() = default;

    AZ::ComponentTypeList CameraModule::GetRequiredSystemComponents() const
    {
        return {};
    }

    AZ::Module* CreateDebugModule()
    {
        return aznew CameraModule;
    }
}
