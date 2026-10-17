/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>

#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/AttachmentReadback.h>
#include <JE/Render/RPI/Public/Pass/Specific/ImageAttachmentPreviewPass.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPIUtils.h>

#include <JE/Render/RPI/Reflect/Shader/ShaderOptionGroup.h>

namespace JE::Render
{
    namespace RPI
    {
        void ImageAttachmentCopy::SetImageAttachment(AZ::RHI::AttachmentId srcAttachmentId, AZ::RHI::AttachmentId destAttachmentId)
        {
            m_srcAttachmentId = srcAttachmentId;
            m_destAttachmentId = destAttachmentId;

            // Use the unique destination attachment id as scope id
            InitScope(m_destAttachmentId);

            // Clear the previous attachment and copy item
            m_copyItem = {};
            m_destImage = nullptr;
        }

        void ImageAttachmentCopy::Reset()
        {
            m_srcAttachmentId = AZ::RHI::AttachmentId{};
            m_destAttachmentId = AZ::RHI::AttachmentId{};
            m_copyItem = {};
            m_destImage = nullptr;
        }

        void ImageAttachmentCopy::InvalidateDestImage()
        {
            m_destImage = nullptr;
        }

        void ImageAttachmentCopy::FrameBegin(Pass::FramePrepareParams params)
        {
            AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();

            if (m_srcAttachmentId.IsEmpty())
            {
                return;
            }

            // Return if the source attachment is not imported
            if (!attachmentDatabase.IsAttachmentValid(m_srcAttachmentId))
            {
                Reset();
                return;
            }

            if (!m_destImage)
            {
                AZ::Data::Instance<AttachmentImagePool> pool = ImageSystemInterface::Get()->GetSystemAttachmentPool();
                AZ::RHI::ImageDescriptor imageDesc = attachmentDatabase.GetImageDescriptor(m_srcAttachmentId);
                // add read flag since the image will always be read by ImageAttachmentPreviewPass
                imageDesc.m_bindFlags |= AZ::RHI::ImageBindFlags::ShaderRead;
                imageDesc.m_arraySize = 1;

                AZ::Name copyName = AZ::Name( AZStd::string::format("%s_%s", m_srcAttachmentId.GetCStr(), "Copy") );
                m_destImage = AttachmentImage::Create(*pool.get(), imageDesc, copyName, nullptr, nullptr);
            }

            if (!m_destImage)
            {
                AZ_Warning("ImageAttachmentCopy", false, "Failed to create a copy to preview attachment [%s]", m_srcAttachmentId.GetCStr());
                Reset();
                return;
            }
            // Import this scope producer
            params.m_frameGraphBuilder->ImportScopeProducer(*this);
            attachmentDatabase.ImportImage(m_destAttachmentId, m_destImage->GetRHIImage());
        }

        void ImageAttachmentCopy::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            AZ::RHI::ImageScopeAttachmentDescriptor srcDescriptor{ m_srcAttachmentId };
            frameGraph.UseCopyAttachment(srcDescriptor, AZ::RHI::ScopeAttachmentAccess::Read);
            AZ::RHI::ImageScopeAttachmentDescriptor destDescriptor{ m_destAttachmentId };
            frameGraph.UseCopyAttachment(destDescriptor, AZ::RHI::ScopeAttachmentAccess::Write);

            frameGraph.SetEstimatedItemCount(1);
        }

        void ImageAttachmentCopy::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            // copy descriptor for copying image
            AZ::RHI::CopyImageDescriptor copyImage;
            const AZ::RHI::Image* image = context.GetImage(m_srcAttachmentId);
            copyImage.m_sourceImage = image;
            copyImage.m_sourceSize = image->GetDescriptor().m_size;
            copyImage.m_sourceSubresource.m_arraySlice = m_sourceArraySlice;
            if ((image->GetAspectFlags() & AZ::RHI::ImageAspectFlags::Depth) != AZ::RHI::ImageAspectFlags::None)
            {
                // Only the depth is previewed, so no need to copy a potential stencil part as well
                copyImage.m_sourceSubresource.m_aspect = AZ::RHI::ImageAspect::Depth;
                copyImage.m_destinationSubresource.m_aspect = AZ::RHI::ImageAspect::Depth;
            }
            copyImage.m_destinationImage = context.GetImage(m_destAttachmentId);
            
            m_copyItem = copyImage;
        }

        void ImageAttachmentCopy::BuildCommandList(const AZ::RHI::FrameGraphExecuteContext& context)
        {
            context.GetCommandList()->Submit(m_copyItem.GetDeviceCopyItem(context.GetDeviceIndex()));
        }

        Ptr<ImageAttachmentPreviewPass> ImageAttachmentPreviewPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<ImageAttachmentPreviewPass> imageAttachmentPreviewPass = aznew ImageAttachmentPreviewPass(descriptor);
            return imageAttachmentPreviewPass;
        }

        ImageAttachmentPreviewPass::ImageAttachmentPreviewPass(const PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            InitScope(AZ::RHI::ScopeId(GetPathName()));
        }

        ImageAttachmentPreviewPass::~ImageAttachmentPreviewPass()
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }

        void ImageAttachmentPreviewPass::PreviewImageAttachmentForPass(Pass* pass, const PassAttachment* passAttachment, RenderPipeline* previewOutputPipeline, AZ::u32 imageArraySlice)
        {
            if (passAttachment->GetAttachmentType() != AZ::RHI::AttachmentType::Image)
            {
                return;
            }

            ClearPreviewAttachment();
            
            // find the attachment in pass's attachment binding
            uint32_t bindingIndex = 0;
            for (auto& binding : pass->GetAttachmentBindings())
            {
                if (passAttachment == binding.GetAttachment())
                {
                    AZ::RHI::AttachmentId attachmentId = binding.GetAttachment()->GetAttachmentId();
                    
                    // Append slot index and pass name so the read back's name won't be same as the attachment used in other passes.
                    AZStd::string readbackName = AZStd::string::format("%s_%d_%s", attachmentId.GetCStr(),
                        bindingIndex, GetName().GetCStr());
                    m_attachmentCopy = AZStd::make_shared<JE::Render::RPI::ImageAttachmentCopy>();
                    m_attachmentCopy->SetImageAttachment(attachmentId, AZ::Name(readbackName));

                    pass->m_attachmentCopy = m_attachmentCopy;
                    break;
                }
                bindingIndex++;
            }

            if (bindingIndex == pass->GetAttachmentBindings().size())
            {
                AZ_Warning("RPI", false, "failed to find the attachment %s", passAttachment->GetAttachmentId().GetCStr());
                return;
            }

            m_updateDrawData = true;
            m_imageAttachmentId = m_attachmentCopy->m_destAttachmentId;
            m_attachmentCopy->m_sourceArraySlice = AZ::u16(imageArraySlice);

            // Set the output of this pass to write to the pipeline output
            if (!m_outputColorAttachment)
            {
                RenderPipeline* pipeline = previewOutputPipeline ? previewOutputPipeline : pass->GetRenderPipeline();
                if (pipeline)
                {
                    Pass* pipelinePass = pipeline->GetRootPass().get();
                    PassAttachmentBinding* binding = nullptr;

                    // Get either first output or first input/output
                    if (pipelinePass->GetOutputCount() > 0)
                    {
                        binding = &pipelinePass->GetOutputBinding(0);
                    }
                    else if (pipelinePass->GetInputOutputCount() > 0)
                    {
                        binding = &pipelinePass->GetInputOutputBinding(0);
                    }

                    if (binding)
                    {
                        SetOutputColorAttachment(binding->GetAttachment());
                    }

                    AZ_Warning("PassSystem", binding != nullptr, "ImageAttachmentPreviewPass couldn't find a color attachment on pipeline");
                }
            }
        }

        void ImageAttachmentPreviewPass::ClearPreviewAttachment()
        {
            ClearDrawData();
            // Allocate and release the copy scope only when there is an attachment to preview.
            // So we only need a weak ptr in the RenderPass and don't need to worry about releasing.
            m_attachmentCopy = nullptr;
            m_imageAttachmentId = AZ::RHI::AttachmentId{};
            m_updateDrawData = true;
            m_outputColorAttachment = nullptr;
        }
        
        void ImageAttachmentPreviewPass::SetPreviewLocation(AZ::Vector2 position, AZ::Vector2 size, bool keepAspectRatio)
        {
            m_position = position;
            m_size = size;
            m_keepAspectRatio = keepAspectRatio;
        }
        
        void ImageAttachmentPreviewPass::ClearDrawData()
        {
            if (m_needsShaderLoad)
            {
                return;
            }
            // update pass srg
            for (auto& previewInfo : m_imageTypePreviewInfo)
            {
                // unbind previous binded image views
                m_passSrg->SetImageView(previewInfo.m_imageInput, nullptr, 0);

                previewInfo.m_item.SetPipelineState(nullptr);
                previewInfo.m_imageCount = 0;
            }
            m_passSrgChanged = true;
        }

        void ImageAttachmentPreviewPass::SetOutputColorAttachment(AZ::RHI::Ptr<PassAttachment> outputImageAttachment)
        {
            m_outputColorAttachment = outputImageAttachment;
            m_updateDrawData = true;
        }

        void ImageAttachmentPreviewPass::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            AZ::Data::Asset<ShaderAsset> shaderAsset = AZ::Data::static_pointer_cast<ShaderAsset>(asset);
            if (shaderAsset)
            {
                m_needsShaderLoad = true;
                m_updateDrawData = true;
            }
        }

        void ImageAttachmentPreviewPass::LoadShader()
        {
            m_needsShaderLoad = false;

            // Load Shader
            constexpr const char* ShaderPath = "shaders/imagepreview.azshader";
            AZ::Data::Asset<ShaderAsset> shaderAsset = JE::Render::RPI::FindShaderAsset(ShaderPath);
            if (!shaderAsset.IsReady())
            {
                AZ_Error("PassSystem", false, "[ImageAttachmentsPreviewPass]: Failed to load shader '%s'!", GetPathName().GetCStr(), ShaderPath);
                return;
            }

            m_shader = Shader::FindOrCreate(shaderAsset);
            if (m_shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ImageAttachmentsPreviewPass]: Failed to create shader instance from asset '%s'!", ShaderPath);
                return;
            }

            // Load SRG
            const auto srgLayout = m_shader->FindShaderResourceGroupLayout(SrgBindingSlot::Pass);
            if (srgLayout)
            {
                m_passSrg = ShaderResourceGroup::Create(shaderAsset, m_shader->GetSupervariantIndex(), srgLayout->GetName());

                if (!m_passSrg)
                {
                    AZ_Error("PassSystem", false, "Failed to create SRG from shader asset '%s'", ShaderPath);
                    return;
                }
            }

            // Find srg input indexes
            m_imageTypePreviewInfo[static_cast<uint32_t>(ImageType::Image2d)].m_imageInput = m_passSrg->FindShaderInputImageIndex(AZ::Name("m_image"));
            m_imageTypePreviewInfo[static_cast<uint32_t>(ImageType::Image2dMs)].m_imageInput = m_passSrg->FindShaderInputImageIndex(AZ::Name("m_msImage"));
            m_colorRangeMinMaxInput = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_colorRangeMinMax"));
            
            // Setup initial data for pipeline state descriptors. The rest of the data will be set when the draw data is updated

            // option names from the azsl file
            const char* optionValues[] =
            {
                "ImageType::Image2d",
                "ImageType::Image2dMs"
            };
            const char* optionName = "o_imageType";

            ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();

            AZ::RHI::InputStreamLayout inputStreamLayout;
            inputStreamLayout.SetTopology(AZ::RHI::PrimitiveTopology::TriangleStrip);
            inputStreamLayout.Finalize();

            AZ::RHI::RenderAttachmentLayout attachmentsLayout;
            AZ::RHI::RenderAttachmentLayoutBuilder attachmentsLayoutBuilder;
            attachmentsLayoutBuilder.AddSubpass()
                ->RenderTargetAttachment(AZ::RHI::Format::R8G8B8A8_UNORM); // Set any format to avoid errors when building the layout.

            attachmentsLayoutBuilder.End(attachmentsLayout);

            for (uint32_t index = 0; index < static_cast<uint32_t>(ImageType::ImageTypeCount); index++)
            {
                ImageTypePreviewInfo& previewInfo = m_imageTypePreviewInfo[index];
                auto& pipelineDesc = previewInfo.m_pipelineStateDescriptor;

                shaderOption.SetValue(AZ::Name(optionName), AZ::Name(optionValues[index]));

                m_shader->GetVariant(shaderOption.GetShaderVariantId()).ConfigurePipelineState(pipelineDesc, shaderOption);
                pipelineDesc.m_renderAttachmentConfiguration.m_renderAttachmentLayout = attachmentsLayout;
                pipelineDesc.m_inputStreamLayout = inputStreamLayout;
                previewInfo.m_shaderVariantKeyFallback = shaderOption.GetShaderVariantKeyFallbackValue();
            }

            AZ::Data::AssetBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusConnect(shaderAsset.GetId());
        }

        void ImageAttachmentPreviewPass::BuildInternal()
        {
            m_updateDrawData = true;
        }

        void ImageAttachmentPreviewPass::FrameBeginInternal(FramePrepareParams params)
        {
            bool scopeImported = false;
            if (!m_imageAttachmentId.IsEmpty() && m_outputColorAttachment)
            {
                // Only import the scope if the attachment is valid
                auto attachmentDatabase = params.m_frameGraphBuilder->GetAttachmentDatabase();
                bool isAttachmentValid = attachmentDatabase.IsAttachmentValid(m_imageAttachmentId);
                if (!isAttachmentValid)
                {
                    // Import the cached copy dest image if it exists (copied)
                    // So the attachment can be still previewed when the pass is disabled.
                    if (m_attachmentCopy && m_attachmentCopy->m_destImage)
                    {
                        attachmentDatabase.ImportImage(m_attachmentCopy->m_destAttachmentId, m_attachmentCopy->m_destImage->GetRHIImage());
                        isAttachmentValid = true;
                    }
                }

                if (isAttachmentValid)
                {
                    if (m_needsShaderLoad)
                    {
                        LoadShader();
                    }

                    params.m_frameGraphBuilder->ImportScopeProducer(*this);
                    scopeImported = true;
                }
            }

            // If the scope is not imported, we need compile the updated pass srg here
            if (m_passSrgChanged && !scopeImported)
            {
                m_passSrg->Compile();
                m_passSrgChanged = false;
            }
        }

        bool ImageAttachmentPreviewPass::ReadbackOutput(AZStd::shared_ptr<AttachmentReadback> readback)
        {
            if (m_outputColorAttachment)
            {
                m_readbackOption = PassAttachmentReadbackOption::Output;
                m_attachmentReadback = readback;
                AZStd::string readbackName = AZStd::string::format("%s_%s", m_outputColorAttachment->GetAttachmentId().GetCStr(), GetName().GetCStr());
                return m_attachmentReadback->ReadPassAttachment(m_outputColorAttachment.get(), AZ::Name(readbackName));
            }
            return false;
        }
        
        void ImageAttachmentPreviewPass::SetColorTransformRange(float colorTransformRange[2])
        {
            m_attachmentColorTranformRange[0] = AZ::GetMin(colorTransformRange[0], colorTransformRange[1]);
            m_attachmentColorTranformRange[1] = AZ::GetMax(colorTransformRange[0], colorTransformRange[1]);
            m_updateDrawData = true;
        }

        void ImageAttachmentPreviewPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            // add attachments to the scope
            // input attachment
            AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase = frameGraph.GetAttachmentDatabase();
            AZ::RHI::ImageDescriptor imageDesc = attachmentDatabase.GetImageDescriptor(m_imageAttachmentId);
            // only preview mip 0 and array 0
            AZ::RHI::ImageViewDescriptor imageViewDesc = AZ::RHI::ImageViewDescriptor::Create(
                AZ::RHI::Format::Unknown, // no overwrite
                0,  //mipSliceMin
                0,  //mipSliceMax
                0,  //arraySliceMin
                0   //arraySliceMax
            );

            // If the format contains depth, set m_aspectFlags to depth, otherwise set it to color
            imageViewDesc.m_aspectFlags = AZ::RHI::CheckBitsAny(AZ::RHI::GetImageAspectFlags(imageDesc.m_format), AZ::RHI::ImageAspectFlags::Depth) ? 
                AZ::RHI::ImageAspectFlags::Depth : AZ::RHI::ImageAspectFlags::Color;

            auto scopeAttachmentDesc = AZ::RHI::ImageScopeAttachmentDescriptor{ m_imageAttachmentId, imageViewDesc};
            frameGraph.UseAttachment(scopeAttachmentDesc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentUsage::Shader, AZ::RHI::ScopeAttachmentStage::FragmentShader);

            // output attachment
            frameGraph.UseColorAttachment(AZ::RHI::ImageScopeAttachmentDescriptor{ m_outputColorAttachment->GetAttachmentId() });
            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(m_imageTypePreviewInfo.size()));
        }

        void ImageAttachmentPreviewPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            // setup srg data and draw item
            if (m_updateDrawData)
            {
                m_updateDrawData = false;

                // clear some old data
                ClearDrawData();
                
                ImageType imageType = ImageType::Unsupported;
                float aspectRatio = 1;

                // Find image type
                const AZ::RHI::ImageView* inputImageView = context.GetImageView(m_imageAttachmentId);
                if (!inputImageView)
                {
                    AZ_Warning("RPI", false, "Image attachment [%s] doesn't have image view in current context", m_imageAttachmentId.GetCStr());
                }
                else
                {
                    const AZ::RHI::ImageDescriptor& desc = inputImageView->GetImage()->GetDescriptor();
                    aspectRatio = desc.m_size.m_width / static_cast<float>(desc.m_size.m_height);

                    if (desc.m_dimension == AZ::RHI::ImageDimension::Image2D)
                    {
                        if (desc.m_multisampleState.m_samples == 1)
                        {
                            imageType = ImageType::Image2d;
                        }
                        else if (desc.m_multisampleState.m_samples > 1)
                        {
                            imageType = ImageType::Image2dMs;
                        }
                    }

                    if (imageType != ImageType::Unsupported)
                    {
                        const uint32_t typeIndex = static_cast<uint32_t>(imageType);
                        auto& previewInfo = m_imageTypePreviewInfo[typeIndex];
                        m_passSrg->SetShaderVariantKeyFallbackValue(previewInfo.m_shaderVariantKeyFallback);
                        m_passSrg->SetImageView(previewInfo.m_imageInput, inputImageView, 0);
                        m_passSrg->SetConstant(m_colorRangeMinMaxInput, m_attachmentColorTranformRange);
                        m_passSrgChanged = true;
                        previewInfo.m_imageCount = 1;
                    }
                    else
                    {
                        AZ_Warning("RPI", false, "Image attachment [%s] with format %d is not supported", m_imageAttachmentId.GetCStr(),
                            static_cast<uint32_t>(desc.m_format));
                    }
                }
                
                // config pipeline states related to output attachment
                const AZ::RHI::ImageView* outputImageView = context.GetImageView(m_outputColorAttachment->GetAttachmentId());
                AZ::RHI::Format outputFormat = outputImageView->GetDescriptor().m_overrideFormat;
                if (outputFormat == AZ::RHI::Format::Unknown)
                {
                    outputFormat = outputImageView->GetImage()->GetDescriptor().m_format;
                }
                
                // Base viewport and scissor off of output attachment
                AZ::RHI::Size targetImageSize = outputImageView->GetImage()->GetDescriptor().m_size;

                float width = m_size.GetX() * targetImageSize.m_width;
                float height = m_size.GetY() * targetImageSize.m_height;

                if (m_keepAspectRatio)
                {
                    if (width/height > aspectRatio)
                    {
                        width = height * aspectRatio;
                    }
                    else
                    {
                        height = width / aspectRatio;
                    }
                }

                float xMin = m_position.GetX() * targetImageSize.m_width;
                float yMin = m_position.GetY() * targetImageSize.m_height;

                m_viewport = AZ::RHI::Viewport(xMin, xMin + width, yMin, yMin + height);
                m_scissor = AZ::RHI::Scissor(0, 0, targetImageSize.m_width, targetImageSize.m_height);
                
                // compile
                if (m_passSrgChanged)
                {
                    m_passSrg->Compile();
                    m_passSrgChanged = false;
                }

                // rebuild draw item
                for (auto& previewInfo : m_imageTypePreviewInfo)
                {
                    if (previewInfo.m_imageCount == 0)
                    {
                        continue;
                    }
                    previewInfo.m_pipelineStateDescriptor.m_renderAttachmentConfiguration.m_renderAttachmentLayout.m_attachmentFormats[0] = outputFormat;
                    previewInfo.m_pipelineStateDescriptor.m_renderStates.m_multisampleState = outputImageView->GetImage()->GetDescriptor().m_multisampleState;

                    // draw each image by using instancing
                    AZ::RHI::DrawInstanceArguments drawInstanceArgs(previewInfo.m_imageCount, 0);
                    AZ::RHI::DrawLinear drawLinear(4, 0);

                    previewInfo.m_geometryView.SetDrawArguments(drawLinear);
                    previewInfo.m_item.SetDrawInstanceArgs(drawInstanceArgs);
                    previewInfo.m_item.SetGeometryView(&previewInfo.m_geometryView);
                    previewInfo.m_item.SetPipelineState(m_shader->AcquirePipelineState(previewInfo.m_pipelineStateDescriptor));
                }
            }
        }

        void ImageAttachmentPreviewPass::BuildCommandList(const AZ::RHI::FrameGraphExecuteContext& context)
        {
            AZ::RHI::CommandList* commandList = context.GetCommandList();

            commandList->SetViewport(m_viewport);
            commandList->SetScissor(m_scissor);

            // submit srg
            commandList->SetShaderResourceGroupForDraw(*m_passSrg->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));

            // submit draw call
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                const ImageTypePreviewInfo& previewInfo = m_imageTypePreviewInfo[index];
                if (previewInfo.m_imageCount > 0)
                {
                    commandList->Submit(previewInfo.m_item.GetDeviceDrawItem(context.GetDeviceIndex()), index);
                }
            }
        }
    }
}
