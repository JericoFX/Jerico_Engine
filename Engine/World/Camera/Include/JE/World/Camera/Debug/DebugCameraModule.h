/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <AzCore/Module/Module.h>
#include <JE/World/Camera/CameraExports.h>

namespace JE::World::Camera::Debug
{
    class JE_WORLD_CAMERA_API CameraModule : public AZ::Module
    {
    public:
        AZ_RTTI(CameraModule, "{C4F5D301-5C7F-42C2-8326-08F685B2D7A3}", AZ::Module);
        CameraModule();
        ~CameraModule() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };

    // Register existing preview cameras and controllers in the host's ModuleManager.
    // This does not create another CameraSystemComponent or rendering context.
    JE_WORLD_CAMERA_API AZ::Module* CreateDebugModule();
}
