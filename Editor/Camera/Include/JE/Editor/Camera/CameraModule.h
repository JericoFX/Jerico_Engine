/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <JE/World/Camera/CameraModule.h>
#include <JE/Editor/Camera/CameraExports.h>

namespace JE::Editor::Camera
{
    class JE_EDITOR_CAMERA_API CameraAuthoringModule : public JE::World::Camera::CameraModule
    {
    public:
        AZ_RTTI(CameraAuthoringModule, "{7BB66336-1C06-445A-A5D3-9CFE348DB81D}", JE::World::Camera::CameraModule);
        CameraAuthoringModule();
        ~CameraAuthoringModule() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
    JE_EDITOR_CAMERA_API AZ::Module* CreateAuthoringModule();
}
