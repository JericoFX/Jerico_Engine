/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceTracePass.h"
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>
#include <SpecularReflections/SpecularReflectionsFeatureProcessor.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<ReflectionScreenSpaceTracePass> ReflectionScreenSpaceTracePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ReflectionScreenSpaceTracePass> pass = aznew ReflectionScreenSpaceTracePass(descriptor);
            return pass;
        }

        ReflectionScreenSpaceTracePass::ReflectionScreenSpaceTracePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceTracePass::BuildInternal()
        {
            AZ::Data::Instance<JE::Render::RPI::AttachmentImagePool> pool = JE::Render::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            // retrieve the previous frame image attachment from the pass
            AZ_Assert(m_ownedAttachments.size() == 3, "ReflectionScreenSpaceTracePass must have the following attachment images defined: ReflectionImage, TraceCoordsImage, and PreviousFrameImage");
            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> previousFrameImageAttachment = m_ownedAttachments[2];

            // update the image attachment descriptor to sync up size and format
            previousFrameImageAttachment->Update();

            // change the lifetime since we want it to live between frames
            previousFrameImageAttachment->m_lifetime = AZ::RHI::AttachmentLifetimeType::Imported;

            // set the bind flags
            AZ::RHI::ImageDescriptor& imageDesc = previousFrameImageAttachment->m_descriptor.m_image;
            imageDesc.m_bindFlags |= AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderReadWrite;

            // create the image attachment
            AZ::RHI::ClearValue clearValue = AZ::RHI::ClearValue::CreateVector4Float(0, 0, 0, 0);
            m_previousFrameImageAttachment = JE::Render::RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(previousFrameImageAttachment->m_path.GetCStr()), &clearValue, nullptr);

            previousFrameImageAttachment->m_path = m_previousFrameImageAttachment->GetAttachmentId();
            previousFrameImageAttachment->m_importedResource = m_previousFrameImageAttachment;
        }

        void ReflectionScreenSpaceTracePass::CompileResources([[maybe_unused]] const AZ::RHI::FrameGraphCompileContext& context)
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            SpecularReflectionsFeatureProcessor* specularReflectionsFeatureProcessor = scene->GetFeatureProcessor<SpecularReflectionsFeatureProcessor>();
            AZ_Assert(specularReflectionsFeatureProcessor, "ReflectionScreenSpaceTracePass requires the SpecularReflectionsFeatureProcessor");

            JE::Render::RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();
            AZ_Assert(outputAttachment, "ReflectionScreenSpaceTracePass: Output binding has no attachment!");
            AZ::RHI::Size outputImageSize = outputAttachment->m_descriptor.m_image.m_size;

            const SSROptions& ssrOptions = specularReflectionsFeatureProcessor->GetSSROptions();

            m_shaderResourceGroup->SetConstant(m_invOutputScaleNameIndex, 1.0f / ssrOptions.GetOutputScale());
            m_shaderResourceGroup->SetConstant(m_outputWidthNameIndex, outputImageSize.m_width);
            m_shaderResourceGroup->SetConstant(m_outputHeightNameIndex, outputImageSize.m_height);
            m_shaderResourceGroup->SetConstant(m_rayTracingEnabledNameIndex, ssrOptions.IsRayTracingEnabled());
            m_shaderResourceGroup->SetConstant(m_rayTraceFallbackDataNameIndex, ssrOptions.IsRayTracingFallbackEnabled());
            m_shaderResourceGroup->SetConstant(m_maxRayDistanceNameIndex, ssrOptions.m_maxRayDistance);
            m_shaderResourceGroup->SetConstant(m_maxDepthThresholdNameIndex, ssrOptions.m_maxDepthThreshold);
            m_shaderResourceGroup->SetConstant(m_maxRoughnessNameIndex, ssrOptions.m_maxRoughness);
            m_shaderResourceGroup->SetConstant(m_roughnessBiasNameIndex, ssrOptions.m_roughnessBias);

            FullscreenTrianglePass::CompileResources(context);
        }

    } // namespace Features
} // namespace JE::Render
