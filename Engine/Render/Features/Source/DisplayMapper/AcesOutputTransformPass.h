/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

#include <JE/Render/Features/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <DisplayMapper/DisplayMapperFullScreenPass.h>

namespace JE::Render
{
    namespace Features
    {
        /**
         *  The Aces output transform pass.
         *  This pass impelements the ACES color pipeline, which includes Reference Rendering Transform(RRT)
         *  and Output Device Transform(ODT).
         */
        class AcesOutputTransformPass final
            : public DisplayMapperFullScreenPass
        {
            AZ_RPI_PASS(AcesOutputTransformPass);

        public:
            AZ_RTTI(AcesOutputTransformPass, "{705F8A80-CAF2-4A9C-BF40-2141ABD70BDC}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(AcesOutputTransformPass, AZ::SystemAllocator);

            //! Creates a DisplayMapperPass
            static JE::Render::RPI::Ptr<AcesOutputTransformPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetDisplayBufferFormat(AZ::RHI::Format format);
            void SetAcesParameterOverrides(const AcesParameterOverrides& acesParameterOverrides);

        private:
            explicit AcesOutputTransformPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides
            void InitializeInternal() override;

            // Scope producer functions...
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            AZ::RHI::ShaderInputConstantIndex m_shaderInputColorMatIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputCinemaLimitsIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputAcesSplineParamsIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputFlagsIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputOutputModeIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputSurroundGammaIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputGammaIndex;

            JE::Render::Features::DisplayMapperParameters m_displayMapperParameters = {};

            AZ::RHI::Format m_displayBufferFormat = AZ::RHI::Format::Unknown;

            AcesParameterOverrides m_acesParameterOverrides;
        };
    } // namespace Features
} // namespace JE::Render
