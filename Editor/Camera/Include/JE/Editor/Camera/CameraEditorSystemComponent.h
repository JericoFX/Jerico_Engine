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

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorCameraBus.h>

#include <JE/World/Camera/CameraViewRegistrationBus.h>

namespace JE::Editor::Camera
{
    class JE_EDITOR_CAMERA_API CameraEditorSystemComponent
        : public AZ::Component
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private ::Camera::EditorCameraSystemRequestBus::Handler
        , private JE::World::Camera::CameraViewRegistrationRequestsBus::Handler
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(CameraEditorSystemComponent, "{769802EB-722A-4F89-A475-DA396DA1FDCC}");
        static void Reflect(AZ::ReflectContext* context);

        CameraEditorSystemComponent() = default;
        ~CameraEditorSystemComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEvents
        void NotifyRegisterViews() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EditorCameraRequestBus::Handler
        void CreateCameraEntityFromViewport() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // CameraViewRegistrationRequestsBus::Handler
        void SetViewForEntity(const AZ::EntityId& id, JE::Render::RPI::ViewPtr view) override;
        JE::Render::RPI::ViewPtr GetViewForEntity(const AZ::EntityId& id) override;
        //////////////////////////////////////////////////////////////////////////

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionRegistrationHook() override;
        void OnMenuBindingHook() override;

        AZStd::map<AZ::EntityId, AZStd::weak_ptr<JE::Render::RPI::View>> m_entityViewMap;
    };
}
