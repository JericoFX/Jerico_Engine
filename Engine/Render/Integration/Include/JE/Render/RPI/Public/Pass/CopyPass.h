/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <JE/Render/RPI/Reflect/Pass/CopyPassData.h>

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>

namespace JE::Render
{
    namespace RPI
    {
        class Query;

        //! A copy pass is a leaf pass (pass with no children) used for copying images and buffers on the GPU.
        class JE_RENDER_INTEGRATION_API CopyPass : public Pass
        {
            AZ_RPI_PASS(CopyPass);

            using ScopeQuery = AZStd::array<AZ::RHI::Ptr<Query>, static_cast<size_t>(ScopeQueryType::Count)>;

        public:
            AZ_RTTI(CopyPass, "{7387500D-B1BA-4916-B38C-24F5C8DAF839}", Pass);
            AZ_CLASS_ALLOCATOR(CopyPass, AZ::SystemAllocator);
            virtual ~CopyPass();

            static Ptr<CopyPass> Create(const PassDescriptor& descriptor);

        protected:
            explicit CopyPass(const PassDescriptor& descriptor);

            void CopyBuffer(const AZ::RHI::FrameGraphCompileContext& context);
            void CopyImage(const AZ::RHI::FrameGraphCompileContext& context);
            void CopyBufferToImage(const AZ::RHI::FrameGraphCompileContext& context);
            void CopyImageToBuffer(const AZ::RHI::FrameGraphCompileContext& context);

            // Pass behavior overrides
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void ResetInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependenciesSameDevice(AZ::RHI::FrameGraphInterface frameGraph);
            void CompileResourcesSameDevice(const AZ::RHI::FrameGraphCompileContext& context);
            void BuildCommandListInternalSameDevice(const AZ::RHI::FrameGraphExecuteContext& context);
            void SetupFrameGraphDependenciesDeviceToHost(AZ::RHI::FrameGraphInterface frameGraph);
            void CompileResourcesDeviceToHost(const AZ::RHI::FrameGraphCompileContext& context);
            void BuildCommandListInternalDeviceToHost(const AZ::RHI::FrameGraphExecuteContext& context);
            void SetupFrameGraphDependenciesHostToDevice(AZ::RHI::FrameGraphInterface frameGraph);
            void CompileResourcesHostToDevice(const AZ::RHI::FrameGraphCompileContext& context);
            void BuildCommandListInternalHostToDevice(const AZ::RHI::FrameGraphExecuteContext& context);

        private:
            // Retrieves the copy item type based on the input and output attachment type
            AZ::RHI::CopyItemType GetCopyItemType();

            // Cross device copy strategy
            // The strategy chosen based on device capabilities
            enum class CrossDeviceStrategy
            {
                // No cross device resources are used
                // The synchronization is done through a CPU callback, which performs a memcpy between mapped host buffers
                // The semaphore of the destination device is signalled from the host
                IntermediateHost,
                // Only cross device host memory is used, but no cross device semaphores
                // The synchronization is done through a CPU callback
                // The semaphore of the destination device is signalled from the host
                IntermediateHostCrossDeviceMemory,
                // Cross device fences and cross device host memory is used
                // No CPU callback is involved
                CrossDeviceDeviceMemory,
                // Cross device fences and cross device device memory is used
                // No CPU callback is involved
                CrossDeviceHostMemory
            };
            CrossDeviceStrategy GetCrossDeviceStrategy();
            bool UseCrossDeviceFence();
            bool UseCrossDeviceMemory();

            // The copy item submitted to the command list
            AZ::RHI::CopyItem m_copyItemSameDevice;

            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_copyScopeProducerSameDevice;
            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_copyScopeProducerDeviceToHost;
            AZStd::shared_ptr<AZ::RHI::ScopeProducer> m_copyScopeProducerHostToDevice;

            // Potential data provided by the PassRequest
            CopyPassData m_data;

            AZ::RHI::HardwareQueueClass m_hardwareQueueClass = AZ::RHI::HardwareQueueClass::Graphics;

            enum class CopyMode
            {
                SameDevice,
                DifferentDevices,
                Invalid
            };
            CopyMode m_copyMode = CopyMode::Invalid;
            // Set to true for the MultiDeviceCopyPass, which uses one InputOutput slot instead of one Input and one Output slot.
            bool m_inputOutputCopy = false;

            constexpr static int MaxFrames = AZ::RHI::Limits::Device::FrameCountMax;

            struct PerAspectCopyInfo
            {
                AZ::RHI::CopyItem m_copyItemDeviceToHost{};
                AZ::RHI::CopyItem m_copyItemHostToDevice{};
                AZStd::array<AZ::Data::Instance<Buffer>, MaxFrames> m_device1HostBuffer{};
                AZStd::array<AZ::Data::Instance<Buffer>, MaxFrames> m_device2HostBuffer{};
                AZStd::array<AZ::u64, MaxFrames> m_deviceHostBufferByteCount{};
                AZ::RHI::DeviceImageSubresourceLayout m_inputImageLayout;
            };

            // In case of an image to image copy, we need to store the format of the source image
            AZ::RHI::Format m_sourceFormat{ AZ::RHI::Format::Unknown };

            // Multiple aspects cannot be copied at the same time, so we need a copy items (and corresponding other members)
            // for each aspect. This is the case for example, when we want to copy a depth-stencil-image.
            AZStd::vector<PerAspectCopyInfo> m_perAspectCopyInfos;

            int m_currentBufferIndex = 0;
            AZStd::array<Ptr<AZ::RHI::Fence>, MaxFrames> m_device1SignalFence;
            AZStd::array<Ptr<AZ::RHI::Fence>, MaxFrames> m_device2WaitFence;

            enum class CopyIndex : int
            {
                DeviceToHost = 0,
                HostToDevice = 1,
                SameDevice = DeviceToHost
            };

            // Add the ScopeQuery's QueryPool to the FrameGraph
            void AddScopeQueryToFrameGraph(AZ::RHI::FrameGraphInterface frameGraph, CopyIndex copyIndex);

            // RPI::Pass overrides...
            TimestampResult GetTimestampResultInternal() const override;
            PipelineStatisticsResult GetPipelineStatisticsResultInternal() const override;

            // Helper function to get the query by the scope index and query type
            AZ::RHI::Ptr<Query> GetQuery(ScopeQueryType queryType, CopyIndex copyIndex);

            // Executes a lambda depending on the passed ScopeQuery types
            template<typename Func>
            void ExecuteOnTimestampQuery(Func&& func, CopyIndex copyIndex);
            template<typename Func>
            void ExecuteOnPipelineStatisticsQuery(Func&& func, CopyIndex copyIndex);

            // Begin recording commands for the ScopeQueries
            void BeginScopeQuery(const AZ::RHI::FrameGraphExecuteContext& context, CopyIndex copyIndex);

            // End recording commands for the ScopeQueries
            void EndScopeQuery(const AZ::RHI::FrameGraphExecuteContext& context, CopyIndex copyIndex);

            // Readback the results from the ScopeQueries
            void ReadbackScopeQueryResults(CopyIndex copyIndex);

            struct QueryEntry
            {
                ScopeQuery m_scopeQuery;
                TimestampResult m_timestampResult;
                PipelineStatisticsResult m_statisticsResult;
            };

            // Record per-scope query results, holds one valid entry for same-device copies
            // and two entries for multi-device copy (source and destination scope)
            AZStd::array<QueryEntry, 2> m_queryEntries;
        };
    } // namespace RPI
} // namespace AZ
