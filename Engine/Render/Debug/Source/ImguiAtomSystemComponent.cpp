/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImguiAtomSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/Feature/ImGui/SystemBus.h>
#include <JE/Render/RPI/Public/ViewportContext.h>

#if defined(IMGUI_ENABLED)
#include <JE/Debug/ImGui/ImGuiBus.h>
#endif

namespace JE::Render::Debug
{
        void ImguiAtomSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImguiAtomSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }

        void ImguiAtomSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ImguiAtomSystemComponent"));
        }

        void ImguiAtomSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("CommonService"));
        }

        void ImguiAtomSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatbile)
        {
            incompatbile.push_back(AZ_CRC_CE("ImguiAtomSystemComponent"));
        }

        void ImguiAtomSystemComponent::Activate()
        {
            auto atomViewportRequests = JE::Render::RPI::ViewportContextRequests::Get();
            AZ_Assert(atomViewportRequests, "AtomViewportContextRequests interface not found!");
            const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
            JE::Render::RPI::ViewportContextNotificationBus::Handler::BusConnect(contextName);

            m_initialized = false;
            InitializeViewportSizeIfNeeded();
        }

        void ImguiAtomSystemComponent::Deactivate()
        {
            JE::Render::RPI::ViewportContextNotificationBus::Handler::BusDisconnect();
        }

        void ImguiAtomSystemComponent::InitializeViewportSizeIfNeeded()
        {
#if defined(IMGUI_ENABLED)
            if (m_initialized)
            {
                return;
            }
            auto atomViewportRequests = AZ::Interface<JE::Render::RPI::ViewportContextRequestsInterface>::Get();
            auto defaultViewportContext = atomViewportRequests->GetDefaultViewportContext();
            if (defaultViewportContext)
            {
                // If this succeeds, m_initialized will be set to true.
                OnViewportSizeChanged(defaultViewportContext->GetViewportSize());
            }
#endif
        }

        void ImguiAtomSystemComponent::OnRenderTick()
        {
#if defined(IMGUI_ENABLED)
            InitializeViewportSizeIfNeeded();
            JE::Debug::IImGuiManager* imguiManager = AZ::Interface<JE::Debug::IImGuiManager>().Get();
            if (imguiManager != nullptr)
            {
                ImDrawData* drawData = imguiManager->GetImguiDrawData();
                if (drawData != nullptr)
                {
                    AZ::Render::ImGuiSystemRequestBus::Broadcast(&AZ::Render::ImGuiSystemRequests::RenderImGuiBuffersToCurrentViewport, *drawData);
                }
            }
#endif
        }

        void ImguiAtomSystemComponent::OnViewportSizeChanged([[maybe_unused]] AzFramework::WindowSize size)
        {
#if defined(IMGUI_ENABLED)
            JE::Debug::ImGuiManagerBus::Broadcast([this, size](JE::Debug::ImGuiManagerBus::Events* imgui)
            {
                imgui->OverrideRenderWindowSize(size.m_width, size.m_height);
                // ImGuiManagerListenerBus may not have been connected when this system component is activated
                // as ImGuiManager is not part of a system component we can  require and instead just listens for ESYSTEM_EVENT_GAME_POST_INIT.
                // Let our ImguiAtomSystemComponent know once we successfully connect and update the viewport size.
                if (!m_initialized)
                {
                    auto atomViewportRequests = JE::Render::RPI::ViewportContextRequests::Get();
                    auto defaultViewportContext = atomViewportRequests->GetDefaultViewportContext();
                    OnViewportDpiScalingChanged(defaultViewportContext->GetDpiScalingFactor());
                    m_initialized = true;
                }
            });
#endif //define(IMGUI_ENABLED)
        }

        void ImguiAtomSystemComponent::OnViewportDpiScalingChanged([[maybe_unused]] float dpiScale)
        {
#if defined(IMGUI_ENABLED)
            JE::Debug::ImGuiManagerBus::Broadcast(&JE::Debug::ImGuiManagerBus::Events::SetDpiScalingFactor, dpiScale);
#endif //define(IMGUI_ENABLED)
        }
}
