/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <ACES/Aces.h>
#include <JE/Render/Features/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <DisplayMapper/ApplyShaperLookupTablePass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace JE::Render
{
    namespace Features
    {
        static const char* const ToneMapperShaderVariantOptionName{ "o_tonemapperType" };
        static const char* const TransferFunctionShaderVariantOptionName{ "o_transferFunctionType" };
        static const char* const LdrGradingLutShaderVariantOptionName{ "o_colorGradingEnabled" };

        /**
         * This pass is used to apply tonemapping and output transforms other than ACES.
         */
        class OutputTransformPass
            : public ApplyShaperLookupTablePass
            , public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(OutputTransformPass);

        public:
            AZ_RTTI(OutputTransformPass, "{1703EB2E-2415-41AE-9C10-06151F795A4A}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(OutputTransformPass, AZ::SystemAllocator);
            virtual ~OutputTransformPass();

            /// Creates a OutputTransformPass
            static JE::Render::RPI::Ptr<OutputTransformPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetDisplayBufferFormat(AZ::RHI::Format format);

            void SetToneMapperType(const AZ::Render::ToneMapperType& toneMapperType);
            void SetTransferFunctionType(const AZ::Render::TransferFunctionType& transferFunctionType);

        protected:
            explicit OutputTransformPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides
            void InitializeInternal() override;

        private:
            // Scope producer functions...
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // Pass behavior overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            const AZ::Name m_toneMapperShaderVariantOptionName;
            const AZ::Name m_transferFunctionShaderVariantOptionName;
            const AZ::Name m_ldrGradingLutShaderVariantOptionName;

            AZ::Render::ToneMapperType m_toneMapperType = AZ::Render::ToneMapperType::None;
            AZ::Render::TransferFunctionType m_transferFunctionType = AZ::Render::TransferFunctionType::None;

            bool m_needToUpdateShaderVariant = false;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputCinemaLimitsIndex;
            AZ::RHI::ShaderInputNameIndex m_exposureControlBufferInputIndex = "m_exposureControl";

            JE::Render::Features::DisplayMapperParameters m_displayMapperParameters = {};

            AZ::RHI::Format m_displayBufferFormat = AZ::RHI::Format::Unknown;
        };
    } // namespace Features
} // namespace JE::Render
