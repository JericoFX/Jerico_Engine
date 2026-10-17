/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderReloadNotificationBus.h>

namespace JE::Render
{
    namespace RPI
    {
        using ShaderOption = AZStd::pair<AZ::Name, AZ::Name>;
        using ShaderOptionList = AZStd::vector<ShaderOption>;

        //! The PipelineStateForDraw caches descriptor for RHI::PipelineState's creation so the RHI::PipelineState can be created
        //! or updated later when Scene's render pipelines changed or any other data in the descriptor has changed.
        class JE_RENDER_INTEGRATION_API PipelineStateForDraw
            : public AZStd::intrusive_base
            , public ShaderReloadNotificationBus::MultiHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineStateForDraw, AZ::SystemAllocator);

            PipelineStateForDraw();
            PipelineStateForDraw(const PipelineStateForDraw& right);
            ~PipelineStateForDraw();

            //! Initialize the pipeline state from a shader and one of its shader variant
            //! The previous data will be reset
            void Init(const AZ::Data::Instance<Shader>& shader, const ShaderOptionList* optionAndValues = nullptr);
            void Init(const AZ::Data::Instance<Shader>& shader, const ShaderVariantId& shaderVariantId);

            //! Update the pipeline state descriptor for the specified scene
            //! This is usually called when Scene's render pipelines changed
            void SetOutputFromScene(const Scene* scene, AZ::RHI::DrawListTag overrideDrawListTag = AZ::RHI::DrawListTag());

            void SetOutputFromPass(RenderPass* pass);

            //! Set input stream
            void SetInputStreamLayout(const AZ::RHI::InputStreamLayout& inputStreamLayout);

            //! Re-create the RHI::PipelineState for the input Scene.
            //! The created RHI::PipelineState will be cached and can be acquired by using GetPipelineState()
            const AZ::RHI::PipelineState* Finalize();

            //! Get cached RHI::PipelineState.
            //! It triggers an assert if the pipeline state is dirty.
            const AZ::RHI::PipelineState* GetRHIPipelineState() const;

            //! Return the reference of the RenderStates overlay which will be applied to original m_renderState which are loaded from shader variant
            //! Use this function to modify pipeline states RenderStates. 
            //! It sets this pipeline state to dirty whenever it's called.
            //! Use ConstDescriptor() to access read-only RenderStates
            AZ::RHI::RenderStates& RenderStatesOverlay();

            //! Return the reference of the pipeline state descriptor's m_inputStreamLayout which can be modified directly
            //! It sets this pipeline state to dirty whenever it's called.
            //! Use ConstDescriptor() to access read-only InputStreamLayout
            AZ::RHI::InputStreamLayout& InputStreamLayout();

            //! Updates the current shader variant id.
            //! It sets this pipeline state to dirty whenever it's called.
            void UpdateShaderVaraintId(const ShaderVariantId& shaderVariantId);

            const AZ::RHI::PipelineStateDescriptorForDraw& ConstDescriptor() const;

            //! Get the shader which is associated with this PipelineState
            const AZ::Data::Instance<Shader>& GetShader() const;

            //! Setup the shader variant fallback key to a shader resource group if the shader variant is not ready
            //! Return true if the srg was modified. 
            bool UpdateSrgVariantFallback(AZ::Data::Instance<ShaderResourceGroup>& srg) const;

            //! Clear all the states and references
            void Shutdown();

            //! Returns the id of the shader variant being used
            const ShaderVariantId& GetShaderVariantId() const;

        private:
            ///////////////////////////////////////////////////////////////////
            // ShaderReloadNotificationBus overrides...
            void OnShaderReinitialized(const JE::Render::RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const AZ::Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;
            ///////////////////////////////////////////////////////////////////

            // Update shader variant from m_shader. It's called whenever shader, shader asset or shader variant were changed.
            void RefreshShaderVariant();

            AZ::RHI::PipelineStateDescriptorForDraw m_descriptor;

            // The render state overlay which would be applied to render states acquired from shader variant before create the RHI::PipelineState
            AZ::RHI::RenderStates m_renderStatesOverlay;

            AZ::Data::Instance<Shader> m_shader;
            const AZ::RHI::PipelineState* m_pipelineState = nullptr;

            ShaderVariantId m_shaderVariantId;
            
            bool m_initDataFromShader : 1;      // whether it's initialized from a shader
            bool m_hasOutputData : 1;           // whether it has the data from the scene
            bool m_dirty : 1;                   // whether descriptor is dirty
            bool m_isShaderVariantReady : 1;    // whether the shader variant is ready
            bool m_useRenderStatesOverlay : 1;  // whether a render states overlay is used for this pipeline state
        };
    }
}
