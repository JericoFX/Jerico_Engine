/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>

namespace AZ
{
    namespace Render
    {
        // Class for controlling Panini Projection effect
        class PaniniProjectionPass final : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(PaniniProjectionPass);

        public:
            AZ_RTTI(PaniniProjectionPass, "{DBFE786B-16DE-4F44-8188-E4E753270485}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(PaniniProjectionPass, SystemAllocator, 0);

            ~PaniniProjectionPass() = default;
            static JE::Render::RPI::Ptr<PaniniProjectionPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            PaniniProjectionPass(const JE::Render::RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Render
} // namespace AZ
