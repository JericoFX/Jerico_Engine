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

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Shader/ShaderReloadNotificationBus.h>
#include <JE/Render/RPI/Public/PipelineState.h>

namespace JE::Render
{
    namespace RPI
    {
        //! A pass that renders to the entire screen using a single triangle.
        //! This pass is designed to work without any dedicated geometry buffers
        //! and instead issues a 3-vertex draw and relies on the vertex shader
        //! to generate a fullscreen triangle from the vertex ids.
        class JE_RENDER_INTEGRATION_API FullscreenTrianglePass
            : public RenderPass
            , public ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(FullscreenTrianglePass);

        public:
            AZ_RTTI(FullscreenTrianglePass, "{58C1EDD7-0459-4128-BB20-9839BA233AED}", RenderPass);
            AZ_CLASS_ALLOCATOR(FullscreenTrianglePass, AZ::SystemAllocator);
            virtual ~FullscreenTrianglePass();

            //! Creates a FullscreenTrianglePass
            static Ptr<FullscreenTrianglePass> Create(const PassDescriptor& descriptor);

            //! Return the shader
            AZ::Data::Instance<Shader> GetShader() const;

            //! Updates the shader options used in this pass
            void UpdateShaderOptions(const ShaderOptionList& shaderOptions);
            void UpdateShaderOptions(const ShaderVariantId& shaderVariantId);

        protected:
            FullscreenTrianglePass(const PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus overrides...
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const AZ::Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;

            // Common code when updating the shader variant with new options
            void UpdateShaderOptionsCommon();

            AZ::RHI::Viewport m_viewportState;
            AZ::RHI::Scissor m_scissorState;

            // The fullscreen shader that will be used by the pass
            AZ::Data::Instance<Shader> m_shader;

            // Manages changes to the pipeline state
            PipelineStateForDraw m_pipelineStateForDraw;

            // The draw item submitted by this pass
            AZ::RHI::DrawItem m_item;

            // Holds the geometry info for the draw call
            AZ::RHI::GeometryView m_geometryView{ AZ::RHI::MultiDevice::AllDevices };

            // The stencil reference value for the draw item
            uint32_t m_stencilRef;

            AZ::Data::Instance<ShaderResourceGroup> m_drawShaderResourceGroup;

        private:
            void LoadShader();
            void UpdateSrgs();
            void BuildDrawItem();

            PassDescriptor m_passDescriptor;
        };
    }   // namespace RPI
}   // namespace AZ
