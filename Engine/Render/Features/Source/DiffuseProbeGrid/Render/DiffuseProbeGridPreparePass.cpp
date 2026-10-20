/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridPreparePass.h>

namespace JE::Render
{
    namespace Features
    {
        constexpr AZStd::string_view DiffuseProbeGridPrepareShaderProductAssetPath =
            "shaders/diffuseglobalillumination/diffuseprobegridprepare.azshader";

        JE::Render::RPI::Ptr<DiffuseProbeGridPreparePass> DiffuseProbeGridPreparePass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DiffuseProbeGridPreparePass> pass = aznew DiffuseProbeGridPreparePass(descriptor);
            return pass;
        }

        DiffuseProbeGridPreparePass::DiffuseProbeGridPreparePass(const JE::Render::RPI::PassDescriptor& descriptor)
            : JE::Render::RPI::RenderPass(descriptor)
        {
            if (!JE_TRAIT_DIFFUSE_GI_PASSES_SUPPORTED)
            {
                // GI is not supported on this platform
                SetEnabled(false);
            }
            else
            {
                LoadShader();
            }
        }

        void DiffuseProbeGridPreparePass::LoadShader()
        {
            // load shader
            // Note: the shader may not be available on all platforms
            m_shader = JE::Render::RPI::LoadCriticalShader(DiffuseProbeGridPrepareShaderProductAssetPath);
            if (m_shader == nullptr)
            {
                return;
            }

            // load pipeline state
            AZ::RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = m_shader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);

            // load Pass Srg asset
            m_srgLayout = m_shader->FindShaderResourceGroupLayout(JE::Render::RPI::SrgBindingSlot::Pass);

            // retrieve the number of threads per thread group from the shader
            const auto outcome = JE::Render::RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), m_dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false,
                    "[DiffuseProbeGridPreparePass '%s']: Shader '" AZ_STRING_FORMAT "' contains invalid numthreads arguments:\n%s",
                    GetPathName().GetCStr(),
                    AZ_STRING_ARG(DiffuseProbeGridPrepareShaderProductAssetPath),
                    outcome.GetError().c_str());
            }
        }

        bool DiffuseProbeGridPreparePass::IsEnabled() const
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
            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids().empty())
            {
                // no diffuse probe grids
                return false;
            }

            return true;
        }

        void DiffuseProbeGridPreparePass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids().size()));

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                // grid data buffer
                {
                    [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(diffuseProbeGrid->GetGridDataBufferAttachmentId(), diffuseProbeGrid->GetGridDataBuffer());
                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import grid data buffer");

                    AZ::RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                    desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(
                        desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite, AZ::RHI::ScopeAttachmentStage::ComputeShader);
                }
            }
        }

        void DiffuseProbeGridPreparePass::CompileResources([[maybe_unused]] const AZ::RHI::FrameGraphCompileContext& context)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see ValidateSetImageView() in ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdatePrepareSrg(m_shader, m_srgLayout);
                if (!diffuseProbeGrid->GetPrepareSrg()->IsQueuedForCompile())
                {
                    diffuseProbeGrid->GetPrepareSrg()->Compile();
                }
            }
        }

        void DiffuseProbeGridPreparePass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
        {
            AZ::RHI::CommandList* commandList = context.GetCommandList();
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids()[index];

                const AZ::RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetPrepareSrg()->GetRHIShaderResourceGroup();
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));

                AZ::RHI::DeviceDispatchItem dispatchItem;
                dispatchItem.m_arguments = m_dispatchArgs;
                dispatchItem.m_pipelineState = m_pipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = 1;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = 1;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem, index);
            }
        }
    }   // namespace Features
}   // namespace JE::Render
