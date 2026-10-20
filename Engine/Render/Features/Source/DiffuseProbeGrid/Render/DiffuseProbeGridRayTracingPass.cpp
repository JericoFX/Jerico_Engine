/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDispatchRaysItem.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <JE/Render/RPI/Public/Buffer/BufferSystemInterface.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/View.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <JE/Render/Features/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <JE/Render/Features/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <Render/DiffuseProbeGridRayTracingPass.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<DiffuseProbeGridRayTracingPass> DiffuseProbeGridRayTracingPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DiffuseProbeGridRayTracingPass> pass = aznew DiffuseProbeGridRayTracingPass(descriptor);
            return pass;
        }

        DiffuseProbeGridRayTracingPass::DiffuseProbeGridRayTracingPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::RenderPass(descriptor)
        {
            if (AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() == AZ::RHI::MultiDevice::NoDevices || !JE_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // raytracing or GI is not supported on this platform
                SetEnabled(false);
            }
        }

        void DiffuseProbeGridRayTracingPass::CreateRayTracingPipelineState()
        {
            // load the ray tracing shader
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRayTracing.azshader";
            m_rayTracingShader = JE::Render::RPI::LoadCriticalShader(shaderFilePath);
            if (m_rayTracingShader == nullptr)
            {
                return;
            }

            auto shaderVariant = m_rayTracingShader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            AZ::RHI::PipelineStateDescriptorForRayTracing rayGenerationShaderDescriptor;
            shaderVariant.ConfigurePipelineState(rayGenerationShaderDescriptor, m_rayTracingShader->GetDefaultShaderOptions());

            // closest hit shader
            AZStd::string closestHitShaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRayTracingClosestHit.azshader";
            m_closestHitShader = JE::Render::RPI::LoadCriticalShader(closestHitShaderFilePath);

            auto closestHitShaderVariant = m_closestHitShader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            AZ::RHI::PipelineStateDescriptorForRayTracing closestHitShaderDescriptor;
            closestHitShaderVariant.ConfigurePipelineState(closestHitShaderDescriptor, m_closestHitShader->GetDefaultShaderOptions());

            // miss shader
            AZStd::string missShaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRayTracingMiss.azshader";
            m_missShader = JE::Render::RPI::LoadCriticalShader(missShaderFilePath);

            auto missShaderVariant = m_missShader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            AZ::RHI::PipelineStateDescriptorForRayTracing missShaderDescriptor;
            missShaderVariant.ConfigurePipelineState(missShaderDescriptor, m_missShader->GetDefaultShaderOptions());

            // global pipeline state and Srg
            m_globalPipelineState = m_rayTracingShader->AcquirePipelineState(rayGenerationShaderDescriptor);
            AZ_Assert(m_globalPipelineState, "Failed to acquire ray tracing global pipeline state");

            m_globalSrgLayout = m_rayTracingShader->FindShaderResourceGroupLayout(AZ::Name{ "RayTracingGlobalSrg" });
            AZ_Error( "DiffuseProbeGridRayTracingPass", m_globalSrgLayout != nullptr, "Failed to find RayTracingGlobalSrg layout for shader [%s]", shaderFilePath.c_str());

            // build the ray tracing pipeline state descriptor
            AZ::RHI::RayTracingPipelineStateDescriptor descriptor;
            descriptor.m_pipelineState = m_globalPipelineState.get();
            descriptor.m_configuration.m_maxPayloadSize = 96;
            descriptor.m_configuration.m_maxAttributeSize = 32;
            descriptor.m_configuration.m_maxRecursionDepth = MaxRecursionDepth;

            descriptor.AddRayGenerationShaderLibrary(rayGenerationShaderDescriptor, AZ::Name("RayGen"));
            descriptor.AddMissShaderLibrary(missShaderDescriptor, AZ::Name("Miss"));
            descriptor.AddClosestHitShaderLibrary(closestHitShaderDescriptor, AZ::Name("ClosestHit"));

            descriptor.AddHitGroup(AZ::Name("HitGroup"), AZ::Name("ClosestHit"));

            // create the ray tracing pipeline state object
            m_rayTracingPipelineState = aznew AZ::RHI::RayTracingPipelineState;
            m_rayTracingPipelineState->Init(AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport(), descriptor);

            // Since the ray tracing pipeline state changed, we need to rebuilt the shader table
            m_rayTracingRevision = 0;
        }

        bool DiffuseProbeGridRayTracingPass::IsEnabled() const
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

            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<JE::Render::Features::RayTracingFeatureProcessorInterface>();
            if (!rayTracingFeatureProcessor)
            {
                return false;
            }

            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().empty())
            {
                // no diffuse probe grids
                return false;
            }

            return true;
        }

        void DiffuseProbeGridRayTracingPass::BuildInternal()
        {
            if (AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() != AZ::RHI::MultiDevice::NoDevices)
            {
                CreateRayTracingPipelineState();
            }
        }

        void DiffuseProbeGridRayTracingPass::FrameBeginInternal(FramePrepareParams params)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<JE::Render::Features::RayTracingFeatureProcessorInterface>();

            if (!m_rayTracingShaderTable)
            {
                AZ::RHI::RayTracingBufferPools& rayTracingBufferPools = rayTracingFeatureProcessor->GetBufferPools();

                m_rayTracingShaderTable = aznew AZ::RHI::RayTracingShaderTable;
                m_rayTracingShaderTable->Init(AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport(), rayTracingBufferPools);
            }

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridRayTracingPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<JE::Render::Features::RayTracingFeatureProcessorInterface>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().size()));

            // TLAS
            if (!diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().empty())
            {
                AZ::RHI::AttachmentId tlasAttachmentId = rayTracingFeatureProcessor->GetTlasAttachmentId();
                const AZ::RHI::Ptr<AZ::RHI::Buffer>& rayTracingTlasBuffer = rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer();
                if (rayTracingTlasBuffer)
                {
                    [[maybe_unused]] AZ::RHI::ResultCode result =
                        frameGraph.GetAttachmentDatabase().ImportBuffer(tlasAttachmentId, rayTracingTlasBuffer);
                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import ray tracing TLAS buffer with error %d", result);

                    uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(rayTracingTlasBuffer->GetDescriptor().m_byteCount);
                    AZ::RHI::BufferViewDescriptor tlasBufferViewDescriptor = AZ::RHI::BufferViewDescriptor::CreateRaw(0, tlasBufferByteCount);

                    AZ::RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = tlasAttachmentId;
                    desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(
                        desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }
            }

            for (const auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                // grid data
                {
                    AZ::RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                    desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }

                // probe raytrace
                {
                    [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetRayTraceImageAttachmentId(), diffuseProbeGrid->GetRayTraceImage());
                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import probeRayTraceImage");

                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRayTraceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRayTraceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(
                        desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }

                // probe irradiance
                {
                    [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetIrradianceImageAttachmentId(), diffuseProbeGrid->GetIrradianceImage());
                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import probeIrradianceImage");

                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(
                        desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }

                // probe distance
                {
                    [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetDistanceImageAttachmentId(), diffuseProbeGrid->GetDistanceImage());
                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import probeDistanceImage");

                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(
                        desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }

                // probe data
                {
                    [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(diffuseProbeGrid->GetProbeDataImageAttachmentId(), diffuseProbeGrid->GetProbeDataImage());
                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import ProbeDataImage");

                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetProbeDataImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDataImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(
                        desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::RayTracingShader);
                }
            }
        }

        void DiffuseProbeGridRayTracingPass::CompileResources([[maybe_unused]] const AZ::RHI::FrameGraphCompileContext& context)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<JE::Render::Features::RayTracingFeatureProcessorInterface>();

            if (rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() && rayTracingFeatureProcessor->GetSubMeshCount())
            {
                for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
                {
                    // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader
                    // inputs (see line ValidateSetImageView() in ShaderResourceGroupData.cpp)
                    diffuseProbeGrid->UpdateRayTraceSrg(m_rayTracingShader, m_globalSrgLayout);

                    diffuseProbeGrid->GetRayTraceSrg()->SetConstant(m_maxRecursionDepthNameIndex, MaxRecursionDepth);
                    if (!diffuseProbeGrid->GetRayTraceSrg()->IsQueuedForCompile())
                    {
                        diffuseProbeGrid->GetRayTraceSrg()->Compile();
                    }
                }
            }

            uint32_t rayTracingRevision = rayTracingFeatureProcessor->GetRevision();
            if (m_rayTracingRevision != rayTracingRevision)
            {
                // scene changed, need to rebuild the shader table
                m_rayTracingRevision = rayTracingRevision;

                AZStd::shared_ptr<AZ::RHI::RayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<AZ::RHI::RayTracingShaderTableDescriptor>();

                if (rayTracingFeatureProcessor->GetSubMeshCount())
                {
                    // build the ray tracing shader table descriptor
                    descriptor->m_name = AZ::Name("RayTracingShaderTable");
                    descriptor->m_rayTracingPipelineState = m_rayTracingPipelineState;
                    descriptor->m_rayGenerationRecord.emplace_back(AZ::Name("RayGen"));
                    descriptor->m_missRecords.emplace_back(AZ::Name("Miss"));
                    descriptor->m_hitGroupRecords.emplace_back(AZ::Name("HitGroup"));
                }

                m_rayTracingShaderTable->Build(descriptor);
            }
        }

        void DiffuseProbeGridRayTracingPass::BuildCommandListInternal([[maybe_unused]] const AZ::RHI::FrameGraphExecuteContext& context)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<JE::Render::Features::RayTracingFeatureProcessorInterface>();
            AZ_Assert(rayTracingFeatureProcessor, "DiffuseProbeGridRayTracingPass requires the RayTracingFeatureProcessor");

            if (rayTracingFeatureProcessor &&
                rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer() &&
                rayTracingFeatureProcessor->GetSubMeshCount() &&
                m_rayTracingShaderTable)
            {
                const AZ::RHI::ShaderResourceGroup* sceneSrg = m_pipeline->GetScene()->GetRHIShaderResourceGroup();
                // submit the DispatchRaysItems for each DiffuseProbeGrid in this range
                for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
                {
                    AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids()[index];

                    const AZ::RHI::DeviceShaderResourceGroup* shaderResourceGroups[] = {
                        diffuseProbeGrid->GetRayTraceSrg()->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get(),
                        rayTracingFeatureProcessor->GetRayTracingSceneSrg()->GetRHIShaderResourceGroup()->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get(),
                        sceneSrg->GetDeviceShaderResourceGroup(context.GetDeviceIndex()).get()
                    };

                    AZ::RHI::DeviceDispatchRaysItem dispatchRaysItem;
                    dispatchRaysItem.m_arguments.m_direct.m_width = diffuseProbeGrid->GetNumRaysPerProbe().m_rayCount;
                    dispatchRaysItem.m_arguments.m_direct.m_height = AZ::DivideAndRoundUp(diffuseProbeGrid->GetTotalProbeCount(), diffuseProbeGrid->GetFrameUpdateCount());
                    dispatchRaysItem.m_arguments.m_direct.m_depth = 1;
                    dispatchRaysItem.m_rayTracingPipelineState = m_rayTracingPipelineState->GetDeviceRayTracingPipelineState(context.GetDeviceIndex()).get();
                    dispatchRaysItem.m_rayTracingShaderTable = m_rayTracingShaderTable->GetDeviceRayTracingShaderTable(context.GetDeviceIndex()).get();
                    dispatchRaysItem.m_shaderResourceGroupCount = AZ::RHI::ArraySize(shaderResourceGroups);
                    dispatchRaysItem.m_shaderResourceGroups = shaderResourceGroups;
                    dispatchRaysItem.m_globalPipelineState = m_globalPipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();

                    // submit the DispatchRays item
                    context.GetCommandList()->Submit(dispatchRaysItem, index);
                }
            }
        }
    }   // namespace RPI
}   // namespace JE::Render
