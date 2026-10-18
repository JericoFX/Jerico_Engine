/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

#include <JE/Render/RPI/Reflect/Shader/ShaderVariantKey.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

namespace JE::Render
{
    namespace Features
    {
        //! This class provides common code to use shader option.
        class PostProcessingShaderOptionBase
        {
        public:
            PostProcessingShaderOptionBase() = default;
            ~PostProcessingShaderOptionBase() = default;

        protected:
            //! Creates the PSO for ShaderOption and save it as a cache. Use this for raster passes.
            void PreloadShaderVariantForDraw(
                const AZ::Data::Instance<JE::Render::RPI::Shader>& shader,
                const JE::Render::RPI::ShaderOptionGroup& shaderOption,
                const AZ::RHI::RenderAttachmentConfiguration& renderAttachmentConfiguration,
                const AZ::RHI::MultisampleState& multisampleState);
            //! Creates the PSO for ShaderOption and save it as a cache. Use this for compute passes.
            void PreloadShaderVariantForDispatch(
                const AZ::Data::Instance<JE::Render::RPI::Shader>& shader,
                const JE::Render::RPI::ShaderOptionGroup& shaderOption);

            //! Update shaderVariant. Used when the shader is switched.
            void UpdateShaderVariant(const JE::Render::RPI::ShaderOptionGroup& shaderOption);

            //! Set a key in the SRG for dynamic branching for shaders not created in advance.
            void CompileShaderVariant(AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>& shaderResourceGroup);

            //! Get precomputed pipeline.
            const AZ::RHI::PipelineState* GetPipelineStateFromShaderVariant() const;

        private:
            struct ShaderVariantInformation
            {
                bool m_isFullyBaked = false;
                const AZ::RHI::PipelineState* m_pipelineState = nullptr;
            };
            AZStd::unordered_map<AZ::u64, ShaderVariantInformation> m_shaderVariantTable;
            AZ::u64 m_currentShaderVariantKeyValue = 0;
            JE::Render::RPI::ShaderVariantKey m_currentShaderVariantKeyFallbackValue;

            const ShaderVariantInformation* GetShaderVariant(AZ::u64 key) const;
        };
    } // namespace Features
} // namespace JE::Render
