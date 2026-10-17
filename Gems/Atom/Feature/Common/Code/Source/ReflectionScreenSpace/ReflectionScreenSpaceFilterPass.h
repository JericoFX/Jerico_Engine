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
        //! This pass filters the screenspace reflection image
        class ReflectionScreenSpaceFilterPass
            : public JE::Render::RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceFilterPass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceFilterPass, "{54F8F4FC-73DD-4312-B474-3CCB3AAE216A}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceFilterPass, SystemAllocator, 0);

            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<ReflectionScreenSpaceFilterPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionScreenSpaceFilterPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildInternal() override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void FrameEndInternal() override;

            void CreateHistoryAttachmentImage(JE::Render::RPI::Ptr<JE::Render::RPI::PassAttachment>& historyAttachment);

            static const uint32_t ImageFrameCount = 3;
            Data::Instance<JE::Render::RPI::AttachmentImage> m_historyAttachmentImage[ImageFrameCount];
            uint32_t m_currentHistoryAttachmentImage = 0;

            JE::Render::RPI::PassAttachmentBinding* m_historyAttachmentBinding = nullptr;
            JE::Render::RPI::PassAttachmentBinding* m_reflectionInputAttachmentBinding = nullptr;

            RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            RHI::ShaderInputNameIndex m_outputWidthNameIndex = "m_outputWidth";
            RHI::ShaderInputNameIndex m_outputHeightNameIndex = "m_outputHeight";
            RHI::ShaderInputNameIndex m_mipLevelsNameIndex = "m_mipLevels";
            RHI::ShaderInputNameIndex m_coneTracingNameIndex = "m_coneTracing";
            RHI::ShaderInputNameIndex m_rayTracingNameIndex = "m_rayTracing";
            RHI::ShaderInputNameIndex m_temporalFilteringNameIndex = "m_temporalFiltering";
            RHI::ShaderInputNameIndex m_invTemporalFilteringStrengthNameIndex = "m_invTemporalFilteringStrength";
            RHI::ShaderInputNameIndex m_maxRoughnessNameIndex = "m_maxRoughness";
            RHI::ShaderInputNameIndex m_roughnessBiasNameIndex = "m_roughnessBias";
            RHI::ShaderInputNameIndex m_luminanceClampNameIndex = "m_luminanceClamp";
            RHI::ShaderInputNameIndex m_maxLuminanceNameIndex = "m_maxLuminance";
        };
    }   // namespace RPI
}   // namespace AZ
