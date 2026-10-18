/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpacePass.h"
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <SpecularReflections/SpecularReflectionsFeatureProcessor.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<ReflectionScreenSpacePass> ReflectionScreenSpacePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ReflectionScreenSpacePass> pass = aznew ReflectionScreenSpacePass(descriptor);
            return pass;
        }

        ReflectionScreenSpacePass::ReflectionScreenSpacePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ParentPass(descriptor)
        {
        }

        bool ReflectionScreenSpacePass::IsEnabled() const
        {
            if (!ParentPass::IsEnabled())
            {
                return false;
            }

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            SpecularReflectionsFeatureProcessor* specularReflectionsFeatureProcessor = scene->GetFeatureProcessor<SpecularReflectionsFeatureProcessor>();
            if (!specularReflectionsFeatureProcessor)
            {
                return false;
            }

            // delay for a few frames to ensure that the previous frame texture is populated
            static const uint32_t FrameDelay = 10;
            if (m_frameDelayCount < FrameDelay)
            {
                m_frameDelayCount++;
                return false;
            }

            return true;
        }
    } // namespace Features
} // namespace JE::Render
