/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <Render/DiffuseProbeGrid.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>

namespace JE::Render
{
    namespace Features
    {
        //! Compute pass that queries the diffuse irradiance from the DiffuseProbeGrids
        class DiffuseProbeGridQueryPass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridQueryPass);

            AZ_RTTI_WITH_NAME(DiffuseProbeGridQueryPass, "AZ::Render::DiffuseProbeGridQueryPass", "{EE52C193-98E2-4F93-8A19-B4E710F12592}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridQueryPass, AZ::SystemAllocator);
            virtual ~DiffuseProbeGridQueryPass() = default;

            static JE::Render::RPI::Ptr<DiffuseProbeGridQueryPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridQueryPass(const JE::Render::RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;
            void FrameEndInternal() override;

            // shader
            AZ::Data::Instance<JE::Render::RPI::Shader> m_shader;
            const AZ::RHI::PipelineState* m_pipelineState = nullptr;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_srgLayout;
            AZ::RHI::DispatchDirect m_dispatchArgs;

            // output buffer
            AZ::RHI::AttachmentId m_outputBufferAttachmentId;
            AZ::RHI::BufferDescriptor m_outputBufferDesc;
            AZ::RHI::BufferViewDescriptor m_outputBufferViewDesc;
            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> m_outputAttachment;
        };
    }   // namespace Features
}   // namespace JE::Render
