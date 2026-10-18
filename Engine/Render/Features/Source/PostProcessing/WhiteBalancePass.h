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
        // Class for controlling White Balance effect
        class WhiteBalancePass final : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(WhiteBalancePass);

        public:
            AZ_RTTI(WhiteBalancePass, "{15AAF070-3258-4376-9911-CF4E9C7FAF4A}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(WhiteBalancePass, AZ::SystemAllocator, 0);

            ~WhiteBalancePass() = default;
            static JE::Render::RPI::Ptr<WhiteBalancePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            WhiteBalancePass(const JE::Render::RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Features
} // namespace JE::Render
