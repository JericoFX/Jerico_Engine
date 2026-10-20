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
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>

namespace JE::Render
{
    namespace Features
    {
        //! Compute shader that updates probe distance and irradiance borders.
        class DiffuseProbeGridBorderUpdatePass final
            : public JE::Render::RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridBorderUpdatePass);

            AZ_RTTI_WITH_NAME(DiffuseProbeGridBorderUpdatePass, "AZ::Render::DiffuseProbeGridBorderUpdatePass", "{31A5CCD0-CE97-4138-88DA-7BDBD38C9DC8}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridBorderUpdatePass, AZ::SystemAllocator);
            virtual ~DiffuseProbeGridBorderUpdatePass() = default;

            static JE::Render::RPI::Ptr<DiffuseProbeGridBorderUpdatePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridBorderUpdatePass(const JE::Render::RPI::PassDescriptor& descriptor);

            void LoadShader(AZStd::string shaderFilePath,
                            AZ::Data::Instance<JE::Render::RPI::Shader>& shader,
                            const AZ::RHI::PipelineState*& pipelineState,
                            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout>& srgLayout,
                            AZ::RHI::DispatchDirect& dispatchArgs);

            // Pass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;
            void FrameEndInternal() override;

            // the data for submits in this pass are pre-built to properly handle submitting on multiple threads
            struct SubmitItem
            {
                AZ::RHI::ShaderResourceGroup* m_shaderResourceGroup{nullptr};
                AZ::RHI::DispatchItem m_dispatchItem{AZ::RHI::MultiDevice::AllDevices};
            };

            AZStd::vector<SubmitItem> m_submitItems;

            // shader
            AZ::Data::Instance<JE::Render::RPI::Shader> m_rowShader;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_columnShader;
            const AZ::RHI::PipelineState* m_rowPipelineState = nullptr;
            const AZ::RHI::PipelineState* m_columnPipelineState = nullptr;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_rowSrgLayout;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_columnSrgLayout;
            AZ::RHI::DispatchDirect m_rowDispatchArgs;
            AZ::RHI::DispatchDirect m_columnDispatchArgs;
        };
    }   // namespace Features
}   // namespace JE::Render
