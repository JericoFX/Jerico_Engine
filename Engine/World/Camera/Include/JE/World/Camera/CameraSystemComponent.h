/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <JE/World/Camera/CameraExports.h>

#include <AzCore/Component/Component.h>

#include <AzFramework/Components/CameraBus.h>

namespace JE::World::Camera
{
    class JE_WORLD_CAMERA_API CameraSystemComponent
        : public AZ::Component
        , private ::Camera::CameraSystemRequestBus::Handler
        , private ::Camera::ActiveCameraRequestBus::Handler
        , private ::Camera::CameraNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(CameraSystemComponent, "{5DF8DB49-6430-4718-9417-85321596EDA5}");
        static void Reflect(AZ::ReflectContext* context);

        CameraSystemComponent() = default;
        ~CameraSystemComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        //////////////////////////////////////////////////////////////////////////
        // CameraSystemRequestBus
        AZ::EntityId GetActiveCamera() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ActiveCameraRequestBus
        const AZ::Transform& GetActiveCameraTransform() override;
        const ::Camera::Configuration& GetActiveCameraConfiguration() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // CameraNotificationBus
        void OnActiveViewChanged(const AZ::EntityId&) override;
        //////////////////////////////////////////////////////////////////////////

        struct CameraProperties
        {
            AZ::Transform transform;
            ::Camera::Configuration configuration;
        };

        AZ::EntityId m_activeView;
        CameraProperties m_activeViewProperties;
    };
} // namespace Camera
