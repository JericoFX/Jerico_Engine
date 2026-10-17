/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Base.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <CrySystemBus.h>
#include <JE/Render/Bootstrap/BootstrapNotificationBus.h>


namespace JE::Editor::Render
{
    namespace Scene
    {
        //! This class is responsible for displaying stats about Atom's SkinnedMesh feature
        class SkinnedMeshDebugDisplay
            : private AzFramework::ViewportDebugDisplayEventBus::Handler
            , private CrySystemEventBus::Handler
            , private JE::Render::Bootstrap::NotificationBus::Handler
        {
        public:
            SkinnedMeshDebugDisplay();
            ~SkinnedMeshDebugDisplay();

        private:

            // CrySystemEventBus::Handler overrides
            // Register the r_skinnedMeshDisplaySceneStats cvar
            void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
            void OnCrySystemShutdown(ISystem&) override;
            void OnCryEditorInitialized() override;

            // AzFramework::ViewportDebugDisplayEventBus overrides
            // Display the debug text if r_skinnedMeshDisplaySceneStats = 1
            void DisplayViewport2d(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

            // Get the RPI::SceneId, which can be used to query scene stats
            void OnBootstrapSceneReady(JE::Render::RPI::Scene* bootstrapScene) override;

            void GetSceneId();

            // CVar for toggling the display of the scene stats
            int r_skinnedMeshDisplaySceneStats = 0;
            // SceneId to query for the stats
            JE::Render::RPI::SceneId m_sceneId;
        };
    }// namespace Render
}// namespace AZ
