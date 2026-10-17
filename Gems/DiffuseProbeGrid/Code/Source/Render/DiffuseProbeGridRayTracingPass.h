/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ScopeProducer.h>
#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <Render/DiffuseProbeGrid.h>

namespace AZ
{
    namespace Render
    {
        constexpr uint32_t MaxRecursionDepth = 16;

        //! Ray tracing shader that generates probe radiance values.
        class DiffuseProbeGridRayTracingPass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridRayTracingPass);

            AZ_RTTI(DiffuseProbeGridRayTracingPass, "{CB0DF817-3D07-4AC7-8574-F5EE529B8DCA}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridRayTracingPass, SystemAllocator);

            //! Creates a DiffuseProbeGridRayTracingPass
            static JE::Render::RPI::Ptr<DiffuseProbeGridRayTracingPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            explicit DiffuseProbeGridRayTracingPass(const JE::Render::RPI::PassDescriptor& descriptor);

            void CreateRayTracingPipelineState();

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // Pass overrides
            bool IsEnabled() const override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // revision number of the ray tracing TLAS when the shader table was built
            uint32_t m_rayTracingRevision = 0;

            // ray tracing shader and pipeline state
            Data::Instance<JE::Render::RPI::Shader> m_rayTracingShader;
            Data::Instance<JE::Render::RPI::Shader> m_closestHitShader;
            Data::Instance<JE::Render::RPI::Shader> m_missShader;
            RHI::Ptr<RHI::RayTracingPipelineState> m_rayTracingPipelineState;

            // ray tracing shader table
            RHI::Ptr<RHI::RayTracingShaderTable> m_rayTracingShaderTable;

            // ray tracing global shader resource group layout and pipeline state
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_globalSrgLayout;
            RHI::ConstPtr<RHI::PipelineState> m_globalPipelineState;

            RHI::ShaderInputNameIndex m_maxRecursionDepthNameIndex = "m_maxRecursionDepth";
        };
    }   // namespace RPI
}   // namespace AZ
