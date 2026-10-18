/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace JE::Render
{
    namespace Features
    {
        //! Pass used to enabled/disabled debug colors by shader option.
        class DepthOfFieldCompositePass final
            : public JE::Render::RPI::FullscreenTrianglePass
            , public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(DepthOfFieldCompositePass);

        public:
            AZ_RTTI_WITH_NAME(DepthOfFieldCompositePass, "AZ::Render::DepthOfFieldCompositePass", "{7595A972-7ED5-46FE-BBE0-3262846E2964}", JE::Render::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldCompositePass, AZ::SystemAllocator);
            virtual ~DepthOfFieldCompositePass() = default;

            //! Creates a DepthOfFieldCompositePass
            static JE::Render::RPI::Ptr<DepthOfFieldCompositePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetEnabledDebugColoring(bool enabled);

        protected:
            // Pass behavior overrides...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            DepthOfFieldCompositePass(const JE::Render::RPI::PassDescriptor& descriptor);
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            // Scope producer functions...
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            bool m_needToUpdateShaderVariant = true;

            bool m_enabledDebugColoring = false;

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_backBlendFactorDivision2Index = "m_backBlendFactorDivision2";
            AZ::RHI::ShaderInputNameIndex m_backBlendFactorDivision4Index = "m_backBlendFactorDivision4";
            AZ::RHI::ShaderInputNameIndex m_backBlendFactorDivision8Index = "m_backBlendFactorDivision8";
            AZ::RHI::ShaderInputNameIndex m_frontBlendFactorDivision2Index = "m_frontBlendFactorDivision2";
            AZ::RHI::ShaderInputNameIndex m_frontBlendFactorDivision4Index = "m_frontBlendFactorDivision4";
            AZ::RHI::ShaderInputNameIndex m_frontBlendFactorDivision8Index = "m_frontBlendFactorDivision8";

            // scale / offset to convert DofFactor to blend ratio for back buffer.
            AZStd::array<float, 2> m_backBlendFactorDivision2 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_backBlendFactorDivision4 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_backBlendFactorDivision8 = { { 0.0f, 0.0f } };
            // scale / offset to convert DofFactor to blend ratio for front buffer.
            AZStd::array<float, 2> m_frontBlendFactorDivision2 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_frontBlendFactorDivision4 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_frontBlendFactorDivision8 = { { 0.0f, 0.0f } };

            const AZ::Name m_optionName;
            const AZStd::vector<AZ::Name> m_optionValues;
        };
    } // namespace Features
} // namespace JE::Render
