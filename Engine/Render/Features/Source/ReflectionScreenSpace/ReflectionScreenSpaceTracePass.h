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
        //! This pass traces screenspace reflections from the previous frame image.
        class ReflectionScreenSpaceTracePass
            : public JE::Render::RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceTracePass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ReflectionScreenSpaceTracePass, "Render::ReflectionScreenSpaceTracePass", "{70FD45E9-8363-4AA1-A514-3C24AC975E53}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(JE::Render::Features::ReflectionScreenSpaceTracePass, AZ::SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionScreenSpaceTracePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            AZ::Data::Instance<JE::Render::RPI::AttachmentImage>& GetPreviousFrameImageAttachment() { return m_previousFrameImageAttachment; }

        private:
            explicit ReflectionScreenSpaceTracePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildInternal() override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            AZ::Data::Instance<JE::Render::RPI::AttachmentImage> m_previousFrameImageAttachment;

            AZ::RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            AZ::RHI::ShaderInputNameIndex m_outputWidthNameIndex = "m_outputWidth";
            AZ::RHI::ShaderInputNameIndex m_outputHeightNameIndex = "m_outputHeight";
            AZ::RHI::ShaderInputNameIndex m_rayTracingEnabledNameIndex = "m_rayTracingEnabled";
            AZ::RHI::ShaderInputNameIndex m_rayTraceFallbackDataNameIndex = "m_rayTraceFallbackData";
            AZ::RHI::ShaderInputNameIndex m_maxRayDistanceNameIndex = "m_maxRayDistance";
            AZ::RHI::ShaderInputNameIndex m_maxDepthThresholdNameIndex = "m_maxDepthThreshold";
            AZ::RHI::ShaderInputNameIndex m_maxRoughnessNameIndex = "m_maxRoughness";
            AZ::RHI::ShaderInputNameIndex m_roughnessBiasNameIndex = "m_roughnessBias";
        };
    } // namespace Features
} // namespace JE::Render
