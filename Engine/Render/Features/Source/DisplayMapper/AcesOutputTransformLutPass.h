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
         *  The ACES output transform LUT pass.
         *  This pass implements the RRT and ODT components of the ACES pipeline from a baked LUT.
         */
        class AcesOutputTransformLutPass final
            : public DisplayMapperFullScreenPass
        {
            AZ_RPI_PASS(AcesOutputTransformLutPass);

        public:
            AZ_RTTI(AcesOutputTransformLutPass, "{914EE97F-20DA-4916-AE66-DC4141E1A06E}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(AcesOutputTransformLutPass, AZ::SystemAllocator);
            virtual ~AcesOutputTransformLutPass();

            /// Creates a AcesOutputTransformLutPass
            static JE::Render::RPI::Ptr<AcesOutputTransformLutPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetShaperParams(const AZ::Render::ShaperParams& shaperParams);
        private:
            explicit AcesOutputTransformLutPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            void AcquireLutImage();
            void ReleaseLutImage();

            AZ::RHI::ShaderInputImageIndex m_shaderInputLutImageIndex;
            AZ::RHI::ShaderInputImageIndex m_shaderInputColorImageIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputShaperBiasIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputShaperScaleIndex;

            DisplayMapperLut m_displayMapperLut = {};
            AZ::Render::ShaperParams m_shaperParams;
        };
    } // namespace Features
} // namespace JE::Render
