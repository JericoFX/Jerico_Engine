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

namespace JE::Render
{
    namespace Features
    {
        //! This pass composites the screenspace reflection trace onto the reflection buffer.
        class ReflectionScreenSpaceCompositePass
            : public JE::Render::RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceCompositePass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ReflectionScreenSpaceCompositePass, "Render::ReflectionScreenSpaceCompositePass", "{88739CC9-C3F1-413A-A527-9916C697D93A}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(JE::Render::Features::ReflectionScreenSpaceCompositePass, AZ::SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionScreenSpaceCompositePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionScreenSpaceCompositePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            AZ::RHI::ShaderInputNameIndex m_outputScaleNameIndex = "m_outputScale";
            AZ::RHI::ShaderInputNameIndex m_outputWidthNameIndex = "m_outputWidth";
            AZ::RHI::ShaderInputNameIndex m_outputHeightNameIndex = "m_outputHeight";
            AZ::RHI::ShaderInputNameIndex m_maxRoughnessNameIndex = "m_maxRoughness";
        };
    } // namespace Features
} // namespace JE::Render
