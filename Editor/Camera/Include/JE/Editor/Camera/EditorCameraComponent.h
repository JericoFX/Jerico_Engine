/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <JE/Editor/Camera/CameraExports.h>

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Components/CameraBus.h>

#include <AzFramework/Components/EditorEntityEvents.h>
#include <JE/World/Camera/CameraComponent.h>
#include <JE/World/Camera/CameraComponentController.h>

#include <JE/Render/RPI/Public/Base.h>

namespace JE::Editor::Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// The CameraComponent holds all of the data necessary for a camera.
    /// Get and set data through the CameraRequestBus or TransformBus
    //////////////////////////////////////////////////////////////////////////
    using EditorCameraComponentBase = AzToolsFramework::Components::EditorComponentAdapter<JE::World::Camera::CameraComponentController, JE::World::Camera::CameraComponent, JE::World::Camera::CameraComponentConfig>;
    class JE_EDITOR_CAMERA_API EditorCameraComponent
        : public EditorCameraComponentBase
        , public ::Camera::EditorCameraViewRequestBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCameraComponent, EditorCameraComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        virtual ~EditorCameraComponent() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;
        AZ::u32 OnConfigurationChanged() override;

        // AzFramework::DebugDisplayRequestBus::Handler overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        /// EditorCameraViewRequestBus::Handler overrides ...
        void ToggleCameraAsActiveView() override;
        void MatchViewport() override;
        bool IsActiveCamera() const override;
        bool GetCameraState(AzFramework::CameraState& cameraState) override;

    protected:
        void EditorDisplay(AzFramework::DebugDisplayRequests& displayInterface, const AZ::Transform& world);
        AZ::Crc32 OnPossessCameraButtonClicked();
        AZ::Crc32 OnMatchViewportClicked();
        AZStd::string GetCameraViewButtonText() const;

        float m_frustumViewPercentLength = 1.f;
        AZ::Color m_frustumDrawColor = AzFramework::ViewportColors::HoverColor;
    };
} // Camera
