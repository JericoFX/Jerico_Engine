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

#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurSettingsInterface.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        class PostProcessSettings;

        class MotionBlurSettings final
            : public MotionBlurSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI_WITH_NAME(
                MotionBlurSettings, "AZ::Render::MotionBlurSettings",
                "{262E7D7E-4B2D-4022-A67D-8EF4D07FB147}",
                JE::Render::Features::MotionBlurSettingsInterface,
                JE::Render::Features::PostProcessBase);
            AZ_CLASS_ALLOCATOR(MotionBlurSettings, AZ::SystemAllocator, 0);

            MotionBlurSettings(PostProcessFeatureProcessor* featureProcessor);
            ~MotionBlurSettings() = default;

            // BloomSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(MotionBlurSettings* target, float alpha) const;

            // Generate getters and setters.
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:
            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>


            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    } // namespace Features
} // namespace JE::Render
