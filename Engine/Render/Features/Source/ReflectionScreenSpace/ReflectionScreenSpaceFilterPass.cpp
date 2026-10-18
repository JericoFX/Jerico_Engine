/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceFilterPass.h"
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
        JE::Render::RPI::Ptr<ReflectionScreenSpaceFilterPass> ReflectionScreenSpaceFilterPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ReflectionScreenSpaceFilterPass> pass = aznew ReflectionScreenSpaceFilterPass(descriptor);
            return pass;
        }

        ReflectionScreenSpaceFilterPass::ReflectionScreenSpaceFilterPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceFilterPass::CreateHistoryAttachmentImage(JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment>& historyAttachment)
        {
            AZ::Data::Instance<JE::Render::RPI::AttachmentImagePool> pool = JE::Render::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            m_currentHistoryAttachmentImage = (m_currentHistoryAttachmentImage + 1) % ImageFrameCount;

            AZ::RHI::ImageViewDescriptor viewDesc = AZ::RHI::ImageViewDescriptor::Create(AZ::RHI::Format::R16G16B16A16_FLOAT, 0, 0);
            AZ::RHI::ClearValue clearValue = AZ::RHI::ClearValue::CreateVector4Float(0, 0, 0, 0);
            m_historyAttachmentImage[m_currentHistoryAttachmentImage] = JE::Render::RPI::AttachmentImage::Create(*pool.get(), historyAttachment->m_descriptor.m_image, AZ::Name(historyAttachment->m_path.GetCStr()), &clearValue, &viewDesc);

            historyAttachment->m_importedResource = m_historyAttachmentImage[m_currentHistoryAttachmentImage];
        }

        void ReflectionScreenSpaceFilterPass::BuildInternal()
        {
            // retrieve the input reflection image descriptor to get the size
            AZ::RHI::ImageDescriptor reflectionImageDesc = m_ownedAttachments[0]->m_descriptor.m_image;

            // create the history attachment
            AZ::RHI::ImageBindFlags imageBindFlags = AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderReadWrite;
            AZ::RHI::ImageDescriptor historyImageDesc = AZ::RHI::ImageDescriptor::Create2D(imageBindFlags, reflectionImageDesc.m_size.m_width, reflectionImageDesc.m_size.m_height, AZ::RHI::Format::R16G16B16A16_FLOAT);

            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> historyAttachment = aznew JE::Render::RPI::PassAttachment();
            AZStd::string historyAttachmentName = AZStd::string::format("%s.ReflectionScreenSpace_HistoryImage", GetPathName().GetCStr());
            historyAttachment->m_name = historyAttachmentName;
            historyAttachment->m_path = historyAttachmentName;
            historyAttachment->m_lifetime = AZ::RHI::AttachmentLifetimeType::Imported;
            historyAttachment->m_descriptor = historyImageDesc;
            m_ownedAttachments.push_back(historyAttachment);

            CreateHistoryAttachmentImage(historyAttachment);

            // bind the attachment
            m_historyAttachmentBinding = FindAttachmentBinding(AZ::Name("History"));
            m_historyAttachmentBinding->SetAttachment(historyAttachment);

            // retrieve reflection input attachment
            m_reflectionInputAttachmentBinding = FindAttachmentBinding(AZ::Name("ScreenSpaceReflectionInputOutput"));

            FullscreenTrianglePass::BuildInternal();
        }

        void ReflectionScreenSpaceFilterPass::CompileResources([[maybe_unused]] const AZ::RHI::FrameGraphCompileContext& context)
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            SpecularReflectionsFeatureProcessor* specularReflectionsFeatureProcessor = scene->GetFeatureProcessor<SpecularReflectionsFeatureProcessor>();
            AZ_Assert(specularReflectionsFeatureProcessor, "ReflectionScreenSpaceFilterPass requires the SpecularReflectionsFeatureProcessor");

            // retrieve reflection input attachment
            const JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment>& reflectionInputAttachment = m_reflectionInputAttachmentBinding->GetAttachment();
            AZ_Assert(reflectionInputAttachment, "ReflectionScreenSpaceFilterPass: Reflection input binding has no attachment!");
            AZ::RHI::ImageDescriptor& reflectionImageDescriptor = reflectionInputAttachment->m_descriptor.m_image;

            // retrieve output attachment
            JE::Render::RPI::PassAttachmentBinding& outputBinding = GetOutputBinding(0);
            const JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment>& outputAttachment = outputBinding.GetAttachment();
            AZ_Assert(outputAttachment, "ReflectionScreenSpaceFilterPass: Output binding has no attachment!");

            AZ::RHI::ImageDescriptor& outputImageDescriptor = outputAttachment->m_descriptor.m_image;

            const SSROptions& ssrOptions = specularReflectionsFeatureProcessor->GetSSROptions();

            m_shaderResourceGroup->SetConstant(m_invOutputScaleNameIndex, 1.0f / ssrOptions.GetOutputScale());
            m_shaderResourceGroup->SetConstant(m_outputWidthNameIndex, outputImageDescriptor.m_size.m_width);
            m_shaderResourceGroup->SetConstant(m_outputHeightNameIndex, outputImageDescriptor.m_size.m_height);
            m_shaderResourceGroup->SetConstant(m_mipLevelsNameIndex, aznumeric_cast<uint32_t>(reflectionImageDescriptor.m_mipLevels));
            m_shaderResourceGroup->SetConstant(m_coneTracingNameIndex, ssrOptions.m_coneTracing);
            m_shaderResourceGroup->SetConstant(m_rayTracingNameIndex, ssrOptions.IsRayTracingEnabled());
            m_shaderResourceGroup->SetConstant(m_temporalFilteringNameIndex, ssrOptions.m_temporalFiltering);
            m_shaderResourceGroup->SetConstant(m_invTemporalFilteringStrengthNameIndex, 1.0f / ssrOptions.m_temporalFilteringStrength);
            m_shaderResourceGroup->SetConstant(m_maxRoughnessNameIndex, ssrOptions.m_maxRoughness);
            m_shaderResourceGroup->SetConstant(m_roughnessBiasNameIndex, ssrOptions.m_roughnessBias);
            m_shaderResourceGroup->SetConstant(m_luminanceClampNameIndex, ssrOptions.m_luminanceClamp);
            m_shaderResourceGroup->SetConstant(m_maxLuminanceNameIndex, ssrOptions.m_maxLuminance);

            FullscreenTrianglePass::CompileResources(context);
        }

        void ReflectionScreenSpaceFilterPass::FrameEndInternal()
        {
            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> historyAttachment = m_historyAttachmentBinding->GetAttachment();
            historyAttachment->Update();

            AZ::RHI::Size& historyImageSize = historyAttachment->m_descriptor.m_image.m_size;
            AZ::RHI::Size& reflectionImageSize = m_ownedAttachments[0]->m_descriptor.m_image.m_size;

            if (historyImageSize != reflectionImageSize)
            {
                historyImageSize = reflectionImageSize;

                CreateHistoryAttachmentImage(historyAttachment);
                m_historyAttachmentBinding->SetAttachment(historyAttachment);
            }

            FullscreenTrianglePass::FrameEndInternal();
        }

    } // namespace Features
} // namespace JE::Render
