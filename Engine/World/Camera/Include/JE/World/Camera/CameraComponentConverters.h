/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <AzCore/Serialization/SerializeContext.h>
#include <JE/World/Camera/CameraExports.h>

namespace JE::World::Camera::ClassConverters
{
    JE_WORLD_CAMERA_API bool DeprecateCameraComponentWithoutEditor(
        AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    JE_WORLD_CAMERA_API bool UpdateCameraComponentToUseController(
        AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
}
