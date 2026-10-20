/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Reflect/Pass/PassName.h>
#include <JE/Render/RPI/Public/Image/AttachmentImage.h>
#include <Render/DiffuseProbeGrid.h>

namespace JE::Render
{
    namespace Features
    {
        //! Compute shader that relocates probes in the diffuse probe grid.
        class DiffuseProbeGridRelocationPass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridRelocationPass);

            AZ_RTTI_WITH_NAME(DiffuseProbeGridRelocationPass, "AZ::Render::DiffuseProbeGridRelocationPass", "{E6FCBE1A-0404-49B3-AA78-C2B0DEE94FB1}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridRelocationPass, AZ::SystemAllocator);
            virtual ~DiffuseProbeGridRelocationPass() = default;

            static JE::Render::RPI::Ptr<DiffuseProbeGridRelocationPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridRelocationPass(const JE::Render::RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
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

            // revision number of the ray tracing data when the shader table was built
            uint32_t m_rayTracingDataRevision = 0;
        };
    }   // namespace Features
}   // namespace JE::Render
