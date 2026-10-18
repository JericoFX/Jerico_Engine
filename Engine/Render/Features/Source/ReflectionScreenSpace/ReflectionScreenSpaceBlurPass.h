/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>

namespace JE::Render
{
    namespace Features
    {
        //! This pass performs a separable Gaussian blur of the input reflection image
        //! to the lower mip levels of that image.  The blurred mips are used as roughness levels
        //! when applying reflection data to a surface material.
        class ReflectionScreenSpaceBlurPass
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceBlurPass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ReflectionScreenSpaceBlurPass, "Render::ReflectionScreenSpaceBlurPass", "{BC3D92C5-E38A-46FE-8EBD-CAD14E505946}", ParentPass);
            AZ_CLASS_ALLOCATOR(JE::Render::Features::ReflectionScreenSpaceBlurPass, AZ::SystemAllocator);
          
            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionScreenSpaceBlurPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionScreenSpaceBlurPass(const JE::Render::RPI::PassDescriptor& descriptor);

            void CreateChildPassesInternal() override;

            // Pass Overrides...
            void ResetInternal() override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            AZ::RHI::Size m_imageSize;
            uint32_t m_mipLevels = 0;

            AZStd::vector<JE::Render::RPI::Ptr<JE::Render::RPI::FullscreenTrianglePass>> m_verticalBlurChildPasses;
            AZStd::vector<JE::Render::RPI::Ptr<JE::Render::RPI::FullscreenTrianglePass>> m_horizontalBlurChildPasses;
        };
    } // namespace Features
} // namespace JE::Render
