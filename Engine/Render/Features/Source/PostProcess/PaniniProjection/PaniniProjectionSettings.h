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

#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionSettingsInterface.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        class PostProcessSettings;

        class PaniniProjectionSettings final
            : public PaniniProjectionSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI_WITH_NAME(
                PaniniProjectionSettings, "AZ::Render::PaniniProjectionSettings",
                "{30D32346-39CE-49DF-9EEC-FEEC2090A45A}",
                JE::Render::Features::PaniniProjectionSettingsInterface,
                JE::Render::Features::PostProcessBase);
            AZ_CLASS_ALLOCATOR(PaniniProjectionSettings, AZ::SystemAllocator, 0);

            PaniniProjectionSettings(PostProcessFeatureProcessor* featureProcessor);
            ~PaniniProjectionSettings() = default;

            // BloomSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(PaniniProjectionSettings* target, float alpha) const;

            // Generate getters and setters.
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:
            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>


            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    } // namespace Features
} // namespace JE::Render
