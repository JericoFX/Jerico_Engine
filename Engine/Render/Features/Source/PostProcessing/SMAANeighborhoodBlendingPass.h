/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/Features/PostProcessing/SMAAFeatureProcessorInterface.h>
#include <PostProcessing/SMAABasePass.h>

namespace JE::Render
{
    namespace Features
    {
        static const char* const SMAANeighborhoodBlendingPassTemplateName = "SMAANeighborhoodBlendingTemplate";

        //! The SMAA neighborhood blending pass implementation. The third pass of the SMAA applies anti-aliasing by blending with neighborhood pixels
        //! using the blending weight texture output from previous pass.
        class SMAANeighborhoodBlendingPass final
            : public SMAABasePass
        {
        public:
            AZ_RTTI(SMAANeighborhoodBlendingPass, "{EED89560-137F-4666-8E43-FF8A004F82A5}", SMAABasePass);
            AZ_CLASS_ALLOCATOR(SMAANeighborhoodBlendingPass, AZ::SystemAllocator);
            virtual ~SMAANeighborhoodBlendingPass() = default;

            //! Creates a SMAANeighborhoodBlendingPass
            static JE::Render::RPI::Ptr<SMAANeighborhoodBlendingPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            void SetOutputMode(SMAAOutputMode mode);

        private:
            SMAANeighborhoodBlendingPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides
            void InitializeInternal() override;

            // SMAABasePass functions...
            void UpdateSRG() override;
            void GetCurrentShaderOption(JE::Render::RPI::ShaderOptionGroup& shaderOption) const override;

            AZ::RHI::ShaderInputNameIndex m_renderTargetMetricsShaderInputIndex = "m_renderTargetMetrics";

            const AZ::Name m_blendingOutputModeOptionName;

            // [GFX TODO][ATOM-3977] Since these parameters don't have control method, they are fixed at the moment. They will be controlled by feature processor in the future.
            SMAAOutputMode m_outputMode = SMAAOutputMode::BlendResultWithProvisionalTonemap;
        };
    } // namespace Features
} // namespace JE::Render
