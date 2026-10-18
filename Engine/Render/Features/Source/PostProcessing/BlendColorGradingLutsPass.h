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

#include <PostProcess/LookModification/LookModificationSettings.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace JE::Render
{
    namespace Features
    {

        //! This pass will blend together multiple color blending LUTs based on their individual intensity setting as well as their
        //! override strengths.
        //! If there is only one LUT, blending will happen with the LUT and the reference non color graded values based on that LUT's intensity value.
        class BlendColorGradingLutsPass final
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(BlendColorGradingLutsPass);

        public:
            AZ_RTTI(BlendColorGradingLutsPass, "{F1E7ED65-27B1-4AF3-AF8D-C29C2BF31EE7}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(BlendColorGradingLutsPass, AZ::SystemAllocator);
            virtual ~BlendColorGradingLutsPass();

            /// Creates a DisplayMapperPass
            static JE::Render::RPI::Ptr<BlendColorGradingLutsPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetShaperParameters(const AZ::Render::ShaperParams& shaperParams);
            AZStd::optional<AZ::Render::ShaperParams> GetCommonShaperParams() const;

        private:
            explicit BlendColorGradingLutsPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            void AcquireLutImage();
            void ReleaseLutImage();

            void CheckLutBlendSettings();
            LookModificationSettings* GetLookModificationSettings() const;

            bool m_resourcesInitialized = false;

            const AZ::Name m_numSourceLutsShaderVariantOptionName;
            bool m_needToUpdateShaderVariant = false;

            AZ::RHI::ShaderInputNameIndex m_shaderInputBlendedLutImageIndex = "m_blendedLut";
            AZ::RHI::ShaderInputNameIndex m_shaderInputBlendedLutDimensionsIndex = "m_blendedLutDimensions";
            AZ::RHI::ShaderInputNameIndex m_shaderInputBlendedLutShaperTypeIndex = "m_blendedLutShaperType";
            AZ::RHI::ShaderInputNameIndex m_shaderInputBlendededLutShaperBiasIndex = "m_blendedLutShaperBias";
            AZ::RHI::ShaderInputNameIndex m_shaderInputBlendededLutShaperScaleIndex = "m_blendedLutShaperScale";

            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut1ImageIndex = "m_sourceLut1";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut1ShaperTypeIndex = "m_sourceLut1ShaperType";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut1ShaperBiasIndex = "m_sourceLut1ShaperBias";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut1ShaperScaleIndex = "m_sourceLut1ShaperScale";

            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut2ImageIndex = "m_sourceLut2";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut2ShaperTypeIndex = "m_sourceLut2ShaperType";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut2ShaperBiasIndex = "m_sourceLut2ShaperBias";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut2ShaperScaleIndex = "m_sourceLut2ShaperScale";

            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut3ImageIndex = "m_sourceLut3";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut3ShaperTypeIndex = "m_sourceLut3ShaperType";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut3ShaperBiasIndex = "m_sourceLut3ShaperBias";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut3ShaperScaleIndex = "m_sourceLut3ShaperScale";

            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut4ImageIndex = "m_sourceLut4";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut4ShaperTypeIndex = "m_sourceLut4ShaperType";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut4ShaperBiasIndex = "m_sourceLut4ShaperBias";
            AZ::RHI::ShaderInputNameIndex m_shaderInputSourceLut4ShaperScaleIndex = "m_sourceLut4ShaperScale";

            AZ::RHI::ShaderInputNameIndex m_shaderInputWeight0Index = "m_weight0";
            AZ::RHI::ShaderInputNameIndex m_shaderInputWeight1Index = "m_weight1";
            AZ::RHI::ShaderInputNameIndex m_shaderInputWeight2Index = "m_weight2";
            AZ::RHI::ShaderInputNameIndex m_shaderInputWeight3Index = "m_weight3";
            AZ::RHI::ShaderInputNameIndex m_shaderInputWeight4Index = "m_weight4";

            JE::Render::Features::DisplayMapperParameters m_displayMapperParameters = {};

            DisplayMapperLut m_blendedLut = {};
            AZ::Render::ShaperParams m_blendedLutShaperParams;
            AZStd::array<AZ::u32, 3> m_blendedLutDimensions;

            float m_weights[LookModificationSettings::MaxBlendLuts + 1]; // The first index is reserved for the weight of the non color graded value
            AZ::Render::ShaperParams m_colorGradingShaperParams[LookModificationSettings::MaxBlendLuts];
            JE::Render::Features::DisplayMapperAssetLut m_colorGradingLuts[LookModificationSettings::MaxBlendLuts];

            bool m_needToUpdateLut = false;
            uint32_t m_numSourceLuts = LookModificationSettings::MaxBlendLuts + 1; // Initalize to an invalid value
            AZ::HashValue64 m_lutBlendHash = AZ::HashValue64{ 0 };

            struct ShaderVariantInfo
            {
                const bool m_isFullyBaked = false;
                const AZ::RHI::PipelineState* m_pipelineState = nullptr;
            };

            AZStd::vector<ShaderVariantInfo> m_shaderVariant;
            JE::Render::RPI::ShaderVariantKey m_currentShaderVariantKeyFallbackValue;
            uint32_t m_currentShaderVariantIndex = 0;
        };
    } // namespace Features
} // namespace JE::Render
