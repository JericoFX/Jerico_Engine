/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/ShadowmapPass.h>
#include <JE/Render/RPI/Public/Pass/PassUtils.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/RenderPipeline.h>
#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        // --- Pass Creation ---

        JE::Render::RPI::Ptr<JE::Render::Features::ShadowmapPass> ShadowmapPass::Create(const JE::Render::RPI::PassDescriptor& descriptor)
        {
            return aznew ShadowmapPass(descriptor);
        }

        ShadowmapPass::ShadowmapPass(const JE::Render::RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            m_overrideViewportState = true;
            m_overrideScissorSate = true;
        }

        JE::Render::RPI::Ptr<JE::Render::Features::ShadowmapPass> ShadowmapPass::CreateWithPassRequest(const AZ::Name& passName, AZStd::shared_ptr<JE::Render::RPI::RasterPassData> passData)
        {
            // Create a pass request for the descriptor so we can connect it to the parent class input connections
            JE::Render::RPI::PassRequest childRequest;
            childRequest.m_templateName = AZ::Name{ "ShadowmapPassTemplate" };
            childRequest.m_passName = passName;

            // Add a connection to the skinned mesh input
            JE::Render::RPI::PassConnection passConnection;
            passConnection.m_localSlot = AZ::Name{ "SkinnedMeshes" };
            passConnection.m_attachmentRef.m_pass = AZ::Name{ "Parent" };
            passConnection.m_attachmentRef.m_attachment = AZ::Name{ "SkinnedMeshes" };
            childRequest.m_connections.emplace_back(passConnection);

            // Get the template
            const AZStd::shared_ptr<const JE::Render::RPI::PassTemplate> childTemplate = JE::Render::RPI::PassSystemInterface::Get()->GetPassTemplate(childRequest.m_templateName);
            AZ_Assert(childTemplate, "ShadowmapPass::CreateWIthPassRequest - attempting to create a shadowmap pass before the template has been created.");

            // Create the pass
            JE::Render::RPI::PassDescriptor descriptor{ passName, childTemplate, &childRequest };
            descriptor.m_passData = AZStd::move(passData);
            return Create(descriptor);
        }

        void ShadowmapPass::CreatePassTemplate()
        {
            AZStd::shared_ptr<JE::Render::RPI::PassTemplate> m_childTemplate = AZStd::make_shared<JE::Render::RPI::PassTemplate>();
            m_childTemplate->m_name = AZ::Name{ "ShadowmapPassTemplate" };
            m_childTemplate->m_passClass = "ShadowmapPass";

            m_childTemplate->m_slots.resize(2);
            JE::Render::RPI::PassSlot& slot = m_childTemplate->m_slots[0];
            slot.m_name = AZ::Name{ "Shadowmap" };
            slot.m_slotType = JE::Render::RPI::PassSlotType::Output;
            slot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::DepthStencil;

            // This slot it used to create a connection between the skinned mesh compute pass and the cascade shadow map pass
            JE::Render::RPI::PassSlot& skinnedMeshSlot = m_childTemplate->m_slots[1];
            skinnedMeshSlot.m_name = AZ::Name{ "SkinnedMeshes" };
            skinnedMeshSlot.m_slotType = JE::Render::RPI::PassSlotType::Input;
            skinnedMeshSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::InputAssembly;

            m_childTemplate->m_connections.resize(1);
            JE::Render::RPI::PassConnection& connection = m_childTemplate->m_connections[0];
            connection.m_localSlot = AZ::Name{ "Shadowmap" };
            connection.m_attachmentRef.m_pass = AZ::Name{ "Parent" };
            connection.m_attachmentRef.m_attachment = AZ::Name{ "Shadowmap" };

            JE::Render::RPI::PassSystemInterface::Get()->AddPassTemplate(AZ::Name{ "ShadowmapPassTemplate" }, m_childTemplate);
        }

        // --- Build Override ---

        void ShadowmapPass::BuildInternal()
        {
            JE::Render::RPI::Ptr<JE::Render::RPI::ParentPass> parentPass = GetParent();
            if (!parentPass)
            {
                return;
            }

            AZ::RHI::ImageViewDescriptor imageViewDescriptor;
            imageViewDescriptor.m_arraySliceMin = m_arraySlice;
            imageViewDescriptor.m_arraySliceMax = m_arraySlice;

            JE::Render::RPI::PassAttachmentBinding& binding = GetOutputBinding(0);

            JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment> attachment = parentPass->GetOutputBinding(0).GetAttachment();
            if (!attachment)
            {
                AZ_Assert(false, "[ShadowmapPass %s] Cannot find shadowmap image attachment.", GetPathName().GetCStr());
                return;
            }
            const AZ::RHI::AttachmentId attachmentId = attachment->GetAttachmentId();

            AZ::RHI::AttachmentLoadStoreAction action;
            action.m_clearValue = AZ::RHI::ClearValue::CreateDepth(1.f);
            action.m_loadAction = m_clearEnabled ? AZ::RHI::AttachmentLoadAction::Clear : AZ::RHI::AttachmentLoadAction::DontCare;
            action.m_loadActionStencil = AZ::RHI::AttachmentLoadAction::None;
            action.m_storeActionStencil = AZ::RHI::AttachmentStoreAction::None;
            binding.m_unifiedScopeDesc = AZ::RHI::UnifiedScopeAttachmentDescriptor(attachmentId, imageViewDescriptor, action);

            Base::BuildInternal();
        }

        // --- Setters ---

        void ShadowmapPass::SetArraySlice(uint16_t arraySlice)
        {
            m_arraySlice = arraySlice;
        }

        void ShadowmapPass::SetClearEnabled(bool enabled)
        {
            m_clearEnabled = enabled;
        }

        void ShadowmapPass::SetIsStatic(bool isStatic)
        {
            m_isStatic = isStatic;
        }

        void ShadowmapPass::SetCasterMovedBit(AZ::RHI::Handle<uint32_t> bit)
        {
            m_casterMovedBit = bit;
        }

        void ShadowmapPass::ForceRenderNextFrame()
        {
            m_forceRenderNextFrame = true;
        }

        void ShadowmapPass::SetViewportScissorFromImageSize(const AZ::RHI::Size& imageSize)
        {
            const AZ::RHI::Viewport viewport(
                0.f, imageSize.m_width * 1.f,
                0.f, imageSize.m_height * 1.f);
            const AZ::RHI::Scissor scissor(
                0, 0, imageSize.m_width, imageSize.m_height);
            SetViewportScissor(viewport, scissor);
        }

        void ShadowmapPass::SetViewportScissor(const AZ::RHI::Viewport& viewport, const AZ::RHI::Scissor& scissor)
        {
            m_viewportState = viewport;
            m_scissorState = scissor;
        }

        void ShadowmapPass::SetClearShadowDrawPacket(AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> clearShadowDrawPacket)
        {
            m_clearShadowDrawPacket = clearShadowDrawPacket;
        }

        void ShadowmapPass::UpdatePipelineViewTag(const JE::Render::RPI::PipelineViewTag& viewTag)
        {
            SetPipelineViewTag(viewTag);
        }
        
        void ShadowmapPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
        {
            Base::SetupFrameGraphDependencies(frameGraph);

            // Report + 1 to make room for the clear draw packet.
            uint32_t estimatedItemCount = static_cast<uint32_t>(m_drawListView.size() + 1);

            // Override the estimated item count set by the base class. Draw item count is compared against
            // the last frame to detect cases where a moving object leaves the shadow frustum. It wouldn't
            // set m_casterMovedBit since its outside the view, but needs to trigger a re-render anyway.
            if (m_isStatic && !m_forceRenderNextFrame && m_lastFrameDrawCount == m_drawItemCount)
            {
                const auto& views = m_pipeline->GetViews(GetPipelineViewTag());
                if (!views.empty())
                {
                    const JE::Render::RPI::ViewPtr& view = views.front();
                    if (view && (view->GetOrFlags() & m_casterMovedBit.GetIndex()) == 0)
                    {
                        // Shadow is static and no casters moved since last frame.
                        estimatedItemCount = 0;
                    }
                }
            }
            frameGraph.SetEstimatedItemCount(estimatedItemCount);
        }

        void ShadowmapPass::SubmitDrawItems(const AZ::RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex, uint32_t offset) const
        {
            if (m_clearShadowDrawPacket)
            {
                if (startIndex == 0)
                {
                    AZ::RHI::CommandList* commandList = context.GetCommandList();
                    commandList->Submit(m_clearShadowDrawPacket->GetDrawItemProperties(0).m_item->GetDeviceDrawItem(context.GetDeviceIndex()), 0);
                }
                else
                {
                    // Only decrement startIndex if startIndex is greater than 0. This means that RasterPass will submit one less
                    // draw item for the first batch since the clear draw was submitted in this batch.
                    --startIndex; 
                }
                // RasterPass's draw item indices need to be adjusted to make sure it stays inside its expected bounds.
                --endIndex;
                ++offset;
            }
            Base::SubmitDrawItems(context, startIndex, endIndex, offset);
        }

        void ShadowmapPass::FrameEndInternal()
        {
            if (m_clearShadowDrawPacket)
            {
                // If m_clearShadowDrawPacket is valid, then this pass would have rendered this frame if
                // m_forceRenderNextFrame was set to true, so set it to false now. This can't be done safely
                // in SubmitDrawItems because it may be called multiple times from different threads.
                m_forceRenderNextFrame = false;
            }
            m_lastFrameDrawCount = m_drawItemCount;
            Base::FrameEndInternal();
        }

    } // namespace Features
} // namespace JE::Render
