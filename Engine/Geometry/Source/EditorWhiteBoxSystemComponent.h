/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Geometry/GeometryExports.h>

#include "WhiteBoxSystemComponent.h"
#include "Editor/JEEditorFixtureVerification.h"
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace JE::Geometry
{
    //! System component for the White Box Editor/Tool application.
    class JE_GEOMETRY_AUTHORING_API EditorWhiteBoxSystemComponent
        : public WhiteBoxSystemComponent
        , public AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorWhiteBoxSystemComponent, "{42D40E84-A8C4-474B-A4D6-B665CCEA8A83}", WhiteBoxSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionContextModeRegistrationHook() override;
        void OnActionUpdaterRegistrationHook() override;
        void OnActionRegistrationHook() override;
        void OnActionContextModeBindingHook() override;
        void OnMenuBindingHook() override;

    private:
        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        JE::BSP::Editor::FixtureVerification m_fixtureVerification;
    };
} // namespace JE::Geometry
