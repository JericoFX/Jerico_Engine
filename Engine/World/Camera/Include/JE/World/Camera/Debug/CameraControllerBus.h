/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

#include <JE/World/Camera/CameraExports.h>

namespace AZ { class ReflectContext; }

namespace JE::World::Camera::Debug
{
    //! Requests sent to any camera controllers in one entity
    class CameraControllerRequests
        : public AZ::ComponentBus
    {
    public:
        using MutexType = AZStd::recursive_mutex;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Enable the controller that has this typeId. Any other controllers which are different types will be disabled.
        virtual void Enable(AZ::TypeId typeId) = 0;

        //! Reset a controller to initial stats if it's enabled.
        virtual void Reset() = 0;

        //! Disable the controller which is enabled
        virtual void Disable() = 0;
    };

    using CameraControllerRequestBus = AZ::EBus<CameraControllerRequests>;


    //! Notifications sent by each camera controller
    class CameraControllerNotifications
        : public AZ::ComponentBus
    {
    public:

        //! @param typeId  the ID of the controller that was enabled
        virtual void OnControllerEnabled([[maybe_unused]] AZ::TypeId typeId) {}

        //! @param typeId  the ID of the controller that was disabled
        virtual void OnControllerDisabled([[maybe_unused]] AZ::TypeId typeId) {}

        //! Called when user input begins a camera move
        //! @param controllerTypeId  the ID of this camera controller
        //! @param channels  the bitmask indicating the channels that began moving. The channel values are controller-specific.
        virtual void OnCameraMoveBegan([[maybe_unused]] AZ::TypeId controllerTypeId, [[maybe_unused]] uint32_t channels) {}

        //! Called when user input ends after a camera move
        //! @param controllerTypeId  the ID of this camera controller
        //! @param channels  the bitmask indicating the channels that ended moving. The channel values are controller-specific.
        virtual void OnCameraMoveEnded([[maybe_unused]] AZ::TypeId controllerTypeId, [[maybe_unused]] uint32_t channels) {}
    };

    using CameraControllerNotificationBus = AZ::EBus<CameraControllerNotifications>;
} // namespace JE::World::Camera::Debug

AZ_DECLARE_EBUS_MULTI_ADDRESS(JE_WORLD_CAMERA_API, JE::World::Camera::Debug::CameraControllerRequests);
AZ_DECLARE_EBUS_MULTI_ADDRESS(JE_WORLD_CAMERA_API, JE::World::Camera::Debug::CameraControllerNotifications);
