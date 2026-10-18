/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <PostProcessing/DepthOfFieldWriteFocusDepthFromGpuPass.h>
#include <PostProcessing/DepthOfFieldCopyFocusDepthToCpuPass.h>

namespace JE::Render
{
    namespace Features
    {
        class DepthOfFieldSettings;

        //! This pass is used to get the depth value for the specified screen coordinates.
        class DepthOfFieldReadBackFocusDepthPass final
            : public JE::Render::RPI::ParentPass
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::DepthOfFieldReadBackFocusDepthPass, "AZ::Render::DepthOfFieldReadBackFocusDepthPass", "{8738691C-1D8C-4F96-8B4F-2152A4550470}", JE::Render::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldReadBackFocusDepthPass, AZ::SystemAllocator);

            static JE::Render::RPI::Ptr<DepthOfFieldReadBackFocusDepthPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            DepthOfFieldReadBackFocusDepthPass(const JE::Render::RPI::PassDescriptor& descriptor);
            ~DepthOfFieldReadBackFocusDepthPass();

            // Set pass parameter interfaces...
            void SetScreenPosition(const AZ::Vector2& screenPosition);
            float GetFocusDepth();
            float GetNormalizedFocusDistanceForAutoFocus() const;

        protected:
            // Pass behavior overrides...
            void CreateChildPassesInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            void UpdateAutoFocus(DepthOfFieldSettings& dofSettings);

            // Used to avoid buffer name conflicts when creating multiple DepthOfFieldReadBackFocusDepthPass
            static uint32_t s_bufferInstance;
            AZ::Data::Instance<JE::Render::RPI::Buffer> m_buffer;

            JE::Render::RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> m_getDepthPass;
            JE::Render::RPI::Ptr<DepthOfFieldCopyFocusDepthToCpuPass> m_readbackPass;

            float m_normalizedFocusDistanceForAutoFocus = 0.0f;
            float m_delayTimer = 0.0f;
            bool m_isMovingFocus = false;
        };
    } // namespace Features
} // namespace JE::Render
