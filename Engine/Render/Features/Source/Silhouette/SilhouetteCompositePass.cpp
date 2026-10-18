/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Silhouette/SilhouetteCompositePass.h>
#include <Silhouette/SilhouetteFeatureProcessor.h>
#include <JE/Render/RPI/Public/Scene.h>

namespace JE::Render::Features
{
    SilhouetteCompositePass::SilhouetteCompositePass(const JE::Render::RPI::PassDescriptor& descriptor)
        : JE::Render::RPI::FullscreenTrianglePass(descriptor)
    {
    }

    void SilhouetteCompositePass::InitializeInternal()
    {
        JE::Render::RPI::FullscreenTrianglePass::InitializeInternal();
        auto* scene = GetScene();
        if (!scene)
        {
            return;
        }

        m_featureProcessor = scene->GetFeatureProcessor<SilhouetteFeatureProcessor>();
    }

    JE::Render::RPI::Ptr<SilhouetteCompositePass> SilhouetteCompositePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
    {
        JE::Render::RPI::Ptr<SilhouetteCompositePass> pass = aznew SilhouetteCompositePass(descriptor);
        return pass;
    }

    void SilhouetteCompositePass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        JE::Render::RPI::FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);
        if (m_featureProcessor)
        {
            // We check if we need to do the composite pass
            // We don't enable/disable the pass because that causes rebuilding the renderpass when subpasses are being merged.
            frameGraph.SetEstimatedItemCount(m_featureProcessor->NeedsCompositePass() ? 1 : 0);
        }
    }

    void SilhouetteCompositePass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        const auto* scope = static_cast<const SilhouetteCompositePass*>(this)->GetScope();
        if (scope->GetEstimatedItemCount())
        {
            JE::Render::RPI::FullscreenTrianglePass::BuildCommandListInternal(context);
        }
    }

} // namespace JE::Render::Features

