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

#include <Atom/RHI/ImagePool.h>
#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

#include <JE/Render/Features/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace JE::Render
{
    namespace Features
    {

        /**
         *  The bake ACES output transofrm LUT pass.
         *  This pass the RRT and ODT components of the ACES pipeline into a LUT.
         */
        class BakeAcesOutputTransformLutPass final
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(BakeAcesOutputTransformLutPass);

        public:
            AZ_RTTI(BakeAcesOutputTransformLutPass, "{383C28CD-D744-4B48-A30D-086EF66E7BFB}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(BakeAcesOutputTransformLutPass, AZ::SystemAllocator);
            virtual ~BakeAcesOutputTransformLutPass();

            /// Creates a DisplayMapperPass
            static JE::Render::RPI::Ptr<BakeAcesOutputTransformLutPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetDisplayBufferFormat(AZ::RHI::Format format);

            const AZ::Render::ShaperParams& GetShaperParams() const;

        private:
            explicit BakeAcesOutputTransformLutPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // RHI::ScopeProducer overrides...
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            void AcquireLutImage();
            void ReleaseLutImage();

            bool m_resourcesInitialized = false;

            AZ::RHI::ShaderInputConstantIndex m_shaderInputColorMatIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputCinemaLimitsIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputAcesSplineParamsIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputFlagsIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputOutputModeIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputSurroundGammaIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputGammaIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputShaperBiasIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputShaperScaleIndex;

            AZ::RHI::ShaderInputImageIndex m_shaderInputLutImageIndex;

            JE::Render::Features::DisplayMapperParameters m_displayMapperParameters = {};

            DisplayMapperLut m_displayMapperLut = {};

            bool m_needToUpdateLut = false;
            AZ::RHI::Format m_displayBufferFormat = AZ::RHI::Format::Unknown;
            AZ::Render::OutputDeviceTransformType m_outputDeviceTransformType = AZ::Render::OutputDeviceTransformType_48Nits;
            AZ::Render::ShaperParams m_shaperParams;
        };
    } // namespace Features
} // namespace JE::Render
