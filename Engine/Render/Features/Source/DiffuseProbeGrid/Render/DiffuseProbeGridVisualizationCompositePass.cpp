/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridVisualizationCompositePass.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<DiffuseProbeGridVisualizationCompositePass> DiffuseProbeGridVisualizationCompositePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DiffuseProbeGridVisualizationCompositePass> pass = aznew DiffuseProbeGridVisualizationCompositePass(descriptor);
            return pass;
        }

        DiffuseProbeGridVisualizationCompositePass::DiffuseProbeGridVisualizationCompositePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
            if (!JE_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                SetEnabled(false);
            }
        }

        bool DiffuseProbeGridVisualizationCompositePass::IsEnabled() const
        {
            if (!FullscreenTrianglePass::IsEnabled())
            {
                return false;
            }

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (diffuseProbeGridFeatureProcessor)
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
                {
                    if (diffuseProbeGrid->GetVisualizationEnabled())
                    {
                        return true;
                    }
                }
            }

            return false;
        }

    }   // namespace RPI
}   // namespace JE::Render
