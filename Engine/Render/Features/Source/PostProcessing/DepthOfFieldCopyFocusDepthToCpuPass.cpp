/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/CommandList.h>
#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <PostProcessing/DepthOfFieldCopyFocusDepthToCpuPass.h>

namespace JE::Render
{
    namespace Features
    {

        JE::Render::RPI::Ptr<DepthOfFieldCopyFocusDepthToCpuPass> DepthOfFieldCopyFocusDepthToCpuPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DepthOfFieldCopyFocusDepthToCpuPass> depthOfFieldCopyFocusDepthToCpuPass = aznew DepthOfFieldCopyFocusDepthToCpuPass(descriptor);
            return depthOfFieldCopyFocusDepthToCpuPass;
        }

        DepthOfFieldCopyFocusDepthToCpuPass::DepthOfFieldCopyFocusDepthToCpuPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            m_fence = aznew AZ::RHI::Fence;
            AZ_Assert(m_fence != nullptr, "DepthOfFieldCopyFocusDepthToCpuPass failed to create a fence");
            [[maybe_unused]] AZ::RHI::ResultCode result = m_fence->Init(AZ::RHI::MultiDevice::AllDevices, AZ::RHI::FenceState::Reset);
            AZ_Assert(result == AZ::RHI::ResultCode::Success, "DepthOfFieldCopyFocusDepthToCpuPass failed to init fence");
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::SetBufferRef(JE::Render::RPI::Ptr<JE::Render::RPI::Buffer> bufferRef)
        {
            m_bufferRef = bufferRef;
        }

        float DepthOfFieldCopyFocusDepthToCpuPass::GetFocusDepth()
        {
            return m_lastFocusDepth;
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::BuildInternal()
        {
            InitScope(AZ::RHI::ScopeId(GetPathName()));
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::FrameBeginInternal(FramePrepareParams params)
        {
            AZ_Assert(m_bufferRef != nullptr, "%s has a null buffer when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (m_needsInitialize)
            {
                JE::Render::RPI::CommonBufferDescriptor desc;
                desc.m_bufferName = GetPathName().GetStringView();
                desc.m_poolType = JE::Render::RPI::CommonBufferPoolType::ReadBack;
                desc.m_byteCount = sizeof(float);
                desc.m_elementSize = aznumeric_cast<uint32_t>(desc.m_byteCount);
                desc.m_bufferData = nullptr;
                m_readbackBuffer = JE::Render::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                m_copyDescriptor.m_sourceBuffer = m_bufferRef->GetRHIBuffer();
                m_copyDescriptor.m_sourceOffset = 0;
                m_copyDescriptor.m_destinationBuffer = m_readbackBuffer->GetRHIBuffer();
                m_copyDescriptor.m_destinationOffset = 0;
                m_copyDescriptor.m_size = sizeof(float);

                m_needsInitialize = false;
            }

            params.m_frameGraphBuilder->ImportScopeProducer(*this);
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            AZ_Assert(m_bufferRef != nullptr, "%s has a null buffer when calling Prepare.", GetPathName().GetCStr());

            AZ::RHI::BufferScopeAttachmentDescriptor desc;
            desc.m_attachmentId = m_bufferRef->GetAttachmentId();
            desc.m_bufferViewDescriptor = m_bufferRef->GetBufferViewDescriptor();
            desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;
            frameGraph.UseCopyAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read);
            frameGraph.SignalFence(*m_fence);
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            AZ_UNUSED(context);
        }

        void DepthOfFieldCopyFocusDepthToCpuPass::BuildCommandList(const AZ::RHI::FrameGraphExecuteContext& context)
        {
            auto deviceIndex = context.GetDeviceIndex();
            m_fence->GetDeviceFence(deviceIndex)->WaitOnCpuAsync([this, deviceIndex]()
            {
                if (m_readbackBuffer)
                {
                    auto buf = m_readbackBuffer->Map(m_copyDescriptor.m_size, 0);
                    if (buf[deviceIndex] != nullptr)
                    {
                        memcpy(&m_lastFocusDepth, buf[deviceIndex], sizeof(m_lastFocusDepth));
                        m_readbackBuffer->Unmap();
                    }
                }

                m_fence->Reset();
            }
            );

            context.GetCommandList()->Submit(m_copyDescriptor.GetDeviceCopyBufferDescriptor(context.GetDeviceIndex()));
        }

    } // namespace Features
} // namespace JE::Render
