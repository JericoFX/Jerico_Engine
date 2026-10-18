/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/Pass/PassFactory.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Public/Shader/ShaderVariant.h>

#include <JE/Render/RPI/Reflect/Pass/PassTemplate.h>
#include <JE/Render/RPI/Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <PostProcessing/SMAACommon.h>
#include <PostProcessing/SMAANeighborhoodBlendingPass.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<SMAANeighborhoodBlendingPass> SMAANeighborhoodBlendingPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            return aznew SMAANeighborhoodBlendingPass(descriptor);
        }

        SMAANeighborhoodBlendingPass::SMAANeighborhoodBlendingPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : SMAABasePass(descriptor)
            , m_blendingOutputModeOptionName(BlendingOutputModeOptionName)
        {
        }

        void SMAANeighborhoodBlendingPass::InitializeInternal()
        {
            SMAABasePass::InitializeInternal();
            m_renderTargetMetricsShaderInputIndex.Reset();
        }

        void SMAANeighborhoodBlendingPass::UpdateSRG()
        {
            m_shaderResourceGroup->SetConstant(m_renderTargetMetricsShaderInputIndex, m_renderTargetMetrics);
        }

        void SMAANeighborhoodBlendingPass::GetCurrentShaderOption(JE::Render::RPI::ShaderOptionGroup& shaderOption) const
        {
            switch (m_outputMode)
            {
            case SMAAOutputMode::BlendResult:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::BlendResult"));
                break;
            case SMAAOutputMode::PassThrough:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::PassThrough"));
                break;
            case SMAAOutputMode::EdgeTexture:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::EdgeTexture"));
                break;
            case SMAAOutputMode::BlendWeightTexture:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::BlendWeightTexture"));
                break;
            case SMAAOutputMode::BlendResultWithProvisionalTonemap:
            default:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::BlendResultWithProvisionalTonemap"));
                break;
            }
        }

        void SMAANeighborhoodBlendingPass::SetOutputMode(SMAAOutputMode mode)
        {
            if (m_outputMode != mode)
            {
                m_outputMode = mode;
                InvalidateShaderVariant();
            }
        }
    } // namespace Features
} // namespace JE::Render
