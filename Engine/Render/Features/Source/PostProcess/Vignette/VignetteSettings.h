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

#include <JE/Render/Features/PostProcess/Vignette/VignetteSettingsInterface.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        class PostProcessSettings;

        class VignetteSettings final
            : public VignetteSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI_WITH_NAME(
                VignetteSettings, "AZ::Render::VignetteSettings",
                "{5FE434E6-70BF-4064-AAA9-D63225A9E29C}",
                JE::Render::Features::VignetteSettingsInterface,
                JE::Render::Features::PostProcessBase);
            AZ_CLASS_ALLOCATOR(VignetteSettings, AZ::SystemAllocator, 0);

            VignetteSettings(PostProcessFeatureProcessor* featureProcessor);
            ~VignetteSettings() = default;

            // BloomSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(VignetteSettings* target, float alpha) const;

            // Generate getters and setters.
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <JE/Render/Features/PostProcess/Vignette/VignetteParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:
            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/Vignette/VignetteParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>


            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    } // namespace Features
} // namespace JE::Render
