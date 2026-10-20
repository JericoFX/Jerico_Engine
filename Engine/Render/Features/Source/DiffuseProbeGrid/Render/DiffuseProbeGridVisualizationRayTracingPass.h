/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

namespace JE::Render
{
    namespace Features
    {
        //! Ray tracing shader that generates the probe visualization image for a DiffuseProbeGrid
        class DiffuseProbeGridVisualizationRayTracingPass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridVisualizationRayTracingPass);

            AZ_RTTI(DiffuseProbeGridVisualizationRayTracingPass, "{CDFB5C03-08D1-4FCA-8B63-2F8326E0DF1D}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridVisualizationRayTracingPass, AZ::SystemAllocator);

            //! Creates a DiffuseProbeGridVisualizationRayTracingPass
            static JE::Render::RPI::Ptr<DiffuseProbeGridVisualizationRayTracingPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            ~DiffuseProbeGridVisualizationRayTracingPass() = default;

        private:
            explicit DiffuseProbeGridVisualizationRayTracingPass(const JE::Render::RPI::PassDescriptor& descriptor);

            void CreateRayTracingPipelineState();

            // Scope producer functions
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // Pass overrides
            bool IsEnabled() const override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // ray tracing shader and pipeline state
            AZ::Data::Instance<JE::Render::RPI::Shader> m_rayTracingShader;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_missShader;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_closestHitShader;
            AZ::RHI::Ptr<AZ::RHI::RayTracingPipelineState> m_rayTracingPipelineState;

            // ray tracing shader table
            AZ::RHI::Ptr<AZ::RHI::RayTracingShaderTable> m_rayTracingShaderTable;

            // current size of output attachment, updated every frame
            AZ::RHI::Size m_outputAttachmentSize;

            // ray tracing global shader resource group layout and pipeline state
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_globalSrgLayout;
            AZ::RHI::ConstPtr<AZ::RHI::PipelineState> m_globalPipelineState;

            bool m_initialized = false;
        };
    }   // namespace RPI
}   // namespace JE::Render
