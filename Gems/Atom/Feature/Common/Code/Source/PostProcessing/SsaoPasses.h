/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        // Parent pass for SSAO that contains the SSAO compute pass 
        class SsaoParentPass final
            : public JE::Render::RPI::ParentPass
        {
            AZ_RPI_PASS(SsaoParentPass);

        public:
            AZ_RTTI(AZ::Render::SsaoParentPass, "{A03B5913-B201-4146-AF0A-999E6BF31A1A}", JE::Render::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(SsaoParentPass, SystemAllocator);
            virtual ~SsaoParentPass() = default;

            //! Creates an SsaoParentPass
            static JE::Render::RPI::Ptr<SsaoParentPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            SsaoParentPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Direct pointers to child passes for enable/disable
            ParentPass* m_blurParentPass = nullptr;
            FastDepthAwareBlurHorPass* m_blurHorizontalPass = nullptr;
            FastDepthAwareBlurVerPass* m_blurVerticalPass = nullptr;
            Pass* m_downsamplePass = nullptr;
            Pass* m_upsamplePass = nullptr;
        };

        // Computer shader pass that calculates SSAO from a linear depth buffer
        class SsaoComputePass final
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(SsaoComputePass);

        public:
            AZ_RTTI(AZ::Render::SsaoComputePass, "{0BA5F6F7-15D2-490A-8254-7E61F25B62F9}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(SsaoComputePass, SystemAllocator);
            virtual ~SsaoComputePass() = default;

            //! Creates an SsaoComputePass
            static JE::Render::RPI::Ptr<SsaoComputePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            SsaoComputePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    }   // namespace Render
}   // namespace AZ
