/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <CoreLights/ShadowmapAtlas.h>
#include <CoreLights/ShadowmapPass.h>

namespace AZ
{
    namespace Render
    {
        //! CascadedShadowmapsPass owns and manages ShadowmapPasses.
        class CascadedShadowmapsPass final
            : public JE::Render::RPI::ParentPass
        {
            using Base = JE::Render::RPI::ParentPass;
            AZ_RPI_PASS(CascadedShadowmapsPass);

        public:
            AZ_RTTI(CascadedShadowmapsPass, "3956C19A-FBCB-4884-8AA9-3B47EFEC2B33", Base);
            AZ_CLASS_ALLOCATOR(CascadedShadowmapsPass, SystemAllocator);

            virtual ~CascadedShadowmapsPass();
            static JE::Render::RPI::Ptr<CascadedShadowmapsPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            //! This sets camera view name.
            void SetCameraViewName(const AZStd::string& viewName);

            //! This returns pipeline view tag for children.
            const AZStd::span<const JE::Render::RPI::PipelineViewTag> GetPipelineViewTags();

            //! This exposes the shadowmap atlas.
            ShadowmapAtlas& GetShadowmapAtlas() { return m_atlas; }

            //! This queues the image size and array size which will be updated in the beginning of the frame.
            void SetShadowmapSize(ShadowmapSize shadowmapSize, u16 numCascades);

        private:
            CascadedShadowmapsPass() = delete;
            explicit CascadedShadowmapsPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildInternal() override;
            void GetPipelineViewTags(JE::Render::RPI::PipelineViewTags& outTags) const override;
            void GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, JE::Render::RPI::PassesByDrawList& outPassesByDrawList, const JE::Render::RPI::PipelineViewTag& viewTag) const override;

            void CreateChildPassesInternal() override;
            void CreateChildShadowMapPass(u16 cascadeIndex);

            void UpdateShadowmapImageSize();

            const Name m_slotName{ "Shadowmap" };
            Name m_drawListTagName;
            RHI::DrawListTag m_drawListTag;

            // The name of the camera view associated to the shadow.
            // It is used to generate distinct child's pipeline view tags for each camera view.
            AZStd::string m_cameraViewName;

            // Generated pipeline view tags for the children (ShadowmapPass).
            AZStd::fixed_vector<JE::Render::RPI::PipelineViewTag, Shadow::MaxNumberOfCascades> m_childrenPipelineViewTags;
            u16 m_numCascades = 0;

            ShadowmapAtlas m_atlas;
            ShadowmapSize m_shadowmapSize = ShadowmapSize::None;
        };
    } // namespace Render
} // namespace AZ
