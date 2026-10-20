/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDispatchRaysItem.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/View.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <Render/DiffuseProbeGridVisualizationRayTracingPass.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<DiffuseProbeGridVisualizationRayTracingPass> DiffuseProbeGridVisualizationRayTracingPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DiffuseProbeGridVisualizationRayTracingPass> pass = aznew DiffuseProbeGridVisualizationRayTracingPass(descriptor);
            return pass;
        }

        DiffuseProbeGridVisualizationRayTracingPass::DiffuseProbeGridVisualizationRayTracingPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::RenderPass(descriptor)
        {
            if (AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() == AZ::RHI::MultiDevice::NoDevices || !JE_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // raytracing or GI is not supported on this platform
                SetEnabled(false);
            }
        }

        void DiffuseProbeGridVisualizationRayTracingPass::CreateRayTracingPipelineState()
        {
            // load the ray tracing shader
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridVisualizationRayTracing.azshader";
            m_rayTracingShader = JE::Render::RPI::LoadCriticalShader(shaderFilePath);
            if (m_rayTracingShader == nullptr)
            {
                return;
            }

            auto shaderVariant = m_rayTracingShader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            AZ::RHI::PipelineStateDescriptorForRayTracing rayGenerationShaderDescriptor;
            shaderVariant.ConfigurePipelineState(rayGenerationShaderDescriptor);

            // closest hit shader
            AZStd::string closestHitShaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridVisualizationRayTracingClosestHit.azshader";
            m_closestHitShader = JE::Render::RPI::LoadCriticalShader(closestHitShaderFilePath);

            auto closestHitShaderVariant = m_closestHitShader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            AZ::RHI::PipelineStateDescriptorForRayTracing closestHitShaderDescriptor;
            closestHitShaderVariant.ConfigurePipelineState(closestHitShaderDescriptor);

            // miss shader
            AZStd::string missShaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridVisualizationRayTracingMiss.azshader";
            m_missShader = JE::Render::RPI::LoadCriticalShader(missShaderFilePath);

            auto missShaderVariant = m_missShader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            AZ::RHI::PipelineStateDescriptorForRayTracing missShaderDescriptor;
            missShaderVariant.ConfigurePipelineState(missShaderDescriptor);

            // global pipeline state and Srg
            m_globalPipelineState = m_rayTracingShader->AcquirePipelineState(rayGenerationShaderDescriptor);
            AZ_Assert(m_globalPipelineState, "Failed to acquire ray tracing global pipeline state");

            m_globalSrgLayout = m_rayTracingShader->FindShaderResourceGroupLayout(AZ::Name{ "RayTracingGlobalSrg" });
            AZ_Assert(m_globalSrgLayout != nullptr, "Failed to find RayTracingGlobalSrg layout for shader [%s]", shaderFilePath.c_str());

            // build the ray tracing pipeline state descriptor
            AZ::RHI::RayTracingPipelineStateDescriptor descriptor;
            descriptor.m_pipelineState = m_globalPipelineState.get();
            descriptor.m_configuration.m_maxPayloadSize = 64;
            descriptor.m_configuration.m_maxAttributeSize = 32;
            descriptor.m_configuration.m_maxRecursionDepth = 2;

            descriptor.AddRayGenerationShaderLibrary(rayGenerationShaderDescriptor, AZ::Name("RayGen"));
            descriptor.AddMissShaderLibrary(missShaderDescriptor, AZ::Name("Miss"));
            descriptor.AddClosestHitShaderLibrary(closestHitShaderDescriptor, AZ::Name("ClosestHit"));

            descriptor.AddHitGroup(AZ::Name("HitGroup"), AZ::Name("ClosestHit"));

            // create the ray tracing pipeline state object
            m_rayTracingPipelineState = aznew AZ::RHI::RayTracingPipelineState;
            m_rayTracingPipelineState->Init(AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport(), descriptor);
        }

        bool DiffuseProbeGridVisualizationRayTracingPass::IsEnabled() const
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

        void DiffuseProbeGridVisualizationRayTracingPass::FrameBeginInternal(FramePrepareParams params)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            if (!m_initialized)
            {
                CreateRayTracingPipelineState();
                m_initialized = true;
            }

            if (!m_rayTracingShaderTable)
            {
                AZ::RHI::RayTracingBufferPools& rayTracingBufferPools = diffuseProbeGridFeatureProcessor->GetVisualizationBufferPools();

                m_rayTracingShaderTable = aznew AZ::RHI::RayTracingShaderTable;
                m_rayTracingShaderTable->Init(AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport(), rayTracingBufferPools);

                AZStd::shared_ptr<AZ::RHI::RayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<AZ::RHI::RayTracingShaderTableDescriptor>();

                // build the ray tracing shader table descriptor
                descriptor->m_name = AZ::Name("RayTracingShaderTable");
                descriptor->m_rayTracingPipelineState = m_rayTracingPipelineState;
                descriptor->m_rayGenerationRecord.emplace_back(AZ::Name("RayGen"));
                descriptor->m_missRecords.emplace_back(AZ::Name("Miss"));
                descriptor->m_hitGroupRecords.emplace_back(AZ::Name("HitGroup"));

                m_rayTracingShaderTable->Build(descriptor);
            }

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridVisualizationRayTracingPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids().size()));

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                if (!diffuseProbeGrid->GetVisualizationEnabled())
                {
                    continue;
                }

                // TLAS
                {
                    AZ::RHI::AttachmentId tlasAttachmentId = diffuseProbeGrid->GetProbeVisualizationTlasAttachmentId();
                    const AZ::RHI::Ptr<AZ::RHI::Buffer>& visualizationTlasBuffer = diffuseProbeGrid->GetVisualizationTlas()->GetTlasBuffer();
                    if (visualizationTlasBuffer)
                    {
                        if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(tlasAttachmentId))
                        {
                            [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(tlasAttachmentId, visualizationTlasBuffer);
                            AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import ray tracing TLAS buffer with error %d", result);
                        }

                        uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(visualizationTlasBuffer->GetDescriptor().m_byteCount);
                        AZ::RHI::BufferViewDescriptor tlasBufferViewDescriptor = AZ::RHI::BufferViewDescriptor::CreateRaw(0, tlasBufferByteCount);

                        AZ::RHI::BufferScopeAttachmentDescriptor desc;
                        desc.m_attachmentId = tlasAttachmentId;
                        desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                        frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                    }
                }

                // grid data
                {
                    AZ::RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                    desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }

                // probe irradiance
                {
                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;
                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }

                // probe distance
                {
                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }

                // probe data
                {
                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetProbeDataImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDataImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }
            }

            // retrieve the visualization image size, this will determine the number of rays to cast
            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> visualizationImageAttachment = m_ownedAttachments[0];
            AZ_Assert(visualizationImageAttachment.get(), "Invalid DiffuseProbeGrid Visualization image");

            m_outputAttachmentSize = visualizationImageAttachment->GetTransientImageDescriptor().m_imageDescriptor.m_size;
        }

        void DiffuseProbeGridVisualizationRayTracingPass::CompileResources([[maybe_unused]] const AZ::RHI::FrameGraphCompileContext& context)
        {
            const AZ::RHI::ImageView* outputImageView = context.GetImageView(GetOutputBinding(0).GetAttachment()->GetAttachmentId());
            AZ_Assert(outputImageView, "Failed to retrieve output ImageView");

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                if (!diffuseProbeGrid->GetVisualizationEnabled())
                {
                    continue;
                }

                // the DiffuseProbeGridVisualization Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader
                // inputs (see line ValidateSetImageView() in ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateVisualizationRayTraceSrg(m_rayTracingShader, m_globalSrgLayout, outputImageView);
                if (!diffuseProbeGrid->GetVisualizationRayTraceSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetVisualizationRayTraceSrg()->Compile();
                }
            }

            if (auto viewSRG = diffuseProbeGridFeatureProcessor->GetViewSrg(m_pipeline, GetPipelineViewTag()))
            {
                BindSrg(viewSRG->GetRHIShaderResourceGroup());
            }
        }

        void DiffuseProbeGridVisualizationRayTracingPass::BuildCommandListInternal([[maybe_unused]] const AZ::RHI::FrameGraphExecuteContext& context)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<JE::Render::Features::RayTracingFeatureProcessorInterface>();
            AZ_Assert(rayTracingFeatureProcessor, "DiffuseProbeGridVisualizationRayTracingPass requires the RayTracingFeatureProcessor");

            const AZStd::vector<JE::Render::RPI::ViewPtr>& views = m_pipeline->GetViews(JE::Render::RPI::PipelineViewTag{ "MainCamera" });
            if (views.empty())
            {
                return;
            }

            const AZ::RHI::ShaderResourceGroup* sceneSrg = m_pipeline->GetScene()->GetRHIShaderResourceGroup();

            // submit the DispatchRaysItems for each DiffuseProbeGrid in this range
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids()[index];

                if (!diffuseProbeGrid->GetVisualizationEnabled())
                {
                    continue;
                }

                const AZ::RHI::DeviceShaderResourceGroup* shaderResourceGroups[] = {
                    diffuseProbeGrid->GetVisualizationRayTraceSrg()->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get(),
                    rayTracingFeatureProcessor->GetRayTracingSceneSrg()->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get(),
                    views[0]->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get(),
                    sceneSrg->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get()
                };

                AZ::RHI::DeviceDispatchRaysItem dispatchRaysItem;
                dispatchRaysItem.m_arguments.m_direct.m_width = m_outputAttachmentSize.m_width;
                dispatchRaysItem.m_arguments.m_direct.m_height = m_outputAttachmentSize.m_height;
                dispatchRaysItem.m_arguments.m_direct.m_depth = 1;
                dispatchRaysItem.m_rayTracingPipelineState =
                    m_rayTracingPipelineState->GetDeviceRayTracingPipelineState(context.GetDeviceIndex()).get();
                dispatchRaysItem.m_rayTracingShaderTable = m_rayTracingShaderTable->GetDeviceRayTracingShaderTable(context.GetDeviceIndex()).get();
                dispatchRaysItem.m_shaderResourceGroupCount = AZ::RHI::ArraySize(shaderResourceGroups);
                dispatchRaysItem.m_shaderResourceGroups = shaderResourceGroups;
                dispatchRaysItem.m_globalPipelineState = m_globalPipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();

                // submit the DispatchRays item
                context.GetCommandList()->Submit(dispatchRaysItem, index);
            }
        }
    }   // namespace RPI
}   // namespace JE::Render
