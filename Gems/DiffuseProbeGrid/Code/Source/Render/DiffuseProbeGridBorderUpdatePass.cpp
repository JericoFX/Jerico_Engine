/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/Device.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>
#include <Render/DiffuseProbeGridBorderUpdatePass.h>

namespace AZ
{
    namespace Render
    {
        JE::Render::RPI::Ptr<DiffuseProbeGridBorderUpdatePass> DiffuseProbeGridBorderUpdatePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DiffuseProbeGridBorderUpdatePass> pass = aznew DiffuseProbeGridBorderUpdatePass(descriptor);
            return pass;
        }

        DiffuseProbeGridBorderUpdatePass::DiffuseProbeGridBorderUpdatePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::RenderPass(descriptor)
        {
            if (!AZ_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                SetEnabled(false);
            }
            else
            {
                LoadShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridBorderUpdateRow.azshader",
                           m_rowShader,
                           m_rowPipelineState,
                           m_rowSrgLayout,
                           m_rowDispatchArgs);

                LoadShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridBorderUpdateColumn.azshader",
                           m_columnShader,
                           m_columnPipelineState,
                           m_columnSrgLayout,
                           m_columnDispatchArgs);
            }
        }

        void DiffuseProbeGridBorderUpdatePass::LoadShader(AZStd::string shaderFilePath,
                                                          Data::Instance<JE::Render::RPI::Shader>& shader,
                                                          const RHI::PipelineState*& pipelineState,
                                                          RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout,
                                                          RHI::DispatchDirect& dispatchArgs)
        {
            // load shader
            // Note: the shader may not be available on all platforms
            shader = JE::Render::RPI::LoadCriticalShader(shaderFilePath);
            if (shader == nullptr)
            {
                return;
            }

            // load pipeline state
            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = shader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);

            // load Pass Srg asset
            srgLayout = shader->FindShaderResourceGroupLayout(JE::Render::RPI::SrgBindingSlot::Pass);

            // retrieve the number of threads per thread group from the shader
            const auto outcome = JE::Render::RPI::GetComputeShaderNumThreads(shader->GetAsset(), dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false, "[DiffuseProbeGridBorderUpdatePass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath.c_str(), outcome.GetError().c_str());
            }
        }

        bool DiffuseProbeGridBorderUpdatePass::IsEnabled() const
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

            auto* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessorInterface>();
            if (!rayTracingFeatureProcessor || !rayTracingFeatureProcessor->GetSubMeshCount())
            {
                // empty scene
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

        void DiffuseProbeGridBorderUpdatePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // there are 4 submits per grid
            uint32_t totalSubmits = aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids().size() * 4);
            frameGraph.SetEstimatedItemCount(totalSubmits);
            m_submitItems.reserve(totalSubmits);

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                // probe irradiance image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe distance image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
                }
            }
        }

        void DiffuseProbeGridBorderUpdatePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleRealTimeProbeGrids())
            {
                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see line ValidateSetImageView() in ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateBorderUpdateSrgs(m_rowShader, m_rowSrgLayout, m_columnShader, m_columnSrgLayout);

                if (!diffuseProbeGrid->GetBorderUpdateRowIrradianceSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetBorderUpdateRowIrradianceSrg()->Compile();
                }
                if(!diffuseProbeGrid->GetBorderUpdateColumnIrradianceSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetBorderUpdateColumnIrradianceSrg()->Compile();
                }
                if (!diffuseProbeGrid->GetBorderUpdateRowDistanceSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetBorderUpdateRowDistanceSrg()->Compile();
                }
                if (!diffuseProbeGrid->GetBorderUpdateColumnDistanceSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetBorderUpdateColumnDistanceSrg()->Compile();
                }
                // setup the submit items now to properly handle submitting on multiple threads
                uint32_t probeCountX;
                uint32_t probeCountY;
                diffuseProbeGrid->GetTexture2DProbeCount(probeCountX, probeCountY);

                // row irradiance
                {
                    SubmitItem& submitItem = m_submitItems.emplace_back();
                    submitItem.m_shaderResourceGroup = diffuseProbeGrid->GetBorderUpdateRowIrradianceSrg()->GetRHIShaderResourceGroup();
                    auto arguments = m_columnDispatchArgs;
                    arguments.m_totalNumberOfThreadsX = probeCountX * (DiffuseProbeGrid::DefaultNumIrradianceTexels + 2);
                    arguments.m_totalNumberOfThreadsY = probeCountY;
                    arguments.m_totalNumberOfThreadsZ = 1;
                    submitItem.m_dispatchItem.SetPipelineState(m_rowPipelineState);
                    submitItem.m_dispatchItem.SetArguments(arguments);
                }

                // column irradiance
                {
                    SubmitItem& submitItem = m_submitItems.emplace_back();
                    submitItem.m_shaderResourceGroup = diffuseProbeGrid->GetBorderUpdateColumnIrradianceSrg()->GetRHIShaderResourceGroup();
                    auto arguments = m_columnDispatchArgs;
                    arguments.m_totalNumberOfThreadsX = probeCountX;
                    arguments.m_totalNumberOfThreadsY = probeCountY * (DiffuseProbeGrid::DefaultNumIrradianceTexels + 2);
                    arguments.m_totalNumberOfThreadsZ = 1;
                    submitItem.m_dispatchItem.SetPipelineState(m_columnPipelineState);
                    submitItem.m_dispatchItem.SetArguments(arguments);
                }

                // row distance
                {
                    SubmitItem& submitItem = m_submitItems.emplace_back();
                    submitItem.m_shaderResourceGroup = diffuseProbeGrid->GetBorderUpdateRowDistanceSrg()->GetRHIShaderResourceGroup();
                    auto arguments = m_columnDispatchArgs;
                    arguments.m_totalNumberOfThreadsX = probeCountX * (DiffuseProbeGrid::DefaultNumDistanceTexels + 2);
                    arguments.m_totalNumberOfThreadsY = probeCountY;
                    arguments.m_totalNumberOfThreadsZ = 1;
                    submitItem.m_dispatchItem.SetPipelineState(m_rowPipelineState);
                    submitItem.m_dispatchItem.SetArguments(arguments);
                }

                // column distance
                {
                    SubmitItem& submitItem = m_submitItems.emplace_back();
                    submitItem.m_shaderResourceGroup = diffuseProbeGrid->GetBorderUpdateColumnDistanceSrg()->GetRHIShaderResourceGroup();
                    auto arguments = m_columnDispatchArgs;
                    arguments.m_totalNumberOfThreadsX = probeCountX;
                    arguments.m_totalNumberOfThreadsY = probeCountY * (DiffuseProbeGrid::DefaultNumDistanceTexels + 2);
                    arguments.m_totalNumberOfThreadsZ = 1;
                    submitItem.m_dispatchItem.SetPipelineState(m_columnPipelineState);
                    submitItem.m_dispatchItem.SetArguments(arguments);
                }
            }
        }

        void DiffuseProbeGridBorderUpdatePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            uint32_t index = context.GetSubmitRange().m_startIndex;
            while (index < context.GetSubmitRange().m_endIndex)
            {
                SubmitItem& submitItem = m_submitItems[index];

                commandList->SetShaderResourceGroupForDispatch(*submitItem.m_shaderResourceGroup->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));
                commandList->Submit(submitItem.m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()), index++);
            }
        }

        void DiffuseProbeGridBorderUpdatePass::FrameEndInternal()
        {
            m_submitItems.clear();

            RenderPass::FrameEndInternal();
        }
    }   // namespace Render
}   // namespace AZ
