/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>

#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/Bloom/BloomSettings.h>
#include <PostProcessing/BloomParentPass.h>

namespace AZ
{
    namespace Render
    {

        AZ_CVAR(bool, r_enableBloom, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Enable bloom effect support");

        JE::Render::RPI::Ptr<BloomParentPass> BloomParentPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<BloomParentPass> pass = aznew BloomParentPass(descriptor);
            return pass;
        }

        BloomParentPass::BloomParentPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ParentPass(descriptor)
        { }

        bool BloomParentPass::IsEnabled() const
        {
            if (!r_enableBloom)
            {
                return false;
            }

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
                return false;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            const BloomSettings* bloomSettings = postProcessSettings->GetBloomSettings();
            return (bloomSettings != nullptr) && bloomSettings->GetEnabled();
        }

    }   // namespace Render
}   // namespace AZ
