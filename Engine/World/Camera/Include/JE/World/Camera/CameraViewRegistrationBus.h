/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <JE/World/Camera/CameraExports.h>

#include <AzCore/EBus/EBus.h>

#include <Atom/RPI.Public/Base.h>
#include <AzCore/Component/EntityId.h>

namespace JE::World::Camera
{
    class CameraViewRegistrationRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void SetViewForEntity([[maybe_unused]] const AZ::EntityId& id, [[maybe_unused]] AZ::RPI::ViewPtr view) {}
        virtual AZ::RPI::ViewPtr GetViewForEntity([[maybe_unused]]const AZ::EntityId& id) { return {}; }
    };

    using CameraViewRegistrationRequestsBus = AZ::EBus<CameraViewRegistrationRequests>;
} //namespace Camera

AZ_DECLARE_EBUS_SINGLE_ADDRESS(JE_WORLD_CAMERA_API, JE::World::Camera::CameraViewRegistrationRequests);
