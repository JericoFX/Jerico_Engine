/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Render
    {
        //! ProjectedShadowmapsPass owns shadowmap passes for projected lights.
        class ProjectedShadowmapsPass final
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(ProjectedShadowmapsPass);
            using Base = JE::Render::RPI::ParentPass;

        public:
            AZ_CLASS_ALLOCATOR(ProjectedShadowmapsPass, SystemAllocator);
            AZ_RTTI(ProjectedShadowmapsPass, "00024B13-1095-40FA-BEC3-B0F68110BEA2", Base);

            virtual ~ProjectedShadowmapsPass();
            static JE::Render::RPI::Ptr<ProjectedShadowmapsPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides
            RHI::DrawListTag GetDrawListTag() const override;
            const JE::Render::RPI::PipelineViewTag& GetPipelineViewTag() const override;

            //! Sets the image to use as the output for all esm passes. This is needed so multiple pipelines in a scene can share the same resource.
            void SetAtlasAttachmentImage(Data::Instance<JE::Render::RPI::AttachmentImage> atlasAttachmentIamge);

            void QueueAddChild(JE::Render::RPI::Ptr<Pass> pass);
            void QueueRemoveChild(JE::Render::RPI::Ptr<Pass> pass);

        private:
            ProjectedShadowmapsPass() = delete;
            explicit ProjectedShadowmapsPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildInternal() override;

            RHI::DrawListTag m_drawListTag;
            JE::Render::RPI::PipelineViewTag m_pipelineViewTag;
            Data::Instance<JE::Render::RPI::AttachmentImage> m_atlasAttachmentImage;

            struct PassAddRemove
            {
                JE::Render::RPI::Ptr<Pass> m_pass;
                bool m_isRemoval;
            };
            AZStd::vector<PassAddRemove> m_passesToAddOrRemove;

        };
    } // namespace Render
} // namespace AZ
