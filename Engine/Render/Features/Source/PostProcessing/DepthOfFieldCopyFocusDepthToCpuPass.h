/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>

namespace JE::Render
{
    namespace Features
    {
        //! This pass is used to read back the depth value written to the buffer.
        class DepthOfFieldCopyFocusDepthToCpuPass final
            : public JE::Render::RPI::Pass
            , public AZ::RHI::ScopeProducer
        {
            AZ_RPI_PASS(DepthOfFieldCopyFocusDepthToCpuPass);

        public:
            AZ_RTTI(DepthOfFieldCopyFocusDepthToCpuPass, "{EA00AD76-92FC-4223-AB7D-87F588AB5394}", Pass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldCopyFocusDepthToCpuPass, AZ::SystemAllocator);

            //! Creates an DepthOfFieldCopyFocusDepthToCpuPass
            static JE::Render::RPI::Ptr<DepthOfFieldCopyFocusDepthToCpuPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            ~DepthOfFieldCopyFocusDepthToCpuPass() = default;

            void SetBufferRef(JE::Render::RPI::Ptr<JE::Render::RPI::Buffer> bufferRef);
            float GetFocusDepth();

        private:
            explicit DepthOfFieldCopyFocusDepthToCpuPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Scope producer functions
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandList(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // Pass overrides
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            JE::Render::RPI::Ptr<JE::Render::RPI::Buffer> m_bufferRef;
            AZ::Data::Instance<JE::Render::RPI::Buffer> m_readbackBuffer;
            AZ::RHI::CopyBufferDescriptor m_copyDescriptor;
            bool m_needsInitialize = true;
            AZ::RHI::Ptr<AZ::RHI::Fence> m_fence;
            float m_lastFocusDepth = 0;
        };
    } // namespace Features
} // namespace JE::Render
