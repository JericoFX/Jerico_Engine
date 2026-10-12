/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/World/Camera/Debug/CameraControllerBus.h>
#include <JE/World/Camera/Debug/ArcBallControllerBus.h>
#include <JE/World/Camera/Debug/NoClipControllerBus.h>

AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_WORLD_CAMERA_API, JE::World::Camera::Debug::CameraControllerRequests);
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_WORLD_CAMERA_API, JE::World::Camera::Debug::CameraControllerNotifications);
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_WORLD_CAMERA_API, JE::World::Camera::Debug::ArcBallControllerRequests);
AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_WORLD_CAMERA_API, JE::World::Camera::Debug::NoClipControllerRequests);
