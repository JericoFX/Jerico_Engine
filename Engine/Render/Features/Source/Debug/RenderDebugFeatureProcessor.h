/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Debug/RenderDebugFeatureProcessorInterface.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <AzCore/Instance/Instance.h>
#include <Debug/RenderDebugSettings.h>

namespace JE::Render::Features
{
    class RenderDebugFeatureProcessor final
        : public JE::Render::Features::RenderDebugFeatureProcessorInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(RenderDebugFeatureProcessor, AZ::SystemAllocator)
        AZ_RTTI_WITH_NAME(JE::Render::Features::RenderDebugFeatureProcessor, "AZ::Render::RenderDebugFeatureProcessor", "{1F14912D-43E1-4992-9822-BE8967E59EA3}", JE::Render::Features::RenderDebugFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        RenderDebugFeatureProcessor();
        virtual ~RenderDebugFeatureProcessor() = default;

        //! RenderDebugFeatureProcessorInterface overrides...
        RenderDebugSettingsInterface* GetSettingsInterface() override;
        void OnRenderDebugComponentAdded() override;
        void OnRenderDebugComponentRemoved() override;

        // FeatureProcessor overrides ...
        void Activate() override;
        void Deactivate() override;
        void Simulate(const JE::Render::RPI::FeatureProcessor::SimulatePacket& packet) override;
        void Render(const JE::Render::RPI::FeatureProcessor::RenderPacket& packet) override;

    private:
        static constexpr const char* FeatureProcessorName = "RenderDebugFeatureProcessor";
        RenderDebugFeatureProcessor(const RenderDebugFeatureProcessor&) = delete;

        AZStd::unique_ptr<RenderDebugSettings> m_settings = nullptr;

        // Scene and View SRG
        AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_sceneSrg = nullptr;
        AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_viewSrg = nullptr;

        // View SRG members
        AZ::RHI::ShaderInputNameIndex m_renderDebugOptionsIndex = "m_renderDebugOptions";
        AZ::RHI::ShaderInputNameIndex m_renderDebugViewModeIndex = "m_renderDebugViewMode";

        // Scene SRG members
        AZ::RHI::ShaderInputNameIndex m_debuggingEnabledIndex = "m_debuggingEnabled";
        AZ::RHI::ShaderInputNameIndex m_debugOverrideBaseColorIndex = "m_debugOverrideBaseColor";
        AZ::RHI::ShaderInputNameIndex m_debugOverrideRoughnessIndex = "m_debugOverrideRoughness";
        AZ::RHI::ShaderInputNameIndex m_debugOverrideMetallicIndex = "m_debugOverrideMetallic";
        AZ::RHI::ShaderInputNameIndex m_debugLightingDirectionIndex = "m_debugLightingDirection";
        AZ::RHI::ShaderInputNameIndex m_debugLightingIntensityIndex = "m_debugLightingIntensity";

        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex01 = "m_customDebugFloat01";
        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex02 = "m_customDebugFloat02";
        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex03 = "m_customDebugFloat03";
        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex04 = "m_customDebugFloat04";
        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex05 = "m_customDebugFloat05";
        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex06 = "m_customDebugFloat06";
        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex07 = "m_customDebugFloat07";
        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex08 = "m_customDebugFloat08";
        AZ::RHI::ShaderInputNameIndex m_customDebugFloatIndex09 = "m_customDebugFloat09";

        AZ::Name m_shaderDebugEnableOptionName = AZ::Name("o_shader_debugging_enabled");

        AZ::u32 m_debugComponentCount = 0;
    };

}
