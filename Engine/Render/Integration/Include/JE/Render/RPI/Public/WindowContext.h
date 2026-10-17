/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Configuration.h>

#include <Atom/RHI/SwapChain.h>

#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI.Reflect/AttachmentId.h>
#include <JE/Render/RPI/Public/ViewProviderBus.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace JE::Render
{
    namespace RPI
    {
        //! An RPI-level understanding of a Window and its relevant SwapChain
        //! This is how the RPI creates a SwapChain from an AzFramework's Window
        //! and passes it down throughout the RPI. This is a convenient way to
        //! package the SwapChain, Window size, Attachment info and other
        //! window information needed to present to a surface.
        class JE_RENDER_INTEGRATION_API WindowContext final
            : public AzFramework::WindowNotificationBus::Handler
            , public AzFramework::ExclusiveFullScreenRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(WindowContext, AZ::SystemAllocator);

            WindowContext() = default;
            ~WindowContext() = default;

            //! Initializes the WindowContext from the given AzFramework's window
            //! param[in] windowHandle The native window handle of the Window we want to construct an RPI WindowContext for
            //! param[in] masterPassName The name of the pass that supplies input to the window's swapchain pass
            void Initialize(AZ::RHI::Device& device, AzFramework::NativeWindowHandle windowHandle);

            //! Gets the ViewportContexts associated with this WindowContext, if any.
            //! Associated ViewportContexts can be enumerated to find the active rendered scene.
            AZStd::vector<ViewportContextPtr> GetAssociatedViewportContexts();

            //! Registers a ViewportContext as associated with this WindowContext.
            void RegisterAssociatedViewportContext(ViewportContextPtr viewportContext);

            //! Shuts down the WindowContext and destroys the underlying SwapChain
            void Shutdown();

            //! Returns a unique attachment id associated with the swap chain.
            const AZ::RHI::AttachmentId& GetSwapChainAttachmentId(ViewType viewType = ViewType::Default) const;

            //! Retrieves the underlying SwapChain created by this WindowContext
            const AZ::RHI::Ptr<AZ::RHI::SwapChain>& GetSwapChain(ViewType viewType = ViewType::Default) const;

            //! Retrieves the default ViewportState for the WindowContext
            const AZ::RHI::Viewport& GetViewport(ViewType viewType = ViewType::Default) const;

            //! Retrieves the default ScissorState for the WindowContext
            const AZ::RHI::Scissor& GetScissor(ViewType viewType = ViewType::Default) const;

            //! Get the window ID for the WindowContext
            AzFramework::NativeWindowHandle GetWindowHandle() const { return m_windowHandle; }

            //! Get the number of active swapchains
            uint32_t GetSwapChainsSize() const;

            //! Get swapchain scaling mode
            AZ::RHI::Scaling GetSwapChainScalingMode() const;

        private:

            // WindowNotificationBus::Handler overrides ...
            void OnWindowResized(uint32_t width, uint32_t height) override;
            void OnResolutionChanged(uint32_t width, uint32_t height) override;
            void OnWindowClosed() override;
            void OnVsyncIntervalChanged(uint32_t interval) override;

            // ExclusiveFullScreenRequestBus::Handler overrides ...
            bool IsExclusiveFullScreenPreferred() const override;
            bool GetExclusiveFullScreenState() const override;
            bool SetExclusiveFullScreenState(bool fullScreenState) override;

            // Creates the underlying RHI level SwapChain for the given Window plus XR swapchains.
            void CreateSwapChains(AZ::RHI::Device& device);

            // Figure out swapchain's size based on window's client area size, render resolution and
            // device's swapchain scaling support
            AzFramework::WindowSize ResolveSwapchainSize();

            // Resize the swapchain if it's needed when window size or render resolution changed.
            // Return true if swapchain is resized. 
            bool CheckResizeSwapChain();

            // Destroys the underlying default SwapChain
            void DestroyDefaultSwapChain();

            // Destroys the underlying XR SwapChains
            void DestroyXRSwapChains();

            // Destroys swapChain data related to the provided index
            void DestroySwapChain(uint32_t swapChainIndex);

            // Fills the default window states based on the given width and height
            void FillWindowState(const uint32_t width, const uint32_t height);

            // Get the swapchain format to use
            AZ::RHI::Format GetSwapChainFormat(AZ::RHI::Device& device) const;

            // The handle of the window that this is context describes
            AzFramework::NativeWindowHandle m_windowHandle;

            struct SwapChainData
            {
                // RHI SwapChain object itself
                AZ::RHI::Ptr<AZ::RHI::SwapChain> m_swapChain;

                // The default viewport that covers the entire surface
                AZ::RHI::Viewport m_viewport;

                // The default scissor that covers the entire surface
                AZ::RHI::Scissor m_scissor;
            };
            // Data structure to hold SwapChain for Default and XR SwapChains.
            AZStd::vector<SwapChainData> m_swapChainsData;

            // The scaling mode used by the device swapchain.
            // If it supports stretch, the SwapChainPass in the render pipeline shouldn't need to do extra scaling
            AZ::RHI::Scaling m_swapChainScalingMode; 

            // Non-owning reference to associated ViewportContexts (if any)
            AZStd::vector<AZStd::weak_ptr<ViewportContext>> m_viewportContexts;
        };

        using WindowContextSharedPtr = AZStd::shared_ptr<WindowContext>;

    } // namespace RPI
} // namespace AZ
