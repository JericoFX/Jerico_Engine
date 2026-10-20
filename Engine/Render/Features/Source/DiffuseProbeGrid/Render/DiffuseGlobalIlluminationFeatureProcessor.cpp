/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <Render/DiffuseGlobalIlluminationFeatureProcessor.h>

namespace JE::Render
{
    namespace Features
    {
        void DiffuseGlobalIlluminationFeatureProcessor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DiffuseGlobalIlluminationFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        void DiffuseGlobalIlluminationFeatureProcessor::Activate()
        {
            EnableSceneNotification();
        }

        void DiffuseGlobalIlluminationFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
        }

        void DiffuseGlobalIlluminationFeatureProcessor::SetQualityLevel(DiffuseGlobalIlluminationQualityLevel qualityLevel)
        {
            if (qualityLevel >= DiffuseGlobalIlluminationQualityLevel::Count)
            {
                AZ_Assert(false, "SetQualityLevel called with invalid quality level [%d]", qualityLevel);
                return;
            }

            m_qualityLevel = qualityLevel;

            UpdatePasses();
        }

        void DiffuseGlobalIlluminationFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] JE::Render::RPI::RenderPipeline* renderPipeline,
                JE::Render::RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == JE::Render::RPI::SceneNotification::RenderPipelineChangeType::Added
                || changeType == JE::Render::RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                UpdatePasses();
            }
        }

        void DiffuseGlobalIlluminationFeatureProcessor::UpdatePasses()
        {
            float sizeMultiplier = 0.0f;
            switch (m_qualityLevel)
            {
            case DiffuseGlobalIlluminationQualityLevel::Low:
                sizeMultiplier = 0.25f;
                break;
            case DiffuseGlobalIlluminationQualityLevel::Medium:
                sizeMultiplier = 0.5f;
                break;
            case DiffuseGlobalIlluminationQualityLevel::High:
                sizeMultiplier = 1.0f;
                break;
            default:
                AZ_Assert(false, "Unknown DiffuseGlobalIlluminationQualityLevel [%d]", m_qualityLevel);
                break;
            }

            // update the size multiplier on the DiffuseProbeGridDownsamplePass output
            // NOTE: The ownerScene wasn't added to both filters. This is because the passes from the non-owner scene may have invalid SRG values which could lead to
            // GPU error if the scene doesn't have this feature processor enabled.
            // For example, the ASV MultiScene sample may have TDR.
            {
                AZStd::vector<AZ::Name> downsamplePassHierarchy = { AZ::Name("DiffuseGlobalIlluminationPass"), AZ::Name("DiffuseProbeGridDownsamplePass") };
                JE::Render::RPI::PassFilter downsamplePassFilter = JE::Render::RPI::PassFilter::CreateWithPassHierarchy(downsamplePassHierarchy);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(
                    downsamplePassFilter,
                    [sizeMultiplier](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        // update the downsample pass size multipliers
                        for (uint32_t attachmentIndex = 0; attachmentIndex < pass->GetOutputCount(); ++attachmentIndex)
                        {
                            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> attachment = pass->GetOutputBinding(attachmentIndex).GetAttachment();
                            if (attachment)
                            {
                                JE::Render::RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                            }
                        }

                        // set the output scale on the PassSrg
                        JE::Render::RPI::FullscreenTrianglePass* downsamplePass = static_cast<JE::Render::RPI::FullscreenTrianglePass*>(pass);
                        AZ::RHI::ShaderInputNameIndex outputImageScaleShaderInput = "m_outputImageScale";
                        downsamplePass->GetShaderResourceGroup()->SetConstant(
                            outputImageScaleShaderInput, aznumeric_cast<uint32_t>(1.0f / sizeMultiplier));

                        // update the parent pass IrradianceImage size multiplier
                        JE::Render::RPI::ParentPass* parentPass = pass->GetParent();
                        JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> irradianceImageAttachment;
                        for (uint32_t attachmentIndex = 0; attachmentIndex < parentPass->GetInputOutputCount(); ++attachmentIndex)
                        {
                            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> attachment = parentPass->GetInputOutputBinding(attachmentIndex).GetAttachment();
                            if (attachment && attachment->m_name == AZ::Name("IrradianceImage"))
                            {
                                irradianceImageAttachment = attachment;
                                break;
                            }
                        }

                        if (irradianceImageAttachment)
                        {
                            JE::Render::RPI::PassAttachmentSizeMultipliers& sizeMultipliers = irradianceImageAttachment->m_sizeMultipliers;
                            sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                            sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                        }

                        // handle all downsample passes
                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // update the image scale on the DiffuseComposite pass
            {
                AZStd::vector<AZ::Name> compositePassHierarchy = { AZ::Name("DiffuseGlobalIlluminationPass"), AZ::Name("DiffuseCompositePass") };
                JE::Render::RPI::PassFilter compositePassFilter = JE::Render::RPI::PassFilter::CreateWithPassHierarchy(compositePassHierarchy);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(compositePassFilter, [sizeMultiplier](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        JE::Render::RPI::FullscreenTrianglePass* compositePass = static_cast<JE::Render::RPI::FullscreenTrianglePass*>(pass);
                        AZ::RHI::ShaderInputNameIndex imageScaleShaderInput = "m_imageScale";
                        compositePass->GetShaderResourceGroup()->SetConstant(imageScaleShaderInput, aznumeric_cast<uint32_t>(1.0f / sizeMultiplier));

                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }
        }
    } // namespace Features
} // namespace JE::Render
