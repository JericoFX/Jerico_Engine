/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ParentPass.h>

namespace JE::Render
{
    namespace Features
    {
        //! Parent pass for screenspace reflections, implements a frame delay to allow time
        //! for the previous frame texture to be populated
        class ReflectionScreenSpacePass
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(ReflectionScreenSpacePass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ReflectionScreenSpacePass, "Render::ReflectionScreenSpacePass", "{0B27D7F1-F914-4D09-A46D-3E63404771E3}", ParentPass);
            AZ_CLASS_ALLOCATOR(JE::Render::Features::ReflectionScreenSpacePass, AZ::SystemAllocator, 0);
          
            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionScreenSpacePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void ResetFrameDelay() { m_frameDelayCount = 0; }

        private:
            explicit ReflectionScreenSpacePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            bool IsEnabled() const override;

            mutable uint32_t m_frameDelayCount = 0;
        };
    } // namespace Features
} // namespace JE::Render
