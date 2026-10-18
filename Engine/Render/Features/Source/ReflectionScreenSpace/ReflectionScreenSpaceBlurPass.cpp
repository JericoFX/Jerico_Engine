/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceBlurPass.h"
#include "ReflectionScreenSpaceBlurChildPass.h"
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <JE/Render/RPI/Reflect/Pass/FullscreenTrianglePassData.h>
#include <JE/Render/RPI/Reflect/Pass/PassName.h>
#include <JE/Render/RPI/Public/Pass/PassDefines.h>
#include <JE/Render/RPI/Public/Pass/PassFactory.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Reflect/Pass/PassRequest.h>
#include <JE/Render/RPI/Public/Pass/PassFilter.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<ReflectionScreenSpaceBlurPass> ReflectionScreenSpaceBlurPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<ReflectionScreenSpaceBlurPass> pass = aznew ReflectionScreenSpaceBlurPass(descriptor);
            return pass;
        }

        ReflectionScreenSpaceBlurPass::ReflectionScreenSpaceBlurPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::ParentPass(descriptor)
        {
        }

        void ReflectionScreenSpaceBlurPass::ResetInternal()
        {
            RemoveChildren();
        }

        void ReflectionScreenSpaceBlurPass::CreateChildPassesInternal()
        {
            JE::Render::RPI::PassSystemInterface* passSystem = JE::Render::RPI::PassSystemInterface::Get();

            m_verticalBlurChildPasses.clear();
            m_horizontalBlurChildPasses.clear();

            // load shaders
            constexpr const char* verticalBlurShaderFilePath = "Shaders/Reflections/ReflectionScreenSpaceBlurVertical.azshader";
            AZ::Data::Instance<JE::Render::RPI::Shader> verticalBlurShader = JE::Render::RPI::LoadCriticalShader(verticalBlurShaderFilePath);
            if (verticalBlurShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ReflectionScreenSpaceBlurPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), verticalBlurShaderFilePath);
                return;
            }

            constexpr const char* horizontalBlurShaderFilePath = "Shaders/Reflections/ReflectionScreenSpaceBlurHorizontal.azshader";
            AZ::Data::Instance<JE::Render::RPI::Shader> horizontalBlurShader = JE::Render::RPI::LoadCriticalShader(horizontalBlurShaderFilePath);
            if (horizontalBlurShader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ReflectionScreenSpaceBlurPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(), horizontalBlurShaderFilePath);
                return;
            }

            // load pass templates
            const AZStd::shared_ptr<const JE::Render::RPI::PassTemplate> blurVerticalPassTemplate = JE::Render::RPI::PassSystemInterface::Get()->GetPassTemplate(AZ::Name("ReflectionScreenSpaceBlurVerticalPassTemplate"));
            const AZStd::shared_ptr<const JE::Render::RPI::PassTemplate> blurHorizontalPassTemplate = JE::Render::RPI::PassSystemInterface::Get()->GetPassTemplate(AZ::Name("ReflectionScreenSpaceBlurHorizontalPassTemplate"));

            // create pass descriptors
            JE::Render::RPI::PassDescriptor verticalBlurChildDesc;
            verticalBlurChildDesc.m_passTemplate = blurVerticalPassTemplate;

            JE::Render::RPI::PassDescriptor horizontalBlurChildDesc;
            horizontalBlurChildDesc.m_passTemplate = blurHorizontalPassTemplate;

            // add child passes to perform the vertical and horizontal Gaussian blur for each roughness mip level
            for (uint32_t mip = 0; mip < m_mipLevels - 1; ++mip)
            {
                // create Vertical blur child passes
                {
                    AZStd::string verticalBlurChildPassName = AZStd::string::format("ReflectionScreenSpace_VerticalMipBlur%d", mip + 1);
                    verticalBlurChildDesc.m_passName = AZ::Name(verticalBlurChildPassName);

                    JE::Render::RPI::Ptr<JE::Render::Features::ReflectionScreenSpaceBlurChildPass> verticalBlurChildPass = passSystem->CreatePass<JE::Render::Features::ReflectionScreenSpaceBlurChildPass>(verticalBlurChildDesc);
                    verticalBlurChildPass->SetType(JE::Render::Features::ReflectionScreenSpaceBlurChildPass::PassType::Vertical);
                    verticalBlurChildPass->SetMipLevel(mip + 1);
                    m_verticalBlurChildPasses.push_back(verticalBlurChildPass);

                    AddChild(verticalBlurChildPass);
                }

                // create Horizontal blur child passes
                {
                    AZStd::string horizontalBlurChildPassName = AZStd::string::format("ReflectionScreenSpace_HorizonalMipBlur%d", mip + 1);
                    horizontalBlurChildDesc.m_passName = AZ::Name(horizontalBlurChildPassName);

                    JE::Render::RPI::Ptr<JE::Render::Features::ReflectionScreenSpaceBlurChildPass> horizontalBlurChildPass = passSystem->CreatePass<JE::Render::Features::ReflectionScreenSpaceBlurChildPass>(horizontalBlurChildDesc);
                    horizontalBlurChildPass->SetType(JE::Render::Features::ReflectionScreenSpaceBlurChildPass::PassType::Horizontal);
                    horizontalBlurChildPass->SetMipLevel(mip + 1);
                    m_horizontalBlurChildPasses.push_back(horizontalBlurChildPass);

                    AddChild(horizontalBlurChildPass);
                }
            }
        }

        void ReflectionScreenSpaceBlurPass::BuildInternal()
        {
            RemoveChildren();
            m_flags.m_createChildren = true;

            // retrieve the reflection attachment
            JE::Render::RPI::PassAttachment* reflectionImageAttachment = GetInputOutputBinding(0).GetAttachment().get();
            m_imageSize = reflectionImageAttachment->m_descriptor.m_image.m_size;
            m_mipLevels = aznumeric_cast<uint32_t>(reflectionImageAttachment->m_descriptor.m_image.m_mipLevels);

            // create transient attachments, one for each blur mip level
            AZStd::vector<JE::Render::RPI::PassAttachment*> transientPassAttachments;
            for (uint32_t mip = 1; mip <= m_mipLevels - 1; ++mip)
            {
                AZ::RHI::Size mipSize = m_imageSize.GetReducedMip(mip);

                AZ::RHI::ImageBindFlags imageBindFlags = AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderReadWrite | AZ::RHI::ImageBindFlags::CopyRead;
                auto transientImageDesc = AZ::RHI::ImageDescriptor::Create2D(imageBindFlags, mipSize.m_width, mipSize.m_height, AZ::RHI::Format::R16G16B16A16_FLOAT);

                JE::Render::RPI::PassAttachment* transientPassAttachment = aznew JE::Render::RPI::PassAttachment();
                AZStd::string transientAttachmentName = AZStd::string::format("%s.ReflectionScreenSpace_BlurImage%d", GetPathName().GetCStr(), mip);
                transientPassAttachment->m_name = transientAttachmentName;
                transientPassAttachment->m_path = transientAttachmentName;
                transientPassAttachment->m_lifetime = AZ::RHI::AttachmentLifetimeType::Transient;
                transientPassAttachment->m_descriptor = transientImageDesc;
                transientPassAttachments.push_back(transientPassAttachment);

                m_ownedAttachments.push_back(transientPassAttachment);
            }

            // call ParentPass::BuildInternal() first to configure the slots and auto-add the empty bindings,
            // then we will assign attachments to the bindings
            ParentPass::BuildInternal();

            // setup attachment bindings on vertical blur child passes
            uint32_t attachmentIndex = 0;
            for (auto& verticalBlurChildPass : m_verticalBlurChildPasses)
            {
                // mip0 source input
                JE::Render::RPI::PassAttachmentBinding& inputAttachmentBinding = verticalBlurChildPass->GetInputBinding(0);
                inputAttachmentBinding.SetAttachment(reflectionImageAttachment);
                inputAttachmentBinding.m_connectedBinding = &GetInputOutputBinding(0);

                // mipN transient output
                JE::Render::RPI::PassAttachmentBinding& outputAttachmentBinding = verticalBlurChildPass->GetOutputBinding(0);
                outputAttachmentBinding.SetAttachment(transientPassAttachments[attachmentIndex]);

                verticalBlurChildPass->UpdateConnectedBindings();

                attachmentIndex++;
            }

            // setup attachment bindings on horizontal blur child passes
            attachmentIndex = 0;
            for (auto& horizontalBlurChildPass : m_horizontalBlurChildPasses)
            {
                JE::Render::RPI::PassAttachmentBinding& inputAttachmentBinding = horizontalBlurChildPass->GetInputBinding(0);
                inputAttachmentBinding.SetAttachment(transientPassAttachments[attachmentIndex]);
                inputAttachmentBinding.m_connectedBinding = &m_verticalBlurChildPasses[attachmentIndex]->GetOutputBinding(0);

                JE::Render::RPI::PassAttachmentBinding& outputAttachmentBinding = horizontalBlurChildPass->GetOutputBinding(0);
                uint32_t mipLevel = attachmentIndex + 1;
                AZ::RHI::ImageViewDescriptor outputViewDesc;
                outputViewDesc.m_mipSliceMin = static_cast<int16_t>(mipLevel);
                outputViewDesc.m_mipSliceMax = static_cast<int16_t>(mipLevel);
                outputAttachmentBinding.m_unifiedScopeDesc.SetAsImage(outputViewDesc);
                outputAttachmentBinding.SetAttachment(reflectionImageAttachment);

                horizontalBlurChildPass->UpdateConnectedBindings();

                attachmentIndex++;
            }
        }

        void ReflectionScreenSpaceBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            // get input attachment size
            JE::Render::RPI::PassAttachment* inputAttachment = GetInputOutputBinding(0).GetAttachment().get();
            AZ_Assert(inputAttachment, "ReflectionScreenSpaceBlurChildPass: Input binding has no attachment!");

            AZ::RHI::Size size = inputAttachment->m_descriptor.m_image.m_size;
            if (m_imageSize != size)
            {
                m_imageSize = size;

                uint32_t mip = 1;
                for (auto& ownedAttachment : m_ownedAttachments)
                {
                    AZ::RHI::Size mipSize = m_imageSize.GetReducedMip(mip);
                    ownedAttachment->m_descriptor.m_image.m_size = mipSize;
                    mip++;
                }
            }

            ParentPass::FrameBeginInternal(params);
        }
    } // namespace Features
} // namespace JE::Render
