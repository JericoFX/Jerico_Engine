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

#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainSettingsInterface.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        class PostProcessSettings;

        class FilmGrainSettings final
            : public FilmGrainSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI_WITH_NAME(
                FilmGrainSettings, "AZ::Render::FilmGrainSettings",
                "{E75BF1A1-2607-4743-A755-8CE2D59E291E}",
                JE::Render::Features::FilmGrainSettingsInterface,
                JE::Render::Features::PostProcessBase);
            AZ_CLASS_ALLOCATOR(FilmGrainSettings, AZ::SystemAllocator, 0);

            FilmGrainSettings(PostProcessFeatureProcessor* featureProcessor);
            ~FilmGrainSettings() = default;

            // BloomSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(FilmGrainSettings* target, float alpha) const;

            AZ::Data::Instance<JE::Render::RPI::StreamingImage> LoadStreamingImage(const char* textureFilePath, const char* sampleName);

            // Generate getters and setters.
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:
            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>


            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    } // namespace Features
} // namespace JE::Render
