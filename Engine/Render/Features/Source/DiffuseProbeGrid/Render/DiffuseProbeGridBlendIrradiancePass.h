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
        //! Compute shader that generates the irradiance texture from the diffuse probe raytrace.
        class DiffuseProbeGridBlendIrradiancePass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridBlendIrradiancePass);

            AZ_RTTI_WITH_NAME(DiffuseProbeGridBlendIrradiancePass, "AZ::Render::DiffuseProbeGridBlendIrradiancePass", "{521E53EE-2D12-454F-8830-C61EA08CEF35}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridBlendIrradiancePass, AZ::SystemAllocator);
            virtual ~DiffuseProbeGridBlendIrradiancePass() = default;

            static JE::Render::RPI::Ptr<DiffuseProbeGridBlendIrradiancePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridBlendIrradiancePass(const JE::Render::RPI::PassDescriptor& descriptor);

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
