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
#include <Atom/RHI/ScopeProducer.h>
#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>

namespace JE::Render
{
    namespace RHI
    {
        class PipelineState;
    }

    namespace Features
    {
        //! Compute shader that generates the distance texture from the diffuse probe raytrace.
        class DiffuseProbeGridBlendDistancePass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridBlendDistancePass);

            AZ_RTTI_WITH_NAME(DiffuseProbeGridBlendDistancePass, "AZ::Render::DiffuseProbeGridBlendDistancePass", "{8D57B51E-3432-49F6-83F8-DB6DF2F5A5CA}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridBlendDistancePass, AZ::SystemAllocator);
            virtual ~DiffuseProbeGridBlendDistancePass() = default;

            static JE::Render::RPI::Ptr<DiffuseProbeGridBlendDistancePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridBlendDistancePass(const JE::Render::RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // shaders
            struct DiffuseProbeGridShader
            {
                AZ::Data::Instance<JE::Render::RPI::Shader> m_shader;
                const AZ::RHI::PipelineState* m_pipelineState = nullptr;
                AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_srgLayout;
                AZ::RHI::DispatchDirect m_dispatchArgs;
            };

            AZStd::vector<DiffuseProbeGridShader> m_shaders;
        };
    }   // namespace Features
}   // namespace JE::Render
