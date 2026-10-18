/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>

namespace JE::Render
{
    namespace Features
    {
        //! This pass takes performs a separable Gaussian blur of the input reflection image
        //! to the lower mip levels of that image.  The blurred mips are used as roughness levels
        //! when applying reflection data to a surface material.
        class ReflectionScreenSpaceBlurChildPass
            : public JE::Render::RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceBlurChildPass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ReflectionScreenSpaceBlurChildPass, "Render::ReflectionScreenSpaceBlurChildPass", "{238E4D6C-3213-4BA2-8DFE-EAC469346E77}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(JE::Render::Features::ReflectionScreenSpaceBlurChildPass, AZ::SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionScreenSpaceBlurChildPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            enum PassType
            {
                Vertical,
                Horizontal
            };

            void SetType(PassType passType) { m_passType = passType; }
            void SetMipLevel(uint32_t mipLevel) { m_mipLevel = mipLevel; }

        private:
            explicit ReflectionScreenSpaceBlurChildPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            AZ::RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            AZ::RHI::ShaderInputNameIndex m_mipLevelNameIndex = "m_mipLevel";

            bool m_updateSrg = false;
            PassType m_passType;
            uint32_t m_mipLevel = 0;
            AZ::RHI::Size m_imageSize;
            float m_invOutputScale = 1.0f;
        };
    } // namespace Features
} // namespace JE::Render
