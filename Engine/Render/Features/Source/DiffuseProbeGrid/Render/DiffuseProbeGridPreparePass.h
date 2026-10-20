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
        //! Compute shader that updates grid data for DiffuseProbeGrids
        class DiffuseProbeGridPreparePass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridPreparePass);

            AZ_RTTI_WITH_NAME(DiffuseProbeGridPreparePass, "AZ::Render::DiffuseProbeGridPreparePass", "{5934A485-2550-4100-852C-04A375BCE44E}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridPreparePass, AZ::SystemAllocator);
            virtual ~DiffuseProbeGridPreparePass() = default;

            static JE::Render::RPI::Ptr<DiffuseProbeGridPreparePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridPreparePass(const JE::Render::RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // shader
            AZ::Data::Instance<JE::Render::RPI::Shader> m_shader;
            const AZ::RHI::PipelineState* m_pipelineState = nullptr;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_srgLayout;
            AZ::RHI::DispatchDirect m_dispatchArgs;
        };
    }   // namespace Features
}   // namespace JE::Render
