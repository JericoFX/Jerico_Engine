/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoSettingsInterface.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        class PostProcessSettings;

        // The post process sub-settings class for all types of AO (SSAO and GTAO)
        class AoSettings final
            : public AoSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI_WITH_NAME(AoSettings, "AZ::Render::AoSettings", "{6CFCBD33-7419-4BFC-A7E8-30D29373EE29}",
                JE::Render::Features::AoSettingsInterface, JE::Render::Features::PostProcessBase);
            AZ_CLASS_ALLOCATOR(AoSettings, AZ::SystemAllocator);

            AoSettings(PostProcessFeatureProcessor* featureProcessor);
            ~AoSettings() = default;

            // SsaoSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(AoSettings* target, float alpha) const;

            // Generate getters and setters.
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    }
}
