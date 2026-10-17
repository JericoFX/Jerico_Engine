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
#include <Render/DiffuseProbeGridFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        //! Compute shader that classifies probes as active or inactive in the diffuse probe grid.
        class DiffuseProbeGridClassificationPass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridClassificationPass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridClassificationPass, "{98A6477A-F31C-4390-9BEB-9DB8E30BB281}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridClassificationPass, SystemAllocator);
            virtual ~DiffuseProbeGridClassificationPass() = default;

            static JE::Render::RPI::Ptr<DiffuseProbeGridClassificationPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridClassificationPass(const JE::Render::RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // shader
            struct DiffuseProbeGridShader
            {
                Data::Instance<JE::Render::RPI::Shader> m_shader;
                const RHI::PipelineState* m_pipelineState = nullptr;
                RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;
                RHI::DispatchDirect m_dispatchArgs;
            };

            AZStd::vector<DiffuseProbeGridShader> m_shaders;
        };
    }   // namespace Render
}   // namespace AZ
