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
        //! Parent pass for all Depth of Field related passes.
        //! The only difference between this and ParentPass is that this will check for available depth of field settings
        //! and disable itself if no settings are found. See the IsEnabled override.
        class DepthOfFieldParentPass final
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(DepthOfFieldParentPass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::DepthOfFieldParentPass, "AZ::Render::DepthOfFieldParentPass", "{6033066A-CA95-422E-9BF2-8C203171C1A8}", JE::Render::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldParentPass, AZ::SystemAllocator);
            virtual ~DepthOfFieldParentPass() = default;

            //! Creates a DepthOfFieldParentPass
            static JE::Render::RPI::Ptr<DepthOfFieldParentPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        private:
            DepthOfFieldParentPass(const JE::Render::RPI::PassDescriptor& descriptor);
        };
    } // namespace Features
} // namespace JE::Render
