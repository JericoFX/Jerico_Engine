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
        //! Parent pass for all Bloom related passes.
        //! The only difference between this and ParentPass is that this will check for available bloom settings
        //! and disable itself if no settings are found. See the IsEnabled override.
        class BloomParentPass final
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(BloomParentPass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::BloomParentPass, "AZ::Render::BloomParentPass", "{072861A3-A87A-439D-BD8B-D2BDD8D31799}", JE::Render::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(BloomParentPass, AZ::SystemAllocator);
            virtual ~BloomParentPass() = default;

            //! Creates a BloomParentPass
            static JE::Render::RPI::Ptr<BloomParentPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            //! Does a check for available BloomSettings, disable itself when the BloomSetting isn't available.
            bool IsEnabled() const override;

        private:
            BloomParentPass(const JE::Render::RPI::PassDescriptor& descriptor);
        };
    } // namespace Features
} // namespace JE::Render
