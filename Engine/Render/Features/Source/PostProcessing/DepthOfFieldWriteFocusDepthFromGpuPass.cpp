/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <PostProcessing/DepthOfFieldWriteFocusDepthFromGpuPass.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> DepthOfFieldWriteFocusDepthFromGpuPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> pass = aznew DepthOfFieldWriteFocusDepthFromGpuPass(descriptor);
            return pass;
        }

        DepthOfFieldWriteFocusDepthFromGpuPass::DepthOfFieldWriteFocusDepthFromGpuPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ComputePass(descriptor)
        {
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "%s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            m_shaderResourceGroup->SetConstant(m_autoFocusScreenPositionIndex, m_autoFocusScreenPosition);
            m_shaderResourceGroup->SetBufferView(
                m_autoFocusDataBufferIndex, m_bufferRef->GetBufferView());

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::SetScreenPosition(const AZ::Vector2& screenPosition)
        {
            m_autoFocusScreenPosition = screenPosition;
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::SetBufferRef(JE::Render::RPI::Ptr<JE::Render::RPI::Buffer> bufferRef)
        {
            m_bufferRef = bufferRef;
        }

        void DepthOfFieldWriteFocusDepthFromGpuPass::BuildInternal()
        {
            AZ_Assert(m_bufferRef != nullptr, "%s has a null buffer when calling BuildInternal.", GetPathName().GetCStr());

            AttachBufferToSlot(AZ::Name("DofDepthInputOutput"), m_bufferRef);
        }

    } // namespace Features
} // namespace JE::Render
