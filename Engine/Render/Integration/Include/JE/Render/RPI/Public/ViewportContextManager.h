/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/deque.h>
#include <AzCore/std/parallel/atomic.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/ViewportContextBus.h>
#include <JE/Render/RPI/Public/ViewportContext.h>
#include <JE/Render/RPI/Public/ViewGroup.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Exposes the AZ::Interface for ViewportContextRequestsInterface
        class JE_RENDER_INTEGRATION_API ViewportContextManager final
            : public ViewportContextRequestsInterface
        {
        public:
            AZ_RTTI(ViewportContextManager, "{1198B0B3-109A-4D34-B8EC-2E727ACD9740}", ViewportContextRequestsInterface);

            static constexpr AzFramework::ViewportId StartingViewportId = 1000;

            ~ViewportContextManager();
            void Shutdown();

            // ViewportContextRequestsInterface overrides...
            ViewportContextPtr CreateViewportContext(const AZ::Name& contextName, const CreationParameters& params) override;
            ViewportContextPtr GetViewportContextByName(const AZ::Name& contextName) const override;
            ViewportContextPtr GetViewportContextById(AzFramework::ViewportId id) const override;
            void RenameViewportContext(ViewportContextPtr viewportContext, const AZ::Name& newContextName) override;
            void EnumerateViewportContexts(AZStd::function<void(ViewportContextPtr)> visitorFunction) override;
            AZ::Name GetDefaultViewportContextName() const override;
            void PushViewGroup(const AZ::Name& contextName, ViewGroupPtr viewGroup) override;
            bool PopViewGroup(const AZ::Name& contextName, ViewGroupPtr viewGroup) override;
            ViewGroupPtr GetCurrentViewGroup(const AZ::Name& contextName) override;
            ViewportContextPtr GetDefaultViewportContext() const override;
            ViewportContextPtr GetViewportContextByScene(const Scene* scene) const override;

            ViewPtr GetCurrentView(const AZ::Name& context);
            ViewPtr GetCurrentStereoscopicView(const AZ::Name& context, ViewType viewType);
            
        private:
            void RegisterViewportContext(const AZ::Name& contextName, ViewportContextPtr viewportContext);
            void UnregisterViewportContext(AzFramework::ViewportId id);
            AzFramework::ViewportId GetViewportIdFromName(const AZ::Name& contextName) const;

            using ViewPtrStack = AZStd::deque<ViewGroupPtr>;
            struct ViewportContextData
            {
                AZStd::weak_ptr<ViewportContext> context;
                ViewportContext::SizeChangedEvent::Handler sizeChangedHandler;
                ViewportContext::ScalarChangedEvent::Handler dpiScalingChangedHandler;
            };

            // ViewportContextManager is a singleton owned solely by RPISystem, which is tagged as a friend
            ViewportContextManager();

            ViewPtrStack& GetOrCreateViewStackForContext(const AZ::Name& context);
            void UpdateViewForContext(const AZ::Name& context);

            AZStd::unordered_map<AzFramework::ViewportId, ViewportContextData> m_viewportContexts;
            AZStd::unordered_map<AZ::Name, ViewPtrStack> m_viewportViews;
            mutable AZStd::recursive_mutex m_containerMutex;
            AZStd::atomic<AzFramework::ViewportId> m_currentViewportId = StartingViewportId;

            AZ::Name m_defaultViewportContextName;

            friend class RPISystem;
            friend class ViewportContext;
        };
    } // namespace RPI
} // namespace AZ
