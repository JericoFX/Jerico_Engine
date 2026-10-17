/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Pass/Specific/MSAAResolveFullScreenPass.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>

namespace JE::Render
{
    namespace RPI
    {
        JE::Render::RPI::Ptr<MSAAResolveFullScreenPass> MSAAResolveFullScreenPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<MSAAResolveFullScreenPass> pass = aznew MSAAResolveFullScreenPass(descriptor);
            return pass;
        }

        MSAAResolveFullScreenPass::MSAAResolveFullScreenPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        bool MSAAResolveFullScreenPass::IsEnabled() const
        {
            // check Pass base class first to see if the Pass is explicitly disabled
            if (!Pass::IsEnabled())
            {
                return false;
            }

            // check render pipeline MSAA sample count
            return (m_pipeline->GetRenderSettings().m_multisampleState.m_samples > 1);
        }
    }   // namespace RPI
}   // namespace AZ
