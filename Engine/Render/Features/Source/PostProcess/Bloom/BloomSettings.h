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

#include <JE/Render/Features/PostProcess/Bloom/BloomSettingsInterface.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        class PostProcessSettings;

        // The post process sub-settings class for Bloom
        class BloomSettings final
            : public BloomSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI_WITH_NAME(BloomSettings, "AZ::Render::BloomSettings", "{9CDC625A-0545-494E-AB37-552A19741F6A}",
                JE::Render::Features::BloomSettingsInterface, JE::Render::Features::PostProcessBase);
            AZ_CLASS_ALLOCATOR(BloomSettings, AZ::SystemAllocator);

            BloomSettings(PostProcessFeatureProcessor* featureProcessor);
            ~BloomSettings() = default;

            // BloomSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(BloomSettings* target, float alpha) const;

            // Generate getters and setters.
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <JE/Render/Features/PostProcess/Bloom/BloomParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/Bloom/BloomParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    }
}
