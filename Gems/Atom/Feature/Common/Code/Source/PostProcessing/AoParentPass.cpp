/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/AmbientOcclusion/AoSettings.h>
#include <PostProcessing/FastDepthAwareBlurPasses.h>
#include <PostProcessing/GtaoPasses.h>
#include <PostProcessing/SsaoPasses.h>
#include <PostProcessing/AoParentPass.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoConstants.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>

namespace AZ
{
    namespace Render
    {
        // --- AO Parent Pass ---

        JE::Render::RPI::Ptr<AoParentPass> AoParentPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            return aznew AoParentPass(descriptor);
        }

        AoParentPass::AoParentPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ParentPass(descriptor)
        { }

        bool AoParentPass::IsEnabled() const
        {
            if (!ParentPass::IsEnabled())
            {
                return false;
            }
            const JE::Render::RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            const JE::Render::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            if (!fp)
            {
                return true;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return true;
            }
            const AoSettings* aoSettings = postProcessSettings->GetAoSettings();
            if (!aoSettings)
            {
                return true;
            }
            return aoSettings->GetEnabled();
        }

        void AoParentPass::InitializeInternal()
        {
            ParentPass::InitializeInternal();

            m_ssaoParentPass = azrtti_cast<SsaoParentPass*>(FindChildPass(Name("SsaoParent")).get());
            AZ_Assert(m_ssaoParentPass, "[AoParentPass] Could not retrieve SSAO parent pass.");

            m_gtaoParentPass = azrtti_cast<GtaoParentPass*>(FindChildPass(Name("GtaoParent")).get());
            AZ_Assert(m_gtaoParentPass, "[AoParentPass] Could not retrieve GTAO parent pass.");
        }

        void AoParentPass::FrameBeginInternal(FramePrepareParams params)
        {
            JE::Render::RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            JE::Render::RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    AoSettings* aoSettings = postProcessSettings->GetAoSettings();
                    if (aoSettings)
                    {
                        Ao::AoMethodType aoMethod = aoSettings->GetAoMethod();
                        if (aoMethod != m_currentAoMethod)
                        {
                            m_currentAoMethod = aoMethod;
                            switch (aoMethod)
                            {
                            case Ao::AoMethodType::SSAO:
                                m_ssaoParentPass->SetEnabled(true);
                                m_gtaoParentPass->SetEnabled(false);
                                // Break connection from GTAO and replace with connection from SSAO
                                ChangeConnection(Name("Output"), m_gtaoParentPass, Name("Output"));
                                ChangeConnection(Name("Output"), m_ssaoParentPass, Name("Output"));
                                break;
                            case Ao::AoMethodType::GTAO:
                                m_gtaoParentPass->SetEnabled(true);
                                m_ssaoParentPass->SetEnabled(false);
                                // Break connection from SSAO and replace with connection from GTAO
                                ChangeConnection(Name("Output"), m_ssaoParentPass, Name("Output"));
                                ChangeConnection(Name("Output"), m_gtaoParentPass, Name("Output"));
                                break;
                            }
                        }
                    }
                }
            }

            ParentPass::FrameBeginInternal(params);
        }
    }   // namespace Render
}   // namespace AZ
