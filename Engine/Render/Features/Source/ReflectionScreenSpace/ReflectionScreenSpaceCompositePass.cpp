/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceCompositePass.h"
#include "ReflectionScreenSpaceBlurPass.h"
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/Features/SpecularReflections/SpecularReflectionsFeatureProcessorInterface.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<ReflectionScreenSpaceCompositePass> ReflectionScreenSpaceCompositePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ReflectionScreenSpaceCompositePass> pass = aznew ReflectionScreenSpaceCompositePass(descriptor);
            return pass;
        }

        ReflectionScreenSpaceCompositePass::ReflectionScreenSpaceCompositePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceCompositePass::CompileResources([[maybe_unused]] const AZ::RHI::FrameGraphCompileContext& context)
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            SpecularReflectionsFeatureProcessorInterface* specularReflectionsFeatureProcessor = scene->GetFeatureProcessor<SpecularReflectionsFeatureProcessorInterface>();
            AZ_Assert(specularReflectionsFeatureProcessor, "ReflectionScreenSpaceCompositePass requires the SpecularReflectionsFeatureProcessor");

            JE::Render::RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();
            AZ_Assert(outputAttachment, "ReflectionScreenSpaceCompositePass: Output binding has no attachment!");
            AZ::RHI::Size outputImageSize = outputAttachment->m_descriptor.m_image.m_size;

            const SSROptions& ssrOptions = specularReflectionsFeatureProcessor->GetSSROptions();

            m_shaderResourceGroup->SetConstant(m_outputScaleNameIndex, ssrOptions.GetOutputScale());
            m_shaderResourceGroup->SetConstant(m_outputWidthNameIndex, outputImageSize.m_width);
            m_shaderResourceGroup->SetConstant(m_outputHeightNameIndex, outputImageSize.m_height);
            m_shaderResourceGroup->SetConstant(m_maxRoughnessNameIndex, ssrOptions.m_maxRoughness);

            FullscreenTrianglePass::CompileResources(context);
        }
    } // namespace Features
} // namespace JE::Render
