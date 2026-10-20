/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <JE/Render/RPI/Public/Image/AttachmentImagePool.h>
#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridRenderPass.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<JE::Render::Features::DiffuseProbeGridRenderPass> DiffuseProbeGridRenderPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            return aznew DiffuseProbeGridRenderPass(descriptor);
        }

        DiffuseProbeGridRenderPass::DiffuseProbeGridRenderPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            if (!JE_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                SetEnabled(false);
                return;
            }

            // create the shader resource group
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRender.azshader";
            m_shader = JE::Render::RPI::LoadCriticalShader(shaderFilePath);
            if (m_shader == nullptr)
            {
                return;
            }

            m_srgLayout = m_shader->FindShaderResourceGroupLayout(JE::Render::RPI::SrgBindingSlot::Pass);
            AZ_Assert(m_srgLayout != nullptr, "[DiffuseProbeGridRenderPass '%s']: Failed to find SRG layout", GetPathName().GetCStr());

            m_shaderResourceGroup = JE::Render::RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), m_srgLayout->GetName());
            AZ_Assert(m_shaderResourceGroup, "[DiffuseProbeGridRenderPass '%s']: Failed to create SRG", GetPathName().GetCStr());
        }

        bool DiffuseProbeGridRenderPass::IsEnabled() const
        {
            if (!RenderPass::IsEnabled())
            {
                return false;
            }

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            if (!scene)
            {
                return false;
            }

            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetProbeGrids().empty())
            {
                // no diffuse probe grids
                return false;
            }

            return true;
        }

        void DiffuseProbeGridRenderPass::FrameBeginInternal(FramePrepareParams params)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // get output attachment size
            AZ_Assert(GetInputOutputCount() > 0, "DiffuseProbeGridRenderPass: Could not find output bindings");
            JE::Render::RPI::PassAttachment* m_outputAttachment = GetInputOutputBinding(0).GetAttachment().get();
            AZ_Assert(m_outputAttachment, "DiffuseProbeGridRenderPass: Output binding has no attachment!");
            
            AZ::RHI::Size size = m_outputAttachment->m_descriptor.m_image.m_size;
            AZ::RHI::Viewport viewport(0.f, aznumeric_cast<float>(size.m_width), 0.f, aznumeric_cast<float>(size.m_height));
            AZ::RHI::Scissor scissor(0, 0, size.m_width, size.m_height);
            
            params.m_viewportState = viewport;
            params.m_scissorState = scissor;

            Base::FrameBeginInternal(params);

            // process attachment readback for RealTime grids, if raytracing is supported on this device
            if (AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() != AZ::RHI::MultiDevice::NoDevices)
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetRealTimeProbeGrids())
                {
                    diffuseProbeGrid->GetTextureReadback().FrameBegin(params);
                }
            }
        }

        void DiffuseProbeGridRenderPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                if (!ShouldRender(diffuseProbeGrid))
                {
                    continue;
                }

                // grid data
                {
                    AZ::RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                    desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::FragmentShader);
                }

                // probe irradiance image
                {
                    AZ::RHI::AttachmentId attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked && !frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                    {
                        // import the irradiance image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, diffuseProbeGrid->GetIrradianceImage());
                        AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import probeIrradianceImage");
                    }

                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = attachmentId;
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::FragmentShader);
                }

                // probe distance image
                {
                    AZ::RHI::AttachmentId attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked && !frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                    {
                        // import the distance image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, diffuseProbeGrid->GetDistanceImage());
                        AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import probeDistanceImage");
                    }

                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = attachmentId;
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::FragmentShader);
                }

                // probe data image
                {
                    AZ::RHI::AttachmentId attachmentId = diffuseProbeGrid->GetProbeDataImageAttachmentId();
                    if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked && !frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                    {
                        // import the probe data image now, since it is baked and therefore was not imported during the raytracing pass
                        [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(attachmentId, diffuseProbeGrid->GetProbeDataImage());
                        AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import ProbeDataImage");
                    }

                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = attachmentId;
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDataImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;
                
                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::FragmentShader);
                }

                diffuseProbeGrid->GetTextureReadback().Update(GetName());
            }

            Base::SetupFrameGraphDependencies(frameGraph);
        }

        void DiffuseProbeGridRenderPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                if (!ShouldRender(diffuseProbeGrid))
                {
                    continue;
                }

                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() of ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateRenderObjectSrg();

                if (!diffuseProbeGrid->GetRenderObjectSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetRenderObjectSrg()->Compile();
                }
            }

            if (auto viewSRG = diffuseProbeGridFeatureProcessor->GetViewSrg(m_pipeline, GetPipelineViewTag()))
            {
                BindSrg(viewSRG->GetRHIShaderResourceGroup());
            }

            Base::CompileResources(context);
        }

        bool DiffuseProbeGridRenderPass::ShouldRender(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid)
        {
            // check for baked mode with no valid textures
            if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::Baked &&
                !diffuseProbeGrid->HasValidBakedTextures())
            {
                return false;
            }

            // check for RealTime mode without ray tracing
            if (diffuseProbeGrid->GetMode() == DiffuseProbeGridMode::RealTime &&
                (AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() == AZ::RHI::MultiDevice::NoDevices))
            {
                return false;
            }

            // check if culled out
            if (!diffuseProbeGrid->GetIsVisible())
            {
                return false;
            }

            // DiffuseProbeGrid should be rendered
            return true;
        }
    } // namespace Features
} // namespace JE::Render
