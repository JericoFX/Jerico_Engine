/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <JE/Render/Integration/RenderIntegrationExports.h>

#include <JE/Render/Integration/PerViewportDynamicDrawInterface.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>

namespace JE::Render::Integration
{
    class JE_RENDER_INTEGRATION_API PerViewportDynamicDrawManager final : public PerViewportDynamicDrawInterface
    {
    public:
        AZ_TYPE_INFO(PerViewportDynamicDrawManager, "{BED66185-00A7-43F7-BD28-C56BC8E4C535}");

        PerViewportDynamicDrawManager();
        ~PerViewportDynamicDrawManager();

        // PerViewportDynamicDrawInterface overrides...
        void RegisterDynamicDrawContext(AZ::Name name, DrawContextFactory contextInitializer) override;
        void UnregisterDynamicDrawContext(AZ::Name name) override;
        AZ::RHI::Ptr<JE::Render::RPI::DynamicDrawContext> GetDynamicDrawContextForViewport(AZ::Name name, AzFramework::ViewportId viewportId) override;

    private:
        struct ViewportData
        {
            AZStd::unordered_map<AZ::Name, AZ::RHI::Ptr<JE::Render::RPI::DynamicDrawContext>> m_dynamicDrawContexts;

            // Event handlers
            AZ::Event<JE::Render::RPI::RenderPipelinePtr>::Handler m_pipelineChangedHandler;
            AZ::Event<AzFramework::ViewportId>::Handler m_viewportDestroyedHandler;

            // Cached state
            bool m_initialized = false;
        };
        AZStd::map<AzFramework::ViewportId, ViewportData> m_viewportData;
        AZStd::unordered_map<AZ::Name, DrawContextFactory> m_registeredDrawContexts;
        AZStd::mutex m_mutexDrawContexts;
    };
} //namespace JE::Render::Integration
