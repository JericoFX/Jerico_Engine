/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        //! The main interface, usable in Editor mode, to request an "HDR Color Grading" component to generate & activate
        //! the LUT. The operations are asynchronous, so it is important to register for completion events
        //! on the EditorHDRColorGradingNotificationBus. 
        class EditorHDRColorGradingRequests
            : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorHDRColorGradingRequests, "AZ::Render::EditorHDRColorGradingRequests", "{13C81A89-587D-4AA6-B66D-903F8F947EF0}");

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual ~EditorHDRColorGradingRequests() {}

            // Starts generating/baking an LUT asset. When the asset is generated a notification
            // will be sent with EditorHDRColorGradingNotificationBus::OnGenerateLutCompleted.
            virtual void GenerateLutAsync() = 0;

            // Adds and activates a "Look Modification" component using the LUT asset
            // genrated when GenerateLutAsync() was called. Also, the "HDR Color Grading" component
            // will be deactivated.
            // When the whole operation is finished, a notification
            // will be sent with EditorHDRColorGradingNotificationBus::OnActivateLutCompleted.
            virtual void ActivateLutAsync() = 0;

        };

        typedef AZ::EBus<EditorHDRColorGradingRequests> EditorHDRColorGradingRequestBus;


        //! This EBus is useful to get notified whenever operations invoked on EditorHDRColorGradingRequestBus
        //! are completed. This notification EBus is only useful in Editor mode.
        class EditorHDRColorGradingNotification
            : public AZ::ComponentBus
        {
        public:

            //! Destroys the instance of the class.
            virtual ~EditorHDRColorGradingNotification() {}

            //! This event is sent on response to EditorHDRColorGradingRequestBus::GenerateLutAsync()
            //! when the LUT asset is ready.
            virtual void OnGenerateLutCompleted(const AZStd::string& lutAssetAbsolutePath) = 0;

            //! This event is sent on response to EditorHDRColorGradingRequestBus::ActivateLutAsync()
            //! when the "Look Modification" component is activated.
            virtual void OnActivateLutCompleted() = 0;

        };

        using EditorHDRColorGradingNotificationBus = AZ::EBus<EditorHDRColorGradingNotification>;

    } // namespace Render
} // namespace AZ
