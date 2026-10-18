/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>

namespace JE::Render
{
    namespace Features
    {
        //! This pass is used to write the depth value of the specified screen coordinates to the buffer.
        class DepthOfFieldWriteFocusDepthFromGpuPass final
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(DepthOfFieldWriteFocusDepthFromGpuPass);

        public:
            AZ_RTTI(DepthOfFieldWriteFocusDepthFromGpuPass, "{60DF04D2-A9FE-4B21-8050-96AFFC46BB87}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldWriteFocusDepthFromGpuPass, AZ::SystemAllocator);

            /// Creates a DepthOfFieldWriteFocusDepthFromGpuPass
            static JE::Render::RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            // Set pass parameter interfaces...
            void SetScreenPosition(const AZ::Vector2& screenPosition);
            void SetBufferRef(JE::Render::RPI::Ptr<JE::Render::RPI::Buffer> bufferRef);

        private:
            DepthOfFieldWriteFocusDepthFromGpuPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_autoFocusDataBufferIndex = "m_outputFocusDepth";
            AZ::RHI::ShaderInputNameIndex m_autoFocusScreenPositionIndex = "m_autoFocusScreenPosition";

            JE::Render::RPI::Ptr<JE::Render::RPI::Buffer> m_bufferRef = nullptr;
            AZ::Vector2 m_autoFocusScreenPosition{ 0.0f, 0.0f };

            // Scope producer functions...
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            // Pass overrides
            void BuildInternal() override;
        };
    } // namespace Features
} // namespace JE::Render
