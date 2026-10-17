/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>
#include <Atom/RHI/DrawListTagRegistry.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Represents the concrete state to configure a PipelineStateDescriptor. ShaderVariant's match
        //! the RHI::PipelineStateType of the parent Shader instance. For shaders on the raster
        //! pipeline, the RHI::DrawFilterTag is also provided.
        class JE_RENDER_INTEGRATION_API ShaderVariant final
        {
            friend class Shader;
        public:
            ShaderVariant() = default;
            virtual ~ShaderVariant();
            AZ_DEFAULT_COPY_MOVE(ShaderVariant);

            //! Fills a pipeline state descriptor with settings provided by the ShaderVariant. 
            //! It also configures the specialization constants if they are being used by the shader variant.
            //! (Note that this does not fill the InputStreamLayout or OutputAttachmentLayout as that also requires 
            //! information from the mesh data and pass system and must be done as a separate step).
            void ConfigurePipelineState(AZ::RHI::PipelineStateDescriptor& descriptor, const ShaderVariantId& specialization) const;
            //! Fills a pipeline state descriptor with settings provided by the ShaderVariant.
            //! It also configures the specialization constants if they are being used by the shader variant.
            void ConfigurePipelineState(AZ::RHI::PipelineStateDescriptor& descriptor, const ShaderOptionGroup& specialization) const;
            //! Fills a pipeline state descriptor with settings provided by the ShaderVariant.
            //! Only use this function if the shader variant is not using ANY specialization constant. Otherwise
            //! an error will be raised and the default values will be used.
            void ConfigurePipelineState(AZ::RHI::PipelineStateDescriptor& descriptor) const;

            const ShaderVariantId& GetShaderVariantId() const { return m_shaderVariantAsset->GetShaderVariantId(); }

            //! Returns whether the variant is fully baked variant (all options are static branches), or false if the
            //! variant uses dynamic branches for some shader options.
            //! If the shader variant is not fully baked, the ShaderVariantKeyFallbackValue must be correctly set when drawing.
            bool IsFullyBaked() const { return m_shaderVariantAsset->IsFullyBaked(); }

            //! Returns whether the variant is using specialization constants for all of the options.
            bool IsFullySpecialized() const;

            //! Return true if this shader variant has at least one shader option using specialization constant.
            bool UseSpecializationConstants() const;

            //! Return true if this variant needs the ShaderVariantKeyFallbackValue to be correctly set when drawing.
            bool UseKeyFallback() const;

            bool IsRootVariant() const { return m_shaderVariantAsset->IsRootVariant(); }

            ShaderVariantStableId GetStableId() const { return m_shaderVariantAsset->GetStableId(); }

            const AZ::Data::Asset<ShaderAsset>& GetShaderAsset() const { return m_shaderAsset; }
            const AZ::Data::Asset<ShaderVariantAsset>& GetShaderVariantAsset() const { return m_shaderVariantAsset; }

        private:
            // Called by Shader. Initializes runtime data from asset data. Returns whether the call succeeded.
            bool Init(
                const AZ::Data::Asset<ShaderAsset>& shaderAsset,
                const AZ::Data::Asset<ShaderVariantAsset>& shaderVariantAsset,
                SupervariantIndex supervariantIndex);

            //! A reference to the shader asset that this is a variant of.
            AZ::Data::Asset<ShaderAsset> m_shaderAsset;

            // Cached state from the asset to avoid an indirection.
            AZ::RHI::PipelineStateType m_pipelineStateType = AZ::RHI::PipelineStateType::Count;

            // State assigned to the pipeline state descriptor.
            AZ::RHI::ConstPtr<AZ::RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor;
            
            AZ::Data::Asset<ShaderVariantAsset> m_shaderVariantAsset;

            const AZ::RHI::RenderStates* m_renderStates = nullptr; // Cached from ShaderAsset.
            SupervariantIndex m_supervariantIndex;

            // True if there's at least one shader option that is using a specialization constant.
            bool m_useSpecializationConstants = false;
        };
    }
}
