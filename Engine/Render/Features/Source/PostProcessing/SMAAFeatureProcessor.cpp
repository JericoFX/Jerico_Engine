/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/SMAAConfigurationDescriptor.h>
#include <PostProcessing/SMAAFeatureProcessor.h>
#include <PostProcessing/SMAAEdgeDetectionPass.h>
#include <PostProcessing/SMAABlendingWeightCalculationPass.h>
#include <PostProcessing/SMAANeighborhoodBlendingPass.h>

#include <Atom/RHI/Factory.h>

#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/RPISystemInterface.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>

namespace JE::Render
{
    namespace Features
    {
        void SMAAFeatureProcessor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SMAAFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }

            SMAAConfigurationDescriptor::Reflect(context);
        }

        SMAAFeatureProcessor::SMAAFeatureProcessor()
            : SMAAFeatureProcessorInterface()
            , m_convertToPerceptualColorPassTemplateNameId(SMAAConvertToPerceptualColorPassTemplateName)
            , m_edgeDetectioPassTemplateNameId(SMAAEdgeDetectionPassTemplateName)
            , m_blendingWeightCalculationPassTemplateNameId(SMAABlendingWeightCalculationPassTemplateName)
            , m_neighborhoodBlendingPassTemplateNameId(SMAANeighborhoodBlendingPassTemplateName)
        {
        }

        void SMAAFeatureProcessor::Activate()
        {
            AZ::Data::Asset<JE::Render::RPI::AnyAsset> smaaAsset = JE::Render::RPI::AssetUtils::LoadAssetByProductPath<JE::Render::RPI::AnyAsset>("passes/SMAAConfiguration.azasset", JE::Render::RPI::AssetUtils::TraceLevel::Error);
            const SMAAConfigurationDescriptor* smaaConfigurationDescriptor = JE::Render::RPI::GetDataFromAnyAsset<SMAAConfigurationDescriptor>(smaaAsset);
            m_data.m_enable = smaaConfigurationDescriptor->m_enable != 0;
            SetQualityByPreset(static_cast<SMAAQualityPreset>(smaaConfigurationDescriptor->m_quality));
            m_data.m_edgeDetectionMode = static_cast<SMAAEdgeDetectionMode>(smaaConfigurationDescriptor->m_edgeDetectionMode);
            m_data.m_outputMode = static_cast<SMAAOutputMode>(smaaConfigurationDescriptor->m_outputMode);
            smaaAsset.Release();
        }

        void SMAAFeatureProcessor::Deactivate()
        {
            m_data = {};
        }

        void SMAAFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);
            AZ_UNUSED(packet);
        }

        void SMAAFeatureProcessor::UpdateConvertToPerceptualPass()
        {
            JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithTemplateName(m_convertToPerceptualColorPassTemplateNameId, GetParentScene());
            JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                {
                    pass->SetEnabled(m_data.m_enable);
                    return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void SMAAFeatureProcessor::UpdateEdgeDetectionPass()
        {            
            JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithTemplateName(m_edgeDetectioPassTemplateNameId, GetParentScene());
            JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                {
                    auto* edgeDetectionPass = azrtti_cast<JE::Render::Features::SMAAEdgeDetectionPass*>(pass);

                    edgeDetectionPass->SetEnabled(m_data.m_enable);
                    if (m_data.m_enable)
                    {
                        edgeDetectionPass->SetEdgeDetectionMode(m_data.m_edgeDetectionMode);
                        edgeDetectionPass->SetChromaThreshold(m_data.m_chromaThreshold);
                        edgeDetectionPass->SetDepthThreshold(m_data.m_depthThreshold);
                        edgeDetectionPass->SetLocalContrastAdaptationFactor(m_data.m_localContrastAdaptationFactor);
                        edgeDetectionPass->SetPredicationEnable(m_data.m_predicationEnable);
                        edgeDetectionPass->SetPredicationThreshold(m_data.m_predicationThreshold);
                        edgeDetectionPass->SetPredicationScale(m_data.m_predicationScale);
                        edgeDetectionPass->SetPredicationStrength(m_data.m_predicationStrength);
                    }
                    return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void SMAAFeatureProcessor::UpdateBlendingWeightCalculationPass()
        {
            JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithTemplateName(m_blendingWeightCalculationPassTemplateNameId, GetParentScene());
            JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                {
                    auto* blendingWeightCalculationPass = azrtti_cast<JE::Render::Features::SMAABlendingWeightCalculationPass*>(pass);

                    blendingWeightCalculationPass->SetEnabled(m_data.m_enable);
                    if (m_data.m_enable)
                    {
                        blendingWeightCalculationPass->SetMaxSearchSteps(m_data.m_maxSearchSteps);
                        blendingWeightCalculationPass->SetMaxSearchStepsDiagonal(m_data.m_maxSearchStepsDiagonal);
                        blendingWeightCalculationPass->SetCornerRounding(m_data.m_cornerRounding);
                        blendingWeightCalculationPass->SetDiagonalDetectionEnable(m_data.m_enableDiagonalDetection);
                        blendingWeightCalculationPass->SetCornerDetectionEnable(m_data.m_enableCornerDetection);
                    }
                    return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void SMAAFeatureProcessor::UpdateNeighborhoodBlendingPass()
        {
            JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithTemplateName(m_neighborhoodBlendingPassTemplateNameId, GetParentScene());
            JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                {
                    auto* neighborhoodBlendingPass = azrtti_cast<JE::Render::Features::SMAANeighborhoodBlendingPass*>(pass);

                    if (m_data.m_enable)
                    {
                        neighborhoodBlendingPass->SetOutputMode(m_data.m_outputMode);
                    }
                    else
                    {
                        neighborhoodBlendingPass->SetOutputMode(SMAAOutputMode::PassThrough);
                    }
                    return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                });
        }

        void SMAAFeatureProcessor::Render([[maybe_unused]] const SMAAFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SMAAFeatureProcessor: Render");

            UpdateConvertToPerceptualPass();
            UpdateEdgeDetectionPass();
            UpdateBlendingWeightCalculationPass();
            UpdateNeighborhoodBlendingPass();
        }

        void SMAAFeatureProcessor::SetEnable(bool enable)
        {
            m_data.m_enable = enable;
        }

        void SMAAFeatureProcessor::SetQualityByPreset(SMAAQualityPreset preset)
        {
            switch (preset)
            {
            case SMAAQualityPreset::Low:
                // SMAA_PRESET_LOW
                SetChromaThreshold(0.15f);
                SetMaxSearchSteps(4);
                SetDiagonalDetectionEnable(false);
                SetCornerDetectionEnable(false);
                break;
            case SMAAQualityPreset::Middle:
                // SMAA_PRESET_MEDIUM
                SetChromaThreshold(0.1f);
                SetMaxSearchSteps(8);
                SetDiagonalDetectionEnable(false);
                SetCornerDetectionEnable(false);
                break;
            case SMAAQualityPreset::High:
                // SMAA_PRESET_HIGH
                SetChromaThreshold(0.1f);
                SetMaxSearchSteps(16);
                SetMaxSearchStepsDiagonal(8);
                SetCornerRounding(25);
                SetDiagonalDetectionEnable(true);
                SetCornerDetectionEnable(true);
                break;
            case SMAAQualityPreset::Ultra:
                // SMAA_PRESET_ULTRA
                SetChromaThreshold(0.05f);
                SetMaxSearchSteps(32);
                SetMaxSearchStepsDiagonal(16);
                SetCornerRounding(25);
                SetDiagonalDetectionEnable(true);
                SetCornerDetectionEnable(true);
                break;
            }
        }

        void SMAAFeatureProcessor::SetEdgeDetectionMode(SMAAEdgeDetectionMode mode)
        {
            m_data.m_edgeDetectionMode = mode;
        }

        void SMAAFeatureProcessor::SetChromaThreshold(float threshold)
        {
            m_data.m_chromaThreshold = threshold;
        }

        void SMAAFeatureProcessor::SetDepthThreshold(float threshold)
        {
            m_data.m_depthThreshold = threshold;
        }

        void SMAAFeatureProcessor::SetLocalContrastAdaptationFactor(float factor)
        {
            m_data.m_localContrastAdaptationFactor = factor;
        }

        void SMAAFeatureProcessor::SetPredicationEnable(bool enable)
        {
            m_data.m_predicationEnable = enable;
        }

        void SMAAFeatureProcessor::SetPredicationThreshold(float threshold)
        {
            m_data.m_predicationThreshold = threshold;
        }

        void SMAAFeatureProcessor::SetPredicationScale(float scale)
        {
            m_data.m_predicationScale = scale;
        }

        void SMAAFeatureProcessor::SetPredicationStrength(float strength)
        {
            m_data.m_predicationStrength = strength;
        }

        void SMAAFeatureProcessor::SetMaxSearchSteps(int steps)
        {
            m_data.m_maxSearchSteps = steps;
        }

        void SMAAFeatureProcessor::SetMaxSearchStepsDiagonal(int steps)
        {
            m_data.m_maxSearchStepsDiagonal = steps;
        }

        void SMAAFeatureProcessor::SetCornerRounding(int cornerRounding)
        {
            m_data.m_cornerRounding = cornerRounding;
        }

        void SMAAFeatureProcessor::SetDiagonalDetectionEnable(bool enable)
        {
            m_data.m_enableDiagonalDetection = enable;
        }

        void SMAAFeatureProcessor::SetCornerDetectionEnable(bool enable)
        {
            m_data.m_enableCornerDetection = enable;
        }

        void SMAAFeatureProcessor::SetOutputMode(SMAAOutputMode mode)
        {
            m_data.m_outputMode = mode;
        }

        const SMAAData& SMAAFeatureProcessor::GetSettings() const
        {
            return m_data;
        }

    } // namespace Features
} // namespace JE::Render
