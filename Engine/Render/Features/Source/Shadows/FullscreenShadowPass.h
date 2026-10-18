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
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/Features/CoreLights/ShadowConstants.h>

namespace JE::Render
{
    namespace Features
    {
        class FullscreenShadowPass final
            : public JE::Render::RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(FullscreenShadowPass);

            using Base = JE::Render::RPI::FullscreenTrianglePass;

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::FullscreenShadowPass, "AZ::Render::FullscreenShadowPass", "{A7D3076A-DD01-4B79-AF34-4BB72DAD35E2}", JE::Render::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(FullscreenShadowPass, AZ::SystemAllocator);
            virtual ~FullscreenShadowPass() = default;

            static JE::Render::RPI::Ptr<FullscreenShadowPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);
            void SetBlendBetweenCascadesEnable(bool enable)
            {
                m_blendBetweenCascadesEnable = enable;
            }

            void SetFilterMethod(JE::Render::Features::ShadowFilterMethod method)
            {
                m_filterMethod = method;
            }

            void SetFilteringSampleCountMode(JE::Render::Features::ShadowFilterSampleCount filteringSampleCount)
            {
                m_filteringSampleCountMode = filteringSampleCount;
            }

            void SetReceiverShadowPlaneBiasEnable(bool enable)
            {
                m_receiverShadowPlaneBiasEnable = enable;
            }

            // Set directional light's raw index which is used for accessing the directional light in the shader
            void SetLightRawIndex(int lightRawIndex)
            {
                m_lightIndex = lightRawIndex;
            }

        private:

            bool m_blendBetweenCascadesEnable = false;
            bool m_receiverShadowPlaneBiasEnable = false;
            ShadowFilterMethod m_filterMethod = ShadowFilterMethod::None;
            ShadowFilterSampleCount m_filteringSampleCountMode = ShadowFilterSampleCount::PcfTap16;

            FullscreenShadowPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // Scope producer functions...
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            AZ::RHI::Size GetDepthBufferDimensions();
            uint16_t GetDepthBufferMSAACount();

            void SetConstantData();

            AZ::RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";

            int m_lightIndex = 0;

            AZ::Name m_depthInputName;
            AZ::Name m_outputName;
        };
    } // namespace Features
} // namespace JE::Render
