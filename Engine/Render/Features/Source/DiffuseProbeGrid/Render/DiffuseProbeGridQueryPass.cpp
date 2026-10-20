/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <DiffuseProbeGrid_Traits_Platform.h>
#include <Render/DiffuseProbeGridQueryPass.h>

namespace JE::Render
{
    namespace Features
    {
        JE::Render::RPI::Ptr<DiffuseProbeGridQueryPass> DiffuseProbeGridQueryPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            JE::Render::RPI::Ptr<DiffuseProbeGridQueryPass> pass = aznew DiffuseProbeGridQueryPass(descriptor);
            return pass;
        }

        DiffuseProbeGridQueryPass::DiffuseProbeGridQueryPass(const JE::Render::RPI::PassDescriptor& descriptor)
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

        void DiffuseProbeGridQueryPass::LoadShader()
        {
            // load shader
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridQuery.azshader";
            m_shader = JE::Render::RPI::LoadCriticalShader(shaderFilePath);
            if (m_shader == nullptr)
            {
                return;
            }

            // load pipeline state
            AZ::RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = m_shader->GetVariant(JE::Render::RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);

            // load Srg layout
            m_srgLayout = m_shader->FindShaderResourceGroupLayout(JE::Render::RPI::SrgBindingSlot::Pass);

            // retrieve the number of threads per thread group from the shader
            const auto outcome = JE::Render::RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), m_dispatchArgs);
            if (!outcome.IsSuccess())
            {
                AZ_Error("PassSystem", false, "[DiffuseProbeGridQueryPass '%s']: Shader '%s' contains invalid numthreads arguments:\n%s", GetPathName().GetCStr(), shaderFilePath.c_str(), outcome.GetError().c_str());
            }
        }

        bool DiffuseProbeGridQueryPass::IsEnabled() const
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

            // Note: the pass is enabled even if none of the queries are in a DiffuseProbeGrid volume.  This is necessary to provide a zero result
            // for those queries in the transient output buffer.
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            if (!diffuseProbeGridFeatureProcessor || !diffuseProbeGridFeatureProcessor->GetIrradianceQueryCount())
            {
                // no irradiance queries
                return false;
            }

            return true;
        }

        void DiffuseProbeGridQueryPass::BuildInternal()
        {
            AZStd::string uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
            m_outputBufferAttachmentId = AZStd::string::format("DiffuseProbeGridQueryOutputBuffer_%s", uuidString.c_str());

            // setup output PassAttachment
            m_outputAttachment = aznew JE::Render::RPI::PassAttachment();
            m_outputAttachment->m_name = "Output";
            AZ::Name attachmentPath(AZStd::string::format("%s.%s", GetPathName().GetCStr(), m_outputAttachment->m_name.GetCStr()));
            m_outputAttachment->m_path = attachmentPath;
            m_outputAttachment->m_lifetime = AZ::RHI::AttachmentLifetimeType::Transient;

            JE::Render::RPI::PassAttachmentBinding* outputBinding = FindAttachmentBinding(AZ::Name("Output"));
            AZ_Assert(outputBinding, "Failed to find Output slot on DiffuseProbeGridQueryPass");
            outputBinding->SetAttachment(m_outputAttachment);
        }

        void DiffuseProbeGridQueryPass::FrameBeginInternal(FramePrepareParams params)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // create output buffer descriptors
            m_outputBufferDesc.m_byteCount = diffuseProbeGridFeatureProcessor->GetIrradianceQueryCount() * sizeof(AZ::Vector4);
            m_outputBufferDesc.m_bindFlags = AZ::RHI::BufferBindFlags::ShaderReadWrite;
            m_outputBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, diffuseProbeGridFeatureProcessor->GetIrradianceQueryCount(), AZ::RHI::Format::R32G32B32A32_FLOAT);

            m_outputAttachment->m_descriptor = m_outputBufferDesc;

            // create transient buffer
            AZ::RHI::TransientBufferDescriptor transientBufferDesc;
            transientBufferDesc.m_attachmentId = m_outputBufferAttachmentId;
            transientBufferDesc.m_bufferDescriptor = m_outputBufferDesc;
            params.m_frameGraphBuilder->GetAttachmentDatabase().CreateTransientBuffer(transientBufferDesc);

            RenderPass::FrameBeginInternal(params);
        }

        void DiffuseProbeGridQueryPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            frameGraph.SetEstimatedItemCount(aznumeric_cast<uint32_t>(diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids().size()));

            // query buffer
            {
                AZ::RHI::AttachmentId attachmentId = diffuseProbeGridFeatureProcessor->GetQueryBufferAttachmentId();
                AZ::RHI::Ptr<AZ::RHI::Buffer> buffer = diffuseProbeGridFeatureProcessor->GetQueryBuffer()->GetRHIBuffer();

                if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentId))
                {
                    [[maybe_unused]] AZ::RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(attachmentId, buffer);
                    AZ_Assert(result == AZ::RHI::ResultCode::Success, "Failed to import query buffer");
                }

                AZ::RHI::BufferScopeAttachmentDescriptor desc;
                desc.m_attachmentId = attachmentId;
                desc.m_bufferViewDescriptor = diffuseProbeGridFeatureProcessor->GetQueryBufferViewDescriptor();
                desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::ComputeShader);
            }

            // output buffer
            {
                AZ::RHI::BufferScopeAttachmentDescriptor desc;
                desc.m_attachmentId = m_outputBufferAttachmentId;
                desc.m_bufferViewDescriptor = m_outputBufferViewDesc;
                desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Clear;
                desc.m_loadStoreAction.m_clearValue = AZ::RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);

                frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite, AZ::RHI::ScopeAttachmentStage::ComputeShader);
            }

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                // grid data buffer
                {
                    AZ::RHI::BufferScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetGridDataBufferAttachmentId();
                    desc.m_bufferViewDescriptor = diffuseProbeGrid->GetRenderData()->m_gridDataBufferViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe irradiance
                {
                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe distance
                {
                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::ComputeShader);
                }

                // probe data
                {
                    AZ::RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetProbeDataImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDataImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::ComputeShader);
                }
            }
        }

        void DiffuseProbeGridQueryPass::CompileResources([[maybe_unused]] const AZ::RHI::FrameGraphCompileContext& context)
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();
            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids())
            {
                // update DiffuseProbeGrid-specific bindings
                diffuseProbeGrid->UpdateQuerySrg(m_shader, m_srgLayout);

                // bind query buffer
                AZ::RHI::ShaderInputBufferIndex bufferIndex = m_srgLayout->FindShaderInputBufferIndex(AZ::Name("m_irradianceQueries"));
                AZ::RHI::Ptr<AZ::RHI::Buffer> buffer = diffuseProbeGridFeatureProcessor->GetQueryBuffer()->GetRHIBuffer();
                AZ::RHI::BufferViewDescriptor bufferViewDescriptor = diffuseProbeGridFeatureProcessor->GetQueryBufferViewDescriptor();
                diffuseProbeGrid->GetQuerySrg()->SetBufferView(bufferIndex, buffer->GetBufferView(bufferViewDescriptor).get());

                // bind output UAV
                bufferIndex = m_srgLayout->FindShaderInputBufferIndex(AZ::Name("m_output"));
                const AZ::RHI::BufferView* bufferView = context.GetBufferView(AZ::Name(m_outputBufferAttachmentId.GetCStr()), AZ::RHI::ScopeAttachmentUsage::Shader);
                diffuseProbeGrid->GetQuerySrg()->SetBufferView(bufferIndex, bufferView);

                diffuseProbeGrid->GetQuerySrg()->Compile();
            }
        }

        void DiffuseProbeGridQueryPass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
        {
            AZ::RHI::CommandList* commandList = context.GetCommandList();
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            // submit the DispatchItems for each DiffuseProbeGrid in this range
            for (uint32_t index = context.GetSubmitRange().m_startIndex; index < context.GetSubmitRange().m_endIndex; ++index)
            {
                AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = diffuseProbeGridFeatureProcessor->GetVisibleProbeGrids()[index];

                const AZ::RHI::ShaderResourceGroup* shaderResourceGroup = diffuseProbeGrid->GetQuerySrg()->GetRHIShaderResourceGroup();
                commandList->SetShaderResourceGroupForDispatch(*shaderResourceGroup->GetDeviceShaderResourceGroup(context.GetDeviceIndex()));

                AZ::RHI::DeviceDispatchItem dispatchItem;
                dispatchItem.m_arguments = m_dispatchArgs;
                dispatchItem.m_pipelineState = m_pipelineState->GetDevicePipelineState(context.GetDeviceIndex()).get();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = diffuseProbeGridFeatureProcessor->GetIrradianceQueryCount();
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = 1;
                dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;

                commandList->Submit(dispatchItem, index);
            }
        }

        void DiffuseProbeGridQueryPass::FrameEndInternal()
        {
            JE::Render::RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            diffuseProbeGridFeatureProcessor->ClearIrradianceQueries();

            RenderPass::FrameEndInternal();
        }
    }   // namespace Features
}   // namespace JE::Render
