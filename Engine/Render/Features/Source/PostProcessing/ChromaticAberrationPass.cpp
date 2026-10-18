/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/ChromaticAberrationPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<ChromaticAberrationPass> ChromaticAberrationPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ChromaticAberrationPass> pass = aznew ChromaticAberrationPass(descriptor);
            return pass;
        }

        ChromaticAberrationPass::ChromaticAberrationPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ComputePass(descriptor)
        {
        }

        bool ChromaticAberrationPass::IsEnabled() const
        {
            if (!ComputePass::IsEnabled())
            {
                return false;
            }
            const JE::Render::RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            if (!fp)
            {
                return false;
            }
            const JE::Render::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            const ChromaticAberrationSettings* chromaticAberrationSettings = postProcessSettings->GetChromaticAberrationSettings();
            if (!chromaticAberrationSettings)
            {
                return false;
            }
            return chromaticAberrationSettings->GetEnabled();
        }

        void ChromaticAberrationPass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in ChromaticAberration.azsl
            struct Constants
            {
                AZStd::array<AZ::u32, 2> m_outputSize;
                AZStd::array<float, 2> m_outputCenter;
                float m_strength = ChromaticAberration::DefaultStrength;
                float m_blend = ChromaticAberration::DefaultBlend;
            } constants{};

            JE::Render::RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            if (fp)
            {
                JE::Render::RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    ChromaticAberrationSettings* chromaticAberrationSettings = postProcessSettings->GetChromaticAberrationSettings();
                    if (chromaticAberrationSettings)
                    {
                        constants.m_strength = chromaticAberrationSettings->GetStrength();
                        constants.m_blend = chromaticAberrationSettings->GetBlend();
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "ChromaticAberrationPass: No output bindings!");
            JE::Render::RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "ChromaticAberrationPass: Output binding has no attachment!");
            AZ::RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            constants.m_outputSize[0] = size.m_width;
            constants.m_outputSize[1] = size.m_height;
            constants.m_outputCenter[0] = (size.m_width - 1) * 0.5f;
            constants.m_outputCenter[1] = (size.m_height -1) * 0.5f;

            m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);

            JE::Render::RPI::ComputePass::FrameBeginInternal(params);
        }
    } // namespace Features
} // namespace JE::Render
