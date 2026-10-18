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
#include <DisplayMapper/DisplayMapperFullScreenPass.h>

namespace JE::Render
{
    namespace Features
    {
        /**
         * This pass is used to apply a shaper function and lookup table to the input image attachment.
         * The coordinates on the lookup table are computed by taking the color values of the input image
         * and translating it via a shaper function.
         * Right now, preset shaper functions based of ACES 1.0.3 are being used.
         */
        class ApplyShaperLookupTablePass
            : public DisplayMapperFullScreenPass
        {
            AZ_RPI_PASS(AcesOutputTransformLutPass);

        public:
            AZ_RTTI(ApplyShaperLookupTablePass, "{5C76BE12-307A-4595-91CE-AAA13ED6368C}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(ApplyShaperLookupTablePass, AZ::SystemAllocator);
            virtual ~ApplyShaperLookupTablePass();

            /// Creates a AcesOutputTransformLutPass
            static JE::Render::RPI::Ptr<ApplyShaperLookupTablePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetShaperParameters(const AZ::Render::ShaperParams& shaperParams);
            void SetLutAssetId(const AZ::Data::AssetId& assetId);
            const AZ::Data::AssetId& GetLutAssetId() const;

        protected:
            explicit ApplyShaperLookupTablePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            AZ::RHI::ShaderInputImageIndex m_shaderInputLutImageIndex;

            AZ::RHI::ShaderInputConstantIndex m_shaderShaperTypeIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderShaperBiasIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderShaperScaleIndex;

            void SetupFrameGraphDependenciesCommon(AZ::RHI::FrameGraphInterface frameGraph);
            void CompileResourcesCommon(const AZ::RHI::FrameGraphCompileContext& context);

        private:

            // Scope producer functions...
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            void UpdateShaperSrg();
            void AcquireLutImage();
            void ReleaseLutImage();

            DisplayMapperAssetLut       m_lutResource;
            AZ::Data::AssetId           m_lutAssetId;

            AZ::Render::ShaperParams m_shaperParams;

            bool m_needToReloadLut = true;
        };
    } // namespace Features
} // namespace JE::Render
