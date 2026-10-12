/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <AzCore/Module/Module.h>
#include <JE/World/Camera/CameraExports.h>

namespace JE::World::Camera
{
    class JE_WORLD_CAMERA_API CameraModule : public AZ::Module
    {
    public:
        AZ_RTTI(CameraModule, "{C2E72B0D-BCEF-452A-9BFA-03833015258C}", AZ::Module);
        CameraModule();
        ~CameraModule() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };

    // The host transfers ownership to its existing ModuleManager lifecycle.
    JE_WORLD_CAMERA_API AZ::Module* CreateRuntimeModule();
}
