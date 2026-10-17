/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DiffuseProbeGridDownsamplePass.h"
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>

namespace AZ
{
    namespace Render
    {
        JE::Render::RPI::Ptr<DiffuseProbeGridDownsamplePass> DiffuseProbeGridDownsamplePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DiffuseProbeGridDownsamplePass> pass = aznew DiffuseProbeGridDownsamplePass(descriptor);
            return pass;
        }

        DiffuseProbeGridDownsamplePass::DiffuseProbeGridDownsamplePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        bool DiffuseProbeGridDownsamplePass::IsEnabled() const
        {
            if (!Base::IsEnabled())
            {
                return false;
            }

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            // only enabled if there are DiffuseProbeGrids present in the scene
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            return (diffuseProbeGridFeatureProcessor && !diffuseProbeGridFeatureProcessor->GetProbeGrids().empty());
        }

    }   // namespace RPI
}   // namespace AZ
