/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI/DrawPacket.h>
#include <JE/Render/RPI/Public/Pass/RasterPass.h>
#include <JE/Render/RPI/Reflect/Pass/RasterPassData.h>

namespace JE::Render
{
    namespace Features
    {
        //! ShadowmapPass outputs shadowmap depth images.
        class ShadowmapPass final
            : public JE::Render::RPI::RasterPass
        {
            friend class CascadedShadowmapsPass;
            friend class ProjectedShadowmapsPass;

            using Base = JE::Render::RPI::RasterPass;
            AZ_RPI_PASS(ShadowmapPass);
      
        public:
            AZ_RTTI(ShadowmapPass, "FCBDDB8C-E565-4780-9E2E-B45F16203F77", Base);
            AZ_CLASS_ALLOCATOR(ShadowmapPass, AZ::SystemAllocator);

            ~ShadowmapPass() = default;

            static JE::Render::RPI::Ptr<ShadowmapPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            // Creates a common pass template for the child shadowmap passes.
            static void CreatePassTemplate();

            //! Creates a pass descriptor from the input, using the ShadowmapPassTemplate, and adds a pass request to connect to the parent pass.
            //! This function assumes the parent pass has a SkinnedMeshes input slot
            static JE::Render::RPI::Ptr<ShadowmapPass> CreateWithPassRequest(const AZ::Name& passName, AZStd::shared_ptr<JE::Render::RPI::RasterPassData> passData);

            //! This updates array slice for this shadowmap.
            void SetArraySlice(uint16_t arraySlice);

            //! This enable/disable clearing of the image view.
            void SetClearEnabled(bool enabled);

            //! Sets this shadow pass to only update its shadows when a mesh in view moves.
            void SetIsStatic(bool isStatic);

            //! Sets which bit to check to see if a caster in the view moved.
            void SetCasterMovedBit(AZ::RHI::Handle<uint32_t> bit);

            //! When the shadow is static, this forces the shadow to still re-render next frame (due to the light moving for instance)
            void ForceRenderNextFrame();

            //! This update viewport and scissor for this shadowmap from the given image size.
            void SetViewportScissorFromImageSize(const AZ::RHI::Size& imageSize);

            //! This updates viewport and scissor for this shadowmap.
            void SetViewportScissor(const AZ::RHI::Viewport& viewport, const AZ::RHI::Scissor& scissor);

            //! Sets the draw packet used for clearing a shadow viewport.
            void SetClearShadowDrawPacket(AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> clearShadowDrawPacket);

        private:
            ShadowmapPass() = delete;
            explicit ShadowmapPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Sets the underlying pipeline view tag. Purpose of this function is to allow parent shadow passes to update the view tag.
            void UpdatePipelineViewTag(const JE::Render::RPI::PipelineViewTag& viewTag);

            // RHI::Pass overrides...
            void BuildInternal() override;
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void FrameEndInternal() override;

            // RPI::RasterPass overrides...
            void SubmitDrawItems(const AZ::RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex, uint32_t indexOffset) const override;

            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_clearShadowDrawPacket;
            AZ::RHI::Handle<uint32_t> m_casterMovedBit;
            uint16_t m_arraySlice = 0;
            bool m_clearEnabled = true;
            bool m_isStatic = false;
            uint32_t m_lastFrameDrawCount = 0;
            mutable bool m_forceRenderNextFrame = false;
        };
    } // namespace Features
} // namespace JE::Render
