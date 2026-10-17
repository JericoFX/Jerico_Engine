/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <DebugConsole.h>
#include <JE/Render/Debug/RenderDebugExports.h>

#include <JE/Render/RPI/Public/ViewportContextBus.h>

namespace JE::Render::Debug
{
        class ImGuiFeatureProcessor;

        //! Connects the native ImGui manager draw data to Atom's existing viewport pass.
        //! The manager uses the Atom pass marked as the default; it does not create another render context.
        class JE_RENDER_DEBUG_API ImguiAtomSystemComponent final
            : public AZ::Component
            , public JE::Render::RPI::ViewportContextNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(ImguiAtomSystemComponent, "{D423E075-D971-435A-A9C1-57C3B0623A9B}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile);

            ~ImguiAtomSystemComponent() override = default;

            // AZ::Component
            void Activate() override;
            void Deactivate() override;

        private:
            void InitializeViewportSizeIfNeeded();

            // ViewportContextNotificationBus overrides...
            void OnRenderTick() override;
            void OnViewportSizeChanged(AzFramework::WindowSize size) override;
            void OnViewportDpiScalingChanged(float dpiScale) override;

            DebugConsole m_debugConsole;
            bool m_initialized = false;
        };
} // namespace JE::Render::Debug
