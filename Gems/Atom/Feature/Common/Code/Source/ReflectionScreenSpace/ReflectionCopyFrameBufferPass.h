/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>

namespace AZ
{
    namespace Render
    {
        //! This pass copies the frame buffer prior to the post-processing pass.
        class ReflectionCopyFrameBufferPass
            : public JE::Render::RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceCopyFrameBufferPass);

        public:
            AZ_RTTI(Render::ReflectionCopyFrameBufferPass, "{8B0D4281-0913-4662-81ED-37CB890B5653}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionCopyFrameBufferPass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionCopyFrameBufferPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionCopyFrameBufferPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void BuildInternal() override;
        };
    }   // namespace RPI
}   // namespace AZ
