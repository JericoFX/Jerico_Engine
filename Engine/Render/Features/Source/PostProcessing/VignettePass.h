/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>

namespace JE::Render
{
    namespace Features
    {
        // Class for controlling Vignette effect
        class VignettePass final : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(VignettePass);

        public:
            AZ_RTTI(VignettePass, "{69228453-67F9-473D-ACD6-FA712A23FD23}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(VignettePass, AZ::SystemAllocator, 0);

            ~VignettePass() = default;
            static JE::Render::RPI::Ptr<VignettePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            VignettePass(const JE::Render::RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Features
} // namespace JE::Render
