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
        //! This pass downsamples the linear depth into a mip chain
        class ReflectionScreenSpaceDownsampleDepthLinearPass
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceDownsampleDepthLinearPass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ReflectionScreenSpaceDownsampleDepthLinearPass, "Render::ReflectionScreenSpaceDownsampleDepthLinearPass", "{5A215A02-2154-48D8-908D-351063BDB372}", ParentPass);
            AZ_CLASS_ALLOCATOR(JE::Render::Features::ReflectionScreenSpaceDownsampleDepthLinearPass, AZ::SystemAllocator);
          
            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionScreenSpaceDownsampleDepthLinearPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionScreenSpaceDownsampleDepthLinearPass(const JE::Render::RPI::PassDescriptor& descriptor);

            void CreateChildPassesInternal() override;

            // Pass Overrides...
            void ResetInternal() override;
            void BuildInternal() override;

            uint32_t m_mipLevels = 0;
            AZ::RHI::Size m_imageSize;
        };
    } // namespace Features
} // namespace JE::Render
