/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Atom_RHI_Vulkan_Platform.h"
#include "Instance.h"

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/XRRenderingInterface.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/ILogger.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/Queue.h>
#include <RHI/RenderPass.h>
#include <RHI/SwapChain.h>
#include <RHI/ReleaseContainer.h>
#include <RHI/BinarySemaphore.h>
#include <RHI/PhysicalDevice.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        struct SwapChain::PresentationGeneration
        {
            struct ImageSync
            {
                RHI::Ptr<Semaphore> m_renderFinished;
                RHI::Ptr<Semaphore> m_ownershipTransferred;
                VkFence m_presentFence = VK_NULL_HANDLE;
                bool m_presentFencePending = false;
            };
            struct AcquireSync
            {
                VkFence m_fence = VK_NULL_HANDLE;
                bool m_pending = false;
            };

            Device* m_device = nullptr;
            CommandQueue* m_queue = nullptr;
            RHI::Ptr<WSISurface> m_surface;
            VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
            AZStd::vector<ImageSync> m_images;
            AZStd::vector<uint32_t> m_nativeImageIndices;
            AZStd::vector<AcquireSync> m_acquires;
            AZStd::mutex m_mutex;
            RHI::Ptr<Semaphore> m_unusedAcquireSemaphore;
            uint32_t m_firstPresentedImage = UINT32_MAX;
            VkFence m_firstPresentAcquireFence = VK_NULL_HANDLE;
            uint32_t m_queuedPresents = 0;
            bool m_maintenance1 = false;
            bool m_firstPresentComplete = false;
            bool m_completeBySuccessor = false;
            bool m_recreationRequested = false;
            bool m_deviceLost = false;
            bool m_surfaceLost = false;

            ~PresentationGeneration()
            {
                auto& context = m_device->GetContext();
                for (const auto& image : m_images)
                {
                    if (image.m_presentFence != VK_NULL_HANDLE)
                    {
                        context.DestroyFence(m_device->GetNativeDevice(), image.m_presentFence, VkSystemAllocator::Get());
                    }
                }
                for (const auto& acquire : m_acquires)
                {
                    if (acquire.m_fence != VK_NULL_HANDLE)
                    {
                        context.DestroyFence(m_device->GetNativeDevice(), acquire.m_fence, VkSystemAllocator::Get());
                    }
                }
                if (m_swapchain != VK_NULL_HANDLE)
                {
                    context.DestroySwapchainKHR(m_device->GetNativeDevice(), m_swapchain, VkSystemAllocator::Get());
                }
            }

            // Called under m_mutex. Acquire fences only witness acquisition; they do not retire GPU rendering.
            VkResult PollAcquires(bool wait)
            {
                for (auto& acquire : m_acquires)
                {
                    if (!acquire.m_pending)
                    {
                        continue;
                    }
                    VkResult result = wait ? m_device->GetContext().WaitForFences(
                        m_device->GetNativeDevice(), 1, &acquire.m_fence, VK_TRUE, UINT64_MAX) :
                        m_device->GetContext().GetFenceStatus(m_device->GetNativeDevice(), acquire.m_fence);
                    if (result == VK_NOT_READY)
                    {
                        continue;
                    }
                    if (result != VK_SUCCESS)
                    {
                        m_deviceLost = result == VK_ERROR_DEVICE_LOST;
                        return result;
                    }
                    if (acquire.m_fence == m_firstPresentAcquireFence)
                    {
                        m_firstPresentComplete = true;
                    }
                    acquire.m_pending = false;
                }
                return VK_SUCCESS;
            }
        };

        void SwapChain::FlushPresentationWork()
        {
            // Present() itself runs on a command-queue worker. This barrier belongs only to the
            // caller's frame boundary/resize/shutdown, never to PresentInternal or AcquireNewImage.
            auto& queues = static_cast<Device&>(GetDevice()).GetCommandQueueContext();
            for (uint32_t index = 0; index < static_cast<uint32_t>(RHI::HardwareQueueClass::Count); ++index)
            {
                queues.GetCommandQueue(static_cast<RHI::HardwareQueueClass>(index)).FlushCommands();
            }
            if (m_presentationQueue)
            {
                m_presentationQueue->FlushCommands();
            }
            for (const auto& generation : m_retiredGenerations)
            {
                generation->m_queue->FlushCommands();
            }
        }

        void SwapChain::RetireCurrentGeneration()
        {
            if (!m_presentationGeneration)
            {
                return;
            }
            if (m_currentFrameContext.m_imageAvailableSemaphore)
            {
                // An acquired image might never reach a compiled scope during recreation. Keep the wrapper alive
                // until its acquire fence and GPU work have completed; do not recycle a possibly signaled binary.
                auto* semaphore = m_currentFrameContext.m_imageAvailableSemaphore;
                semaphore->SetRecycleValue(false);
                m_presentationGeneration->m_unusedAcquireSemaphore = semaphore;
                static_cast<Device&>(GetDevice()).GetSwapChainSemaphoreAllocator().DeAllocate(semaphore);
            }
            m_currentFrameContext = {};
            m_swapChainBarrier = {};
            m_retiredGenerations.push_back(AZStd::move(m_presentationGeneration));
        }

        void SwapChain::CollectRetiredGenerations(bool waitForCompletion)
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (waitForCompletion)
            {
                for (const auto& generation : m_retiredGenerations)
                {
                    generation->m_queue->FlushCommands();
                }
                device.GetCommandQueueContext().WaitForIdle();
            }
            // A completed first presentation of a newer generation proves earlier presentations on that
            // same presentation queue have passed. Preserve the proof across chains of rapid recreations.
            auto propagate = [&](const AZStd::shared_ptr<PresentationGeneration>& successor)
            {
                AZStd::lock_guard<AZStd::mutex> successorLock(successor->m_mutex);
                successor->PollAcquires(waitForCompletion);
                if (!successor->m_firstPresentComplete)
                {
                    return;
                }
                for (const auto& previous : m_retiredGenerations)
                {
                    if (previous == successor)
                    {
                        break;
                    }
                    if (previous->m_queue == successor->m_queue && previous->m_surface == successor->m_surface)
                    {
                        previous->m_completeBySuccessor = true;
                    }
                }
            };
            for (const auto& generation : m_retiredGenerations)
            {
                propagate(generation);
            }
            if (m_presentationGeneration)
            {
                propagate(m_presentationGeneration);
            }
            bool gpuIdle = waitForCompletion;
            for (auto iterator = m_retiredGenerations.begin(); iterator != m_retiredGenerations.end();)
            {
                auto& generation = **iterator;
                AZStd::unique_lock<AZStd::mutex> lock(generation.m_mutex);
                bool ready = generation.m_queuedPresents == 0;
                generation.PollAcquires(waitForCompletion);
                for (const auto& acquire : generation.m_acquires)
                {
                    ready &= !acquire.m_pending;
                }
                if (generation.m_maintenance1)
                {
                    for (const auto& image : generation.m_images)
                    {
                        if (image.m_presentFencePending)
                        {
                            VkResult result = waitForCompletion ? device.GetContext().WaitForFences(
                                device.GetNativeDevice(), 1, &image.m_presentFence, VK_TRUE, UINT64_MAX) :
                                device.GetContext().GetFenceStatus(device.GetNativeDevice(), image.m_presentFence);
                            ready &= result == VK_SUCCESS;
                            generation.m_deviceLost |= result == VK_ERROR_DEVICE_LOST;
                        }
                    }
                }
                else
                {
                    // Final shutdown without maintenance1 has no portable present-completion primitive.
                    // Preserve the established WaitIdle teardown fallback; do not call it a formal proof.
                    ready &= waitForCompletion || generation.m_completeBySuccessor ||
                        generation.m_firstPresentedImage == UINT32_MAX;
                }
                ready |= generation.m_deviceLost;
                lock.unlock();
                if (ready)
                {
                    // Present/acquire evidence alone does not cover unrelated GPU uses of the old image.
                    if (!gpuIdle)
                    {
                        // A real retirement is ready. Drain all CPU queue workers before the device-wide
                        // GPU idle, including a concurrent newer generation's native QueuePresent call.
                        FlushPresentationWork();
                        device.GetCommandQueueContext().WaitForIdle();
                        gpuIdle = true;
                    }
                    iterator = m_retiredGenerations.erase(iterator);
                }
                else
                {
                    ++iterator;
                }
            }
        }
        static bool IsDefaultSwapChainNeeded()
        {
            auto* xrSystem = RHI::RHISystemInterface::Get()->GetXRSystem();
            return !xrSystem || xrSystem->IsDefaultRenderPipelineNeeded();
        }

        RHI::Ptr<SwapChain> SwapChain::Create()
        {
            return aznew SwapChain();
        }

        VkSwapchainKHR SwapChain::GetNativeSwapChain() const
        {
            return m_nativeSwapChain;
        }

        const SwapChain::FrameContext& SwapChain::GetCurrentFrameContext() const
        {
             return m_currentFrameContext;
        }

        const WSISurface& SwapChain::GetSurface() const
        {
            return *m_surface;
        }

        const CommandQueue& SwapChain::GetPresentationQueue() const
        {
            return *m_presentationQueue;
        }

        void SwapChain::QueueBarrier(const VkPipelineStageFlags src, const VkPipelineStageFlags dst, const VkImageMemoryBarrier& imageBarrier)
        {
            m_swapChainBarrier.m_barrier = imageBarrier;
            m_swapChainBarrier.m_srcPipelineStages = src;
            m_swapChainBarrier.m_dstPipelineStages = dst;
            m_swapChainBarrier.m_isValid = true;
        }

        bool SwapChain::ProcessRecreation()
        {
            if (GetDescriptor().m_isXrSwapChain)
            {
                return false;
            }
            auto updateRecreationRequests = [this]()
            {
                if (m_presentationGeneration)
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_presentationGeneration->m_mutex);
                    m_pendingRecreation |= m_presentationGeneration->m_recreationRequested;
                    m_surfaceNeedsRebuild |= m_presentationGeneration->m_surfaceLost;
                    if (m_presentationGeneration->m_deviceLost)
                    {
                        AZ_Error("Vulkan", false, "Cannot recreate swapchain on a lost device.");
                        return false;
                    }
                }
                return true;
            };
            if (!updateRecreationRequests())
            {
                return false;
            }
            if (!m_pendingRecreation && !m_surfaceNeedsRebuild)
            {
                if (!m_retiredGenerations.empty())
                {
                    // Poll only. CollectRetiredGenerations drains queues only when a retirement is ready.
                    CollectRetiredGenerations();
                }
                return false;
            }
            // Generation replacement is main/caller-owned. A worker may finish or report another error
            // during this drain, so capture its protected requests again before mutating the generation.
            FlushPresentationWork();
            if (!updateRecreationRequests())
            {
                return false;
            }
            CollectRetiredGenerations();
            if (m_surfaceNeedsRebuild)
            {
                // No further acquisition can witness the old surface. Retire it as a terminal WSI
                // boundary: present fences when available, documented WaitIdle fallback otherwise.
                ShutdownImages();
                RetireCurrentGeneration();
                m_nativeSwapChain = VK_NULL_HANDLE;
                CollectRetiredGenerations(true);
                if (!m_retiredGenerations.empty() || BuildSurface(GetDescriptor()) != RHI::ResultCode::Success)
                {
                    return false;
                }
                m_surfaceNeedsRebuild = false;
            }
            if (m_pendingRecreation)
            {
                // Base::Resize updates its image count/descriptor and owns ShutdownImages/InitImages.
                if (Base::Resize(m_dimensions) != RHI::ResultCode::Success)
                {
                    return false;
                }
                m_pendingRecreation = false;
                return true;
            }
            return false;
        }

        void SwapChain::SetVerticalSyncIntervalInternal(uint32_t previousVsyncInterval)
        {
            if (GetDescriptor().m_verticalSyncInterval == 0 || previousVsyncInterval == 0)
            {
                // The presentation mode may change when transitioning to or from a vsynced presentation mode
                // In this case, the swapchain must be recreated.
                m_pendingRecreation = true;
            }
        }

        void SwapChain::SetNameInternal([[maybe_unused]] const AZStd::string_view& name)
        {
            // On some GPUs, like the Adreno 740, setting the name of the swapchain causes a crash, so we don't do it.
        }

        RHI::ResultCode SwapChain::InitInternal(RHI::Device& baseDevice, const RHI::SwapChainDescriptor& descriptor, RHI::SwapChainDimensions* nativeDimensions)
        {
            RHI::ResultCode result = RHI::ResultCode::Success;
            RHI::DeviceObject::Init(baseDevice);

            auto& device = static_cast<Device&>(GetDevice());
            m_dimensions = descriptor.m_dimensions;

            if (descriptor.m_isXrSwapChain)
            {
                if (nativeDimensions)
                {
                    *nativeDimensions = m_dimensions;
                }
            }
            else
            {
                result = BuildSurface(descriptor);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);

                auto& presentationQueue = device.GetCommandQueueContext().GetOrCreatePresentationCommandQueue(*this);
                m_presentationQueue = &presentationQueue;

                if (IsDefaultSwapChainNeeded())
                {
                    result = CreateSwapchain();
                    RETURN_RESULT_IF_UNSUCCESSFUL(result);
                }

                if (nativeDimensions)
                {
                    // Fill out the real swapchain dimensions to return
                    *nativeDimensions = m_dimensions;
                    nativeDimensions->m_imageFormat = ConvertFormat(m_surfaceFormat.format);
                }
            }

            SetName(GetName());
            return result;
        }

        void SwapChain::ShutdownInternal()
        {
            if (GetDescriptor().m_isXrSwapChain)
            {
                return;
            }
            FlushPresentationWork();
            RetireCurrentGeneration();
            CollectRetiredGenerations(true);
            AZ_Assert(m_retiredGenerations.empty(), "Presentation generation did not finish retiring.");
            m_nativeSwapChain = VK_NULL_HANDLE;
            InvalidateSurface();
            m_presentationQueue = nullptr;
            m_swapchainNativeImages.clear();
        }

        RHI::ResultCode SwapChain::InitImageInternal(const RHI::DeviceSwapChain::InitImageRequest& request)
        {
            auto& device = static_cast<Device&>(GetDevice());
            Image* image = static_cast<Image*>(request.m_image);
            RHI::ImageDescriptor imageDesc = request.m_descriptor;
            RHI::ResultCode result = RHI::ResultCode::Success;

            // XR swapchains will retrieve the native swapchain image from xr system where as non-xr
            // swapchains will use the images created internally (i.e RHI::Vulkan)
            if (GetDescriptor().m_isXrSwapChain)
            {
                XRSwapChainDescriptor xrSwapChainDescriptor;
                xrSwapChainDescriptor.m_inputData.m_swapChainIndex = GetDescriptor().m_xrSwapChainIndex;
                xrSwapChainDescriptor.m_inputData.m_swapChainImageIndex = request.m_imageIndex;

                result = GetXRSystem()->GetSwapChainImage(&xrSwapChainDescriptor);
                AZ_Assert(result == RHI::ResultCode::Success, "Xr Session creation was not successful");

                result = image->Init(device, xrSwapChainDescriptor.m_outputData.m_nativeImage, imageDesc);
            }
            else
            {
                if (IsDefaultSwapChainNeeded())
                {
                    imageDesc.m_format = ConvertFormat(m_surfaceFormat.format);
                    result = image->Init(device, m_swapchainNativeImages[request.m_imageIndex], imageDesc);
                }
            }

            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to initialize swapchain image %d", request.m_imageIndex);
                return result;
            }

            Name name(AZStd::string::format("SwapChainImage_%d", request.m_imageIndex));
            image->SetName(name);

            return result;
        }

        RHI::ResultCode SwapChain::ResizeInternal(const RHI::SwapChainDimensions& dimensions, RHI::SwapChainDimensions* nativeDimensions)
        {
            FlushPresentationWork();
            auto& device = static_cast<Device&>(GetDevice());
            m_dimensions = dimensions;

            auto& presentationQueue = device.GetCommandQueueContext().GetOrCreatePresentationCommandQueue(*this);
            m_presentationQueue = &presentationQueue;

            RHI::ResultCode result = CreateSwapchain();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            if (nativeDimensions)
            {
                *nativeDimensions = m_dimensions;
                // [ATOM-4840] This is a workaround when the windows is minimized (0x0 size).
                // Add proper support to handle this case.
                nativeDimensions->m_imageHeight = AZStd::max(m_dimensions.m_imageHeight, 1u);
                nativeDimensions->m_imageWidth = AZStd::max(m_dimensions.m_imageWidth, 1u);

                nativeDimensions->m_imageFormat = ConvertFormat(m_surfaceFormat.format);
            }

            CollectRetiredGenerations();

            return RHI::ResultCode::Success;
        }

        uint32_t SwapChain::PresentInternal()
        {
            if (GetDescriptor().m_isXrSwapChain)
            {
                return 0;
            }
            auto& device = static_cast<Device&>(GetDevice());
            auto generation = m_presentationGeneration;
            AZ_Assert(generation, "No acquired presentation generation.");
            if (!generation)
            {
                return 0;
            }
            const uint32_t imageIndex = generation->m_nativeImageIndices[GetCurrentImageIndex()];
            const auto barrier = m_swapChainBarrier;
            m_swapChainBarrier.m_isValid = false;
            {
                AZStd::lock_guard<AZStd::mutex> lock(generation->m_mutex);
                ++generation->m_queuedPresents;
            }
            auto presentCommand = [generation, imageIndex, barrier, &device](void* queue)
            {
                // No pointer to SwapChain: queued work keeps its immutable native generation alive.
                AZStd::lock_guard<AZStd::mutex> lock(generation->m_mutex);
                auto& image = generation->m_images[imageIndex];
                auto* vulkanQueue = static_cast<Queue*>(queue);
                if (image.m_presentFencePending)
                {
                    VkResult result = device.GetContext().WaitForFences(
                        device.GetNativeDevice(), 1, &image.m_presentFence, VK_TRUE, UINT64_MAX);
                    if (result != VK_SUCCESS)
                    {
                        generation->m_deviceLost = result == VK_ERROR_DEVICE_LOST;
                        generation->m_recreationRequested = true;
                        --generation->m_queuedPresents;
                        VK_RESULT_ASSERT(result);
                        return;
                    }
                    image.m_presentFencePending = false;
                }
                if (generation->m_maintenance1)
                {
                    VkResult result = device.GetContext().ResetFences(device.GetNativeDevice(), 1, &image.m_presentFence);
                    if (result != VK_SUCCESS)
                    {
                        generation->m_deviceLost = result == VK_ERROR_DEVICE_LOST;
                        generation->m_recreationRequested = true;
                        --generation->m_queuedPresents;
                        VK_RESULT_ASSERT(result);
                        return;
                    }
                }
                VkSemaphore waitSemaphore = image.m_renderFinished->GetNativeSemaphore();
                if (barrier.m_isValid)
                {
                    auto commandList = device.AcquireCommandList(vulkanQueue->GetId().m_familyIndex);
                    commandList->BeginCommandBuffer();
                    device.GetContext().CmdPipelineBarrier(
                        commandList->GetNativeCommandBuffer(), barrier.m_srcPipelineStages, barrier.m_dstPipelineStages,
                        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier.m_barrier);
                    commandList->EndCommandBuffer();
                    vulkanQueue->SubmitCommandBuffers(
                        AZStd::vector<RHI::Ptr<CommandList>>{commandList},
                        AZStd::vector<Semaphore::WaitSemaphore>{AZStd::make_pair(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, image.m_renderFinished)},
                        AZStd::vector<RHI::Ptr<Semaphore>>{image.m_ownershipTransferred}, {}, nullptr);
                    image.m_ownershipTransferred->SignalEvent();
                    waitSemaphore = image.m_ownershipTransferred->GetNativeSemaphore();
                }
                VkSwapchainPresentFenceInfoEXT presentFence{};
                presentFence.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
                presentFence.swapchainCount = 1;
                presentFence.pFences = &image.m_presentFence;
                VkPresentInfoKHR info{};
                info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                info.pNext = generation->m_maintenance1 ? &presentFence : nullptr;
                info.waitSemaphoreCount = 1;
                info.pWaitSemaphores = &waitSemaphore;
                info.swapchainCount = 1;
                info.pSwapchains = &generation->m_swapchain;
                info.pImageIndices = &imageIndex;
                const VkResult result = device.GetContext().QueuePresentKHR(vulkanQueue->GetNativeQueue(), &info);
                // These rejected presentations still enqueue their semaphore waits. OOM does not.
                const bool enqueued = result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR ||
                    result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_SURFACE_LOST_KHR ||
                    result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT;
                image.m_presentFencePending = generation->m_maintenance1 && enqueued;
                generation->m_surfaceLost |= result == VK_ERROR_SURFACE_LOST_KHR;
                if (enqueued && generation->m_firstPresentedImage == UINT32_MAX)
                {
                    generation->m_firstPresentedImage = imageIndex;
                }
                if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_SURFACE_LOST_KHR ||
                    result == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT ||
                    result == VK_ERROR_OUT_OF_HOST_MEMORY || result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
                {
                    // Retain the generation; an unconsumed signal is never returned to a reusable pool.
                    generation->m_recreationRequested = true;
                }
                else if (result == VK_SUBOPTIMAL_KHR)
                {
#if AZ_TRAIT_ATOM_VULKAN_RECREATE_SWAPCHAIN_WHEN_SUBOPTIMAL
                    generation->m_recreationRequested = true;
#endif
                }
                else if (result != VK_SUCCESS)
                {
                    generation->m_deviceLost = result == VK_ERROR_DEVICE_LOST;
                    generation->m_recreationRequested = true;
                    AZ_Assert(false, "Unhandled swapchain presentation result %d.", result);
                }
                --generation->m_queuedPresents;
            };
            // Always enqueue this acquired/rendered image, even if acquiring the next image fails.
            generation->m_queue->QueueCommand(AZStd::move(presentCommand));
            uint32_t acquiredImageIndex = GetCurrentImageIndex();
            RHI::ResultCode result = AcquireNewImage(&acquiredImageIndex);
            if (result != RHI::ResultCode::Success)
            {
                AZStd::lock_guard<AZStd::mutex> lock(generation->m_mutex);
                generation->m_recreationRequested = true;
                return 0;
            }
            return acquiredImageIndex;
        }

        void SwapChain::SetHDRMetaData(float maxOutputNits, float minOutputNits, float maxContentLightLevel,
            float maxFrameAverageLightLevel)
        {
            auto& device = static_cast<Device&>(GetDevice());

            //From DX12 RHI
            struct DisplayChromacities
            {
                float RedX;
                float RedY;
                float GreenX;
                float GreenY;
                float BlueX;
                float BlueY;
                float WhiteX;
                float WhiteY;
            };
            static const DisplayChromacities DisplayChromacityList[] =
            {
                { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Display Gamut Rec709
                { 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Display Gamut Rec2020
            };

            // Select the chromaticity based on HDR format
            int selectedChroma = 0;
            if (m_colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)
            {
                selectedChroma = 0;
            }
            else if (m_colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
            {
                selectedChroma = 1;
            }
            else
            {
                AZ_Assert(false, "Unhandled color space for swapchain.");
            }

            const DisplayChromacities& Chroma = DisplayChromacityList[selectedChroma];
            VkHdrMetadataEXT hdrMetadata = {};
            hdrMetadata.sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
            //DX12 RHI scales these by 50000 but VKD3D removes that so just use the raw values.
            hdrMetadata.displayPrimaryRed = {Chroma.RedX, Chroma.RedY};
            hdrMetadata.displayPrimaryGreen = {Chroma.GreenX, Chroma.GreenY};
            hdrMetadata.displayPrimaryBlue = {Chroma.BlueX, Chroma.BlueY};
            hdrMetadata.whitePoint = {Chroma.WhiteX, Chroma.WhiteY};
            //Mult. by 10,000 because its what VKD3D does, you can look at convert_max_luminance and convert_min_luminance for reference.
            //Since the DX12 RHI scales these by 10000 we should too but VKD3D downscales by the same amount for minLuminance.
            //https://github.com/HansKristian-Work/vkd3d-proton/blob/8165096180a9019542b693ce1fcb80f44433b1e2/libs/vkd3d/swapchain.c#L866C36-L866C57
            hdrMetadata.maxLuminance = maxOutputNits * 10000.0f;
            hdrMetadata.minLuminance = minOutputNits;
            hdrMetadata.maxContentLightLevel = maxContentLightLevel;
            hdrMetadata.maxFrameAverageLightLevel = maxFrameAverageLightLevel;

            AZ_Assert(device.GetContext().SetHdrMetadataEXT != nullptr, "Calling SetHDRMetaData when HDR isn't supported.");
            device.GetContext().SetHdrMetadataEXT(device.GetNativeDevice(), 1, &m_nativeSwapChain, &hdrMetadata);
        }

        RHI::ResultCode SwapChain::BuildSurface(const RHI::SwapChainDescriptor& descriptor)
        {
            WSISurface::Descriptor surfaceDesc{};
            surfaceDesc.m_windowHandle = descriptor.m_window;
            RHI::Ptr<WSISurface> surface = WSISurface::Create();
            const RHI::ResultCode result = surface->Init(surfaceDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_surface = surface;

            return result;
        }

        bool SwapChain::ValidateSurfaceDimensions(const RHI::SwapChainDimensions& dimensions)
        {
            return (m_surfaceCapabilities.minImageExtent.width <= dimensions.m_imageWidth &&
                dimensions.m_imageWidth <= m_surfaceCapabilities.maxImageExtent.width &&
                m_surfaceCapabilities.minImageExtent.height <= dimensions.m_imageHeight &&
                dimensions.m_imageHeight <= m_surfaceCapabilities.maxImageExtent.height);
        }

        VkSurfaceFormatKHR SwapChain::GetSupportedSurfaceFormat(const RHI::Format rhiFormat, const VkColorSpaceKHR preferredColorSpace) const
        {
            AZ_Assert(m_surface, "Surface has not been initialized.");
            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            uint32_t surfaceFormatCount = 0;
            [[maybe_unused]] VkResult vkResult = device.GetContext().GetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &surfaceFormatCount, nullptr);
            VK_RESULT_ASSERT(vkResult);

            AZ_Assert(surfaceFormatCount > 0, "Surface support no format.");
            AZStd::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);

            vkResult = device.GetContext().GetPhysicalDeviceSurfaceFormatsKHR(
                    physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &surfaceFormatCount, surfaceFormats.data());
            VK_RESULT_ASSERT(vkResult);

            const VkFormat format = ConvertFormat(rhiFormat);
            VkSurfaceFormatKHR matchedFormat = {};
            matchedFormat.format = VK_FORMAT_UNDEFINED;
            for (uint32_t index = 0; index < surfaceFormatCount; ++index)
            {
                if (surfaceFormats[index].format == format)
                {
                    if (surfaceFormats[index].colorSpace == preferredColorSpace)
                    {
                        return surfaceFormats[index];
                    }

                    if (matchedFormat.format == VK_FORMAT_UNDEFINED)
                    {
                        matchedFormat = surfaceFormats[index];
                    }
                }
            }
            if (matchedFormat.format != VK_FORMAT_UNDEFINED)
            {
                return matchedFormat;
            }
            AZ_Warning("Vulkan", false, "Given format is not supported, so it uses a supported format.");
            return surfaceFormats[0];
        }

        VkPresentModeKHR SwapChain::GetSupportedPresentMode(uint32_t verticalSyncInterval) const
        {
            AZ_Assert(m_surface, "Surface has not been initialized.");

            AZStd::vector<VkPresentModeKHR> preferredModes;
            if (verticalSyncInterval == 0)
            {
                // No vsync, so we try to use the immediate mode.
                preferredModes.push_back(VK_PRESENT_MODE_IMMEDIATE_KHR);
                // If immediate mode is not available we try for mailbox, which technically is still vsync
                // but doesn't block the CPU when queue is full.
                preferredModes.push_back(VK_PRESENT_MODE_MAILBOX_KHR);
            }
            preferredModes.push_back(VK_PRESENT_MODE_FIFO_KHR);
            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());

            uint32_t modeCount = 0;
            [[maybe_unused]] VkResult vkResult = device.GetContext().GetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &modeCount, nullptr);
            VK_RESULT_ASSERT(vkResult);
            // VK_PRESENT_MODE_FIFO_KHR has to be supported.
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPresentModeKHR.html
            AZ_Assert(modeCount > 0, "no available present mode.");
            AZStd::vector<VkPresentModeKHR> supportedModes(modeCount);
            vkResult = device.GetContext().GetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &modeCount, supportedModes.data());
            VK_RESULT_ASSERT(vkResult);

            for (VkPresentModeKHR preferredMode : preferredModes)
            {
                for (VkPresentModeKHR supportedMode : supportedModes)
                {
                    if (supportedMode == preferredMode)
                    {
                        return supportedMode;
                    }
                }
            }
            return supportedModes[0];
        }

        VkSurfaceCapabilitiesKHR SwapChain::GetSurfaceCapabilities()
        {
            AZ_Assert(m_surface, "Surface has not been initialized.");

            auto& device = static_cast<Device&>(GetDevice());
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());

            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            [[maybe_unused]] VkResult vkResult = device.GetContext().GetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevice.GetNativePhysicalDevice(), m_surface->GetNativeSurface(), &surfaceCapabilities);
            VK_RESULT_ASSERT(vkResult);

            return surfaceCapabilities;
        }

        VkCompositeAlphaFlagBitsKHR SwapChain::GetSupportedCompositeAlpha() const
        {
            VkFlags supportedModesBits = m_surfaceCapabilities.supportedCompositeAlpha;
            VkCompositeAlphaFlagBitsKHR preferedModes[] = {
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR };

            for (VkCompositeAlphaFlagBitsKHR mode : preferedModes)
            {
                if (supportedModesBits & mode)
                {
                    return mode;
                }
            }

            AZ_Assert(false, "Could not find a supported composite alpha mode for the swapchain");
            return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }

        RHI::ResultCode SwapChain::BuildNativeSwapChain(const RHI::SwapChainDimensions& dimensions)
        {
            AZ_Assert(m_surface, "Surface is null.");

            if (!ValidateSurfaceDimensions(dimensions))
            {
                AZ_Assert(false, "Swapchain dimensions are not supported.");
                return RHI::ResultCode::InvalidArgument;
            }

            auto& device = static_cast<Vulkan::Device&>(GetDevice());
            auto& queueContext = device.GetCommandQueueContext();
            const VkExtent2D extent = { dimensions.m_imageWidth, dimensions.m_imageHeight };

            // If the graphic queue is the same as the presentation queue, then we will always acquire
            // 1 image at the same time. If it's another queue, we will have 2 at the same time (while the other queue
            // presents the image)
            auto graphicQueueId = queueContext.GetCommandQueue(RHI::HardwareQueueClass::Graphics).GetId();
            auto presentationQueueId = m_presentationQueue->GetId();
            AZStd::vector<uint32_t> familyIndices{ graphicQueueId.m_familyIndex };
            uint32_t simultaneousAcquiredImages = 1;
            if (graphicQueueId != presentationQueueId)
            {
                simultaneousAcquiredImages = 2;
                if (presentationQueueId.m_familyIndex != graphicQueueId.m_familyIndex)
                {
                    familyIndices.push_back(presentationQueueId.m_familyIndex);
                }
            }

            bool hdrEnabled = false;

            if (m_surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                m_surfaceFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
            {
                hdrEnabled = true;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;
            createInfo.flags = 0; // [GFX TODO][ATOM-512] find appropriate flags
            createInfo.surface = m_surface->GetNativeSurface();
            // When acquiring an image the number of images that the application has currently acquired (but not yet presented)
            // need to be less than or equal to the difference between the number of images in swapchain and the value of VkSurfaceCapabilitiesKHR::minImageCount
            createInfo.minImageCount = AZStd::max(dimensions.m_imageCount, simultaneousAcquiredImages + m_surfaceCapabilities.minImageCount);
            createInfo.imageFormat = m_surfaceFormat.format;
            createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1; // non-stereoscopic
            createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            createInfo.imageUsage = RHI::FilterBits(createInfo.imageUsage, m_surfaceCapabilities.supportedUsageFlags);
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = aznumeric_cast<uint32_t>(familyIndices.size());
            createInfo.pQueueFamilyIndices = familyIndices.empty() ? nullptr : familyIndices.data();
            createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            createInfo.compositeAlpha = m_compositeAlphaFlagBits;
            createInfo.presentMode = m_presentMode;
            createInfo.clipped = VK_FALSE;
            // Pass the current swapchain as the old one
            createInfo.oldSwapchain = m_nativeSwapChain;

            VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
            const VkResult result =
                device.GetContext().CreateSwapchainKHR(device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &newSwapchain);
            VK_RESULT_ASSERT(result);
            if (result != VK_SUCCESS)
            {
                // Passing oldSwapchain retires it even when creating the replacement fails.
                RetireCurrentGeneration();
                m_nativeSwapChain = VK_NULL_HANDLE;
                m_pendingRecreation = true;
                return ConvertResult(result);
            }
            m_nativeSwapChain = newSwapchain;

            if(hdrEnabled)
            {
                m_colorSpace = createInfo.imageColorSpace;
                float maxOutputNits = 1000.f;
                float minOutputNits = 0.001f;
                float maxContentLightLevelNits = 2000.f;
                float maxFrameAverageLightLevelNits = 500.f;
                SetHDRMetaData(maxOutputNits, minOutputNits, maxContentLightLevelNits, maxFrameAverageLightLevelNits);
            }

            return ConvertResult(result);
        }

        RHI::ResultCode SwapChain::AcquireNewImage(uint32_t* acquiredImageIndex)
        {
            auto generation = m_presentationGeneration;
            if (!generation)
            {
                return RHI::ResultCode::Fail;
            }
            auto& device = static_cast<Device&>(GetDevice());
            VkFence acquireFence = VK_NULL_HANDLE;
            {
                AZStd::unique_lock<AZStd::mutex> lock(generation->m_mutex);
                VkResult result = generation->PollAcquires(false);
                if (result != VK_SUCCESS)
                {
                    return ConvertResult(result);
                }
                auto findFreeFence = [&]()
                {
                    for (const auto& acquire : generation->m_acquires)
                    {
                        if (!acquire.m_pending)
                        {
                            return acquire.m_fence;
                        }
                    }
                    return VkFence{VK_NULL_HANDLE};
                };
                acquireFence = findFreeFence();
                if (acquireFence == VK_NULL_HANDLE)
                {
                    // Storage is allocated once per generation, not on the render loop. Never wait while
                    // holding the generation mutex: the presentation worker may be needed for progress.
                    VkFence pendingFence = generation->m_acquires.front().m_fence;
                    lock.unlock();
                    result = device.GetContext().WaitForFences(device.GetNativeDevice(), 1, &pendingFence, VK_TRUE, UINT64_MAX);
                    lock.lock();
                    if (result != VK_SUCCESS)
                    {
                        generation->m_deviceLost = result == VK_ERROR_DEVICE_LOST;
                        return ConvertResult(result);
                    }
                    result = generation->PollAcquires(false);
                    if (result != VK_SUCCESS)
                    {
                        return ConvertResult(result);
                    }
                    acquireFence = findFreeFence();
                }
                AZ_Assert(acquireFence != VK_NULL_HANDLE, "No reusable acquire fence after completion.");
                if (acquireFence == VK_NULL_HANDLE)
                {
                    return RHI::ResultCode::Fail;
                }
                result = device.GetContext().ResetFences(device.GetNativeDevice(), 1, &acquireFence);
                if (result != VK_SUCCESS)
                {
                    generation->m_deviceLost = result == VK_ERROR_DEVICE_LOST;
                    return ConvertResult(result);
                }
            }
            auto& allocator = device.GetSwapChainSemaphoreAllocator();
            Semaphore* imageAvailable = allocator.Allocate();
            if (!imageAvailable)
            {
                return RHI::ResultCode::OutOfMemory;
            }
            // Do not hold m_mutex over acquire: a queued presentation must be able to release an image.
            uint32_t nativeImageIndex = 0;
            const VkResult result = device.GetContext().AcquireNextImageKHR(
                device.GetNativeDevice(), generation->m_swapchain, UINT64_MAX,
                imageAvailable->GetNativeSemaphore(), acquireFence, &nativeImageIndex);
            if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                imageAvailable->SetRecycleValue(false);
                allocator.DeAllocate(imageAvailable);
                AZStd::lock_guard<AZStd::mutex> lock(generation->m_mutex);
                generation->m_deviceLost = result == VK_ERROR_DEVICE_LOST;
                generation->m_surfaceLost |= result == VK_ERROR_SURFACE_LOST_KHR;
                generation->m_recreationRequested = true;
                return RHI::ResultCode::Fail;
            }
            {
                AZStd::lock_guard<AZStd::mutex> lock(generation->m_mutex);
                for (auto& acquire : generation->m_acquires)
                {
                    if (acquire.m_fence == acquireFence)
                    {
                        acquire.m_pending = true;
                        break;
                    }
                }
                AZ_Assert(nativeImageIndex < generation->m_images.size(), "Invalid acquired image index.");
                if (nativeImageIndex >= generation->m_images.size())
                {
                    imageAvailable->SetRecycleValue(false);
                    generation->m_unusedAcquireSemaphore = imageAvailable;
                    allocator.DeAllocate(imageAvailable);
                    return RHI::ResultCode::Fail;
                }
                if (!generation->m_firstPresentComplete && generation->m_firstPresentAcquireFence == VK_NULL_HANDLE &&
                    generation->m_firstPresentedImage == nativeImageIndex)
                {
                    generation->m_firstPresentAcquireFence = acquireFence;
                }
                generation->m_images[nativeImageIndex].m_renderFinished->Reset();
                generation->m_images[nativeImageIndex].m_ownershipTransferred->Reset();
#if AZ_TRAIT_ATOM_VULKAN_RECREATE_SWAPCHAIN_WHEN_SUBOPTIMAL
                generation->m_recreationRequested |= result == VK_SUBOPTIMAL_KHR;
#endif
            }
            if (m_currentFrameContext.m_imageAvailableSemaphore)
            {
                allocator.DeAllocate(m_currentFrameContext.m_imageAvailableSemaphore);
            }
            m_currentFrameContext.m_imageAvailableSemaphore = imageAvailable;
            m_currentFrameContext.m_presentableSemaphore = generation->m_images[nativeImageIndex].m_renderFinished.get();
            auto image = AZStd::find(generation->m_nativeImageIndices.begin(), generation->m_nativeImageIndices.end(), nativeImageIndex);
            *acquiredImageIndex = aznumeric_cast<uint32_t>(image - generation->m_nativeImageIndices.begin());
            return RHI::ResultCode::Success;
        }
        void SwapChain::InvalidateSurface()
        {
            m_surface = nullptr;
        }

        RHI::ResultCode SwapChain::CreateSwapchain()
        {
            auto& device = static_cast<Device&>(GetDevice());

            VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            if (r_hdrOutput && m_dimensions.m_imageFormat == RHI::Format::R10G10B10A2_UNORM)
            {
                preferredColorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
            }

            m_surfaceCapabilities = GetSurfaceCapabilities();
            m_surfaceFormat = GetSupportedSurfaceFormat(m_dimensions.m_imageFormat, preferredColorSpace);
            m_presentMode = GetSupportedPresentMode(GetDescriptor().m_verticalSyncInterval);
            m_compositeAlphaFlagBits = GetSupportedCompositeAlpha();

            if (!ValidateSurfaceDimensions(m_dimensions))
            {
                uint32_t oldHeight = m_dimensions.m_imageHeight;
                uint32_t oldWidth = m_dimensions.m_imageWidth;
                m_dimensions.m_imageHeight = AZStd::clamp(
                    m_dimensions.m_imageHeight,
                    m_surfaceCapabilities.minImageExtent.height,
                    m_surfaceCapabilities.maxImageExtent.height);
                m_dimensions.m_imageWidth = AZStd::clamp(
                    m_dimensions.m_imageWidth,
                    m_surfaceCapabilities.minImageExtent.width,
                    m_surfaceCapabilities.maxImageExtent.width);
                AZLOG_DEBUG("Resizing swapchain from (%u, %u) to (%u, %u).",
                    oldWidth, oldHeight, m_dimensions.m_imageWidth, m_dimensions.m_imageHeight);
            }

            RHI::ResultCode result = BuildNativeSwapChain(m_dimensions);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            AZLOG_DEBUG("Swapchain created. Width: %u, Height: %u.\n", m_dimensions.m_imageWidth, m_dimensions.m_imageHeight);

            RetireCurrentGeneration();
            auto generation = AZStd::make_shared<PresentationGeneration>();
            generation->m_device = &device;
            generation->m_queue = m_presentationQueue;
            generation->m_surface = m_surface;
            generation->m_swapchain = m_nativeSwapChain;
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            generation->m_maintenance1 = physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::SwapchainMaintenance1);
            m_presentationGeneration = generation;

            m_dimensions.m_imageCount = 0;
            VkResult vkResult =
                device.GetContext().GetSwapchainImagesKHR(device.GetNativeDevice(), m_nativeSwapChain, &m_dimensions.m_imageCount, nullptr);
            VK_RESULT_ASSERT(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            m_swapchainNativeImages.resize(m_dimensions.m_imageCount);

            // Retrieve the native images of the swapchain so they are
            // available when we init the images in InitImageInternal
            vkResult = device.GetContext().GetSwapchainImagesKHR(
                device.GetNativeDevice(), m_nativeSwapChain, &m_dimensions.m_imageCount, m_swapchainNativeImages.data());
            VK_RESULT_ASSERT(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));
            AZLOG_DEBUG("Obtained presentable images.\n");

            generation->m_images.resize(m_dimensions.m_imageCount);
            generation->m_nativeImageIndices.resize(m_dimensions.m_imageCount);
            for (uint32_t index = 0; index < m_dimensions.m_imageCount; ++index)
            {
                generation->m_nativeImageIndices[index] = index;
            }
            generation->m_acquires.resize(m_dimensions.m_imageCount + 1);
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            for (auto& image : generation->m_images)
            {
                image.m_renderFinished = BinarySemaphore::Create();
                image.m_ownershipTransferred = BinarySemaphore::Create();
                result = image.m_renderFinished->Init(device);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
                result = image.m_ownershipTransferred->Init(device);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
                if (generation->m_maintenance1)
                {
                    vkResult = device.GetContext().CreateFence(
                        device.GetNativeDevice(), &fenceInfo, VkSystemAllocator::Get(), &image.m_presentFence);
                    VK_RESULT_ASSERT(vkResult);
                    RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));
                }
            }
            for (auto& acquire : generation->m_acquires)
            {
                vkResult = device.GetContext().CreateFence(
                    device.GetNativeDevice(), &fenceInfo, VkSystemAllocator::Get(), &acquire.m_fence);
                VK_RESULT_ASSERT(vkResult);
                RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));
            }

            // Acquire the first image
            uint32_t imageIndex = 0;
            result = AcquireNewImage(&imageIndex);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            // DeviceSwapChain::InitImages resets its logical index to zero. Vulkan does not guarantee
            // the first acquired native index is zero; preserve a generation-local logical mapping.
            AZStd::swap(m_swapchainNativeImages[0], m_swapchainNativeImages[imageIndex]);
            AZStd::swap(generation->m_nativeImageIndices[0], generation->m_nativeImageIndices[imageIndex]);
            AZLOG_DEBUG("Acquired the first image.\n");

            return RHI::ResultCode::Success;
        }
    }
}
