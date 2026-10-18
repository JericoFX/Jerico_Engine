/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/DepthExponentiationPass.h>

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderOptionGroup.h>

namespace JE::Render
{
    namespace Features
    {
        DepthExponentiationPass::DepthExponentiationPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : Base(descriptor)
            , m_optionName("o_shadowmapLightType")
            , m_optionValues
            {
                AZ::Name("ShadowmapLightType::Directional"),
                AZ::Name("ShadowmapLightType::Spot")
            }
        {
        }

        JE::Render::RPI::Ptr<DepthExponentiationPass> DepthExponentiationPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            return aznew DepthExponentiationPass(descriptor);
        }

        void DepthExponentiationPass::SetShadowmapType(Shadow::ShadowmapType type)
        {
            if (!m_shaderOptionInitialized)
            {
                InitializeShaderOption();
                m_shaderOptionInitialized = true;
            }
            m_shadowmapType = type;
            SetShaderVariantKeyFallbackValue();
        }

        void DepthExponentiationPass::SetShaderVariantKeyFallbackValue()
        {
            const uint32_t typeIndex = aznumeric_cast<uint32_t>(m_shadowmapType);
            JE::Render::RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
            shaderOption.SetValue(m_optionName, m_optionValues[typeIndex]);

            if (!m_shaderVariant[typeIndex].m_isFullyBaked && m_shaderResourceGroup)
            {
                m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(shaderOption.GetShaderVariantKeyFallbackValue());
            }
        }

        Shadow::ShadowmapType DepthExponentiationPass::GetShadowmapType() const
        {
            return m_shadowmapType;
        }

        void DepthExponentiationPass::BuildInternal()
        {
            InitializeShaderOption();
        }

        void DepthExponentiationPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (!m_shaderOptionInitialized)
            {
                InitializeShaderOption();
                m_shaderOptionInitialized = true;
            }

            Base::FrameBeginInternal(params);
        }

        void DepthExponentiationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            if (m_shaderVariantNeedsUpdate)
            {
                SetShadowmapType(m_shadowmapType);
                m_shaderVariantNeedsUpdate = false;
            }

            Base::CompileResources(context);
        }

        void DepthExponentiationPass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
        {
            const uint32_t typeIndex = aznumeric_cast<uint32_t>(m_shadowmapType);
            m_dispatchItem.SetPipelineState(m_shaderVariant[typeIndex].m_pipelineState);

            Base::BuildCommandListInternal(context);
        }

        void DepthExponentiationPass::InitializeShaderOption()
        {
            AZ_Assert(m_shader != nullptr, "DepthExponentiationPass %s has a null shader when calling FrameBeginInternal.", GetPathName().GetCStr());

            // Caching all pipeline state for each shader variation for performance reason.
            m_shaderVariant.clear();
            for (const AZ::Name& valueName : m_optionValues)
            {
                JE::Render::RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_optionName, valueName);

                JE::Render::RPI::ShaderVariant shaderVariant = m_shader->GetVariant(shaderOption.GetShaderVariantId());

                AZ::RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shaderOption);

                ShaderVariantInfo variationInfo{!shaderVariant.UseKeyFallback(),
                    m_shader->AcquirePipelineState(pipelineStateDescriptor)
                };
                m_shaderVariant.push_back(AZStd::move(variationInfo));
            }
            m_shaderVariantNeedsUpdate = true;
        }

    } // namespace Rendere
} // namespace JE::Render
