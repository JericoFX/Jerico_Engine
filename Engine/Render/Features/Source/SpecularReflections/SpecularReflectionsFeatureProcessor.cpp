/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SpecularReflections/SpecularReflectionsFeatureProcessor.h>
#include <JE/Render/Features/RayTracing/RayTracingPass.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <JE/Render/RPI/Public/RPISystemInterface.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <ReflectionScreenSpace/ReflectionScreenSpacePass.h>

namespace JE::Render
{
    namespace Features
    {
        void SpecularReflectionsFeatureProcessor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SpecularReflectionsFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        void SpecularReflectionsFeatureProcessor::Activate()
        {
            EnableSceneNotification();
        }

        void SpecularReflectionsFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
        }

        void SpecularReflectionsFeatureProcessor::SetSSROptions(const SSROptions& ssrOptions)
        {
            m_ssrOptions = ssrOptions;

            UpdatePasses();
        }

        void SpecularReflectionsFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] JE::Render::RPI::RenderPipeline* renderPipeline,
            JE::Render::RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == JE::Render::RPI::SceneNotification::RenderPipelineChangeType::Added
                || changeType == JE::Render::RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                UpdatePasses();
            }
        }

        void SpecularReflectionsFeatureProcessor::UpdatePasses()
        {
            // disable raytracing if the platform does not support it
            if (AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() == AZ::RHI::MultiDevice::NoDevices)
            {
                m_ssrOptions.m_reflectionMethod = SSROptions::ReflectionMethod::ScreenSpace;
            }

            // determine size multiplier to pass to the shaders
            float sizeMultiplier = m_ssrOptions.m_halfResolution ? 0.5f : 1.0f;

            // parent SSR pass
            {
                JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpacePass"), (JE::Render::RPI::Scene*) nullptr);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this, sizeMultiplier](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        // enable/disable
                        pass->SetEnabled(m_ssrOptions.m_enable);

                        // reset frame delay if screenspace reflections are disabled
                        if (!m_ssrOptions.m_enable)
                        {
                            ReflectionScreenSpacePass* reflectionScreenSpacePass = azrtti_cast<ReflectionScreenSpacePass*>(pass);
                            reflectionScreenSpacePass->ResetFrameDelay();
                        }

                        // size multiplier
                        static AZStd::vector<AZ::Name> attachmentNames = { AZ::Name("RayTracingCoordsOutput"), AZ::Name("FallbackColor") };
                        for (const auto& attachmentName : attachmentNames)
                        {
                            JE::Render::RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(attachmentName);

                            if (attachmentBinding)
                            {
                                JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                                if (attachment.get())
                                {
                                    JE::Render::RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                    sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                    sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                                }
                            }
                        }

                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // copy framebuffer pass
            {
                JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionCopyFrameBufferPass"), (JE::Render::RPI::Scene*) nullptr);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        // enable/disable
                        pass->SetEnabled(m_ssrOptions.m_enable);
                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // raytracing pass
            {
                JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceRayTracingPass"), (JE::Render::RPI::Scene*) nullptr);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this, sizeMultiplier](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        // enable/disable
                        pass->SetEnabled(m_ssrOptions.IsRayTracingEnabled());

                        if (m_ssrOptions.IsRayTracingEnabled())
                        {
                            // options
                            RayTracingPass* rayTracingPass = azrtti_cast<RayTracingPass*>(pass);
                            rayTracingPass->SetMaxRayLength(m_ssrOptions.m_maxRayDistance);

                            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> rayTracingPassSrg = rayTracingPass->GetShaderResourceGroup();

                            rayTracingPassSrg->SetConstant(m_invOutputScaleNameIndex, 1.0f / m_ssrOptions.GetOutputScale());
                            rayTracingPassSrg->SetConstant(m_maxRoughnessNameIndex, m_ssrOptions.m_maxRoughness);
                            rayTracingPassSrg->SetConstant(m_reflectionMethodNameIndex, m_ssrOptions.m_reflectionMethod);
                            rayTracingPassSrg->SetConstant(m_rayTraceFallbackSpecularNameIndex, m_ssrOptions.m_rayTraceFallbackSpecular);

                            // size multiplier
                            static AZStd::vector<AZ::Name> attachmentNames = { AZ::Name("FallbackAlbedo"), AZ::Name("FallbackPosition"), AZ::Name("FallbackNormal") };
                            for (const auto& attachmentName : attachmentNames)
                            {
                                JE::Render::RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(attachmentName);

                                if (attachmentBinding)
                                {
                                    JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                                    if (attachment.get())
                                    {
                                        JE::Render::RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                        sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                        sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                                    }
                                }
                            }
                        }

                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // trace pass
            {
                JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceTracePass"), (JE::Render::RPI::Scene*) nullptr);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [sizeMultiplier](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        // size multiplier
                        static AZStd::vector<AZ::Name> attachmentNames = { AZ::Name("ScreenSpaceReflectionOutput"), AZ::Name("TraceCoordsOutput") };
                        for (const auto& attachmentName : attachmentNames)
                        {
                            JE::Render::RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(attachmentName);

                            if (attachmentBinding)
                            {
                                JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                                if (attachment.get())
                                {
                                    JE::Render::RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                    sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                    sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                                }
                            }
                        }

                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // downsample depth linear pass
            {
                JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceDownsampleDepthLinearPass"), (JE::Render::RPI::Scene*) nullptr);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [sizeMultiplier](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        // size multiplier
                        JE::Render::RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(AZ::Name("DownsampledDepthLinearInputOutput"));

                        if (attachmentBinding)
                        {
                            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                            if (attachment.get())
                            {
                                JE::Render::RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;
            
                                sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                            }
                        }
            
                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // filter pass
            {
                JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceFilterPass"), (JE::Render::RPI::Scene*) nullptr);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [sizeMultiplier](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        // size multiplier
                        static AZStd::vector<AZ::Name> attachmentNames = { AZ::Name("Output") };
                        for (const auto& attachmentName : attachmentNames)
                        {
                            JE::Render::RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(attachmentName);

                            if (attachmentBinding)
                            {
                                JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                                if (attachment.get())
                                {
                                    JE::Render::RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                    sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                    sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                                }
                            }
                        }

                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // copy history pass
            {
                JE::Render::RPI::PassFilter passFilter = JE::Render::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceCopyHistoryPass"), (JE::Render::RPI::Scene*) nullptr);
                JE::Render::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](JE::Render::RPI::Pass* pass) -> JE::Render::RPI::PassFilterExecutionFlow
                    {
                        // enable/disable
                        pass->SetEnabled(m_ssrOptions.m_temporalFiltering);
                        return JE::Render::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }
        }
    } // namespace Features
} // namespace JE::Render
