/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/Features/RenderFeaturesExports.h>
#include <Atom/RHI/DispatchRaysIndirectBuffer.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderReloadNotificationBus.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace JE::Render
{
    namespace Features
    {
        struct RayTracingPassData;

        //! This pass executes a raytracing shader as specified in the PassData.
        class JE_RENDER_FEATURES_API RayTracingPass
            : public JE::Render::RPI::RenderPass
            , private JE::Render::RPI::ShaderReloadNotificationBus::MultiHandler
        {
            AZ_RPI_PASS(RayTracingPass);

        public:
            AZ_RTTI(RayTracingPass, "{7A68A36E-956A-4258-93FE-38686042C4D9}", JE::Render::RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(RayTracingPass, AZ::SystemAllocator);
            virtual ~RayTracingPass();

            //! Creates a RayTracingPass
            static JE::Render::RPI::Ptr<RayTracingPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetMaxRayLength(float maxRayLength) { m_maxRayLength = maxRayLength; }

        protected:
            RayTracingPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass overrides
            bool IsEnabled() const override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;

            // Scope producer functions
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides
            void OnShaderReinitialized(const JE::Render::RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const AZ::Data::Asset<JE::Render::RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const JE::Render::RPI::ShaderVariant& shaderVariant) override;

            // load the raytracing shaders and setup pipeline states
            void CreatePipelineState();

            // pass data
            JE::Render::RPI::PassDescriptor m_passDescriptor;
            const RayTracingPassData* m_passData = nullptr;

            AZ::Name m_fullscreenSizeSourceSlotName;
            bool m_fullscreenDispatch = false;
            JE::Render::RPI::PassAttachmentBinding* m_fullscreenSizeSourceBinding = nullptr;

            bool m_indirectDispatch = false;
            AZ::Name m_indirectDispatchBufferSlotName;
            JE::Render::RPI::PassAttachmentBinding* m_indirectDispatchRaysBufferBinding = nullptr;
            AZ::RHI::Ptr<AZ::RHI::IndirectBufferSignature> m_indirectDispatchRaysBufferSignature;
            AZ::RHI::IndirectBufferView m_indirectDispatchRaysBufferView;
            AZ::RHI::Ptr<AZ::RHI::DispatchRaysIndirectBuffer> m_dispatchRaysIndirectBuffer;

            // revision number of the ray tracing TLAS when the shader table was built
            uint32_t m_rayTracingShaderTableRevision{ std::numeric_limits<uint32_t>::max() };
            uint32_t m_dispatchRaysShaderTableRevision{ std::numeric_limits<uint32_t>::max() };
            uint32_t m_proceduralGeometryTypeRevision = 0;

            // raytracing shaders, pipeline states, and shader table
            AZ::Data::Instance<JE::Render::RPI::Shader> m_rayGenerationShader;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_missShader;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_closestHitShader;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_closestHitProceduralShader;
            AZ::Data::Instance<JE::Render::RPI::Shader> m_intersectionShader;
            AZ::RHI::Ptr<AZ::RHI::RayTracingPipelineState> m_rayTracingPipelineState;
            AZ::RHI::ConstPtr<AZ::RHI::PipelineState> m_globalPipelineState;
            AZ::RHI::Ptr<AZ::RHI::RayTracingShaderTable> m_rayTracingShaderTable;

            // [GFX TODO][ATOM-15610] Add RenderPass::SetSrgsForRayTracingDispatch
            // Remove this as soon as we can use the RenderPass::BindSrg() for raytracing
            AZStd::vector<AZ::RHI::ShaderResourceGroup*> m_rayTracingSRGsToBind;

            bool m_requiresViewSrg = false;
            bool m_requiresSceneSrg = false;
            bool m_requiresRayTracingSceneSrg = false;
            float m_maxRayLength = 1e27f;

            AZ::RHI::ShaderInputNameIndex m_maxRayLengthInputIndex = "m_maxRayLength";

            AZ::RHI::DispatchRaysItem m_dispatchRaysItem;
        };
    } // namespace Features
} // namespace JE::Render
