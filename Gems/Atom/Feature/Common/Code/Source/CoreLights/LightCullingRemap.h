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
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Reflect/Pass/PassName.h>
#include <JE/Render/RPI/Public/Image/AttachmentImage.h>

namespace AZ
{
    namespace Render
    {
        //! Compute shader that takes the results of the LightCulling shader and bins the results.
        //! This allows the forward shader to quickly look up all the lights that affect it by isolating the exact bin with the indices
        //! then walk through them in linear order
        //! Also we are converting from R32 to R16 so it is more read efficient
        class LightCullingRemap final
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(LightCullingRemap);

        public:
            AZ_RTTI(AZ::Render::LightCullingRemap, "{0ED3D558-F7B3-4D3F-B9E6-3A3B8EC30E91}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(LightCullingRemap, SystemAllocator);
            virtual ~LightCullingRemap() = default;

            /// Creates a LightCullingRemap
            static JE::Render::RPI::Ptr<LightCullingRemap> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            static Name GetLightCullingRemapTemplateName()
            {
                return Name("LightCullingRemapTemplate");
            }

        private:

            LightCullingRemap(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void ResetInternal()override;
            void BuildInternal() override;

            // RHI::ScopeProducer overrides...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            void CreateRemappedLightListBuffer();
            void Init();
            AZ::RHI::Size GetTileDataBufferResolution();
            uint32_t FindInputOutputBinding(const AZ::Name& name);

            AZ::RHI::ShaderInputConstantIndex m_tileWidthIndex;
            Data::Instance<JE::Render::RPI::Buffer> m_lightListRemapped;
            AZ::RHI::Size m_tileDim;
            int m_tileDataIndex = -1;
            bool m_initialized = false;
        };
    }   // namespace Render
}   // namespace AZ
