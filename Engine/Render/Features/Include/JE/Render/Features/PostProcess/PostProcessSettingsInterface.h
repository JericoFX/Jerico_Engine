/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <JE/Render/Features/PostProcess/Bloom/BloomSettingsInterface.h>
#include <JE/Render/Features/PostProcess/ChromaticAberration/ChromaticAberrationSettingsInterface.h>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurSettingsInterface.h>
#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionSettingsInterface.h>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainSettingsInterface.h>
#include <JE/Render/Features/PostProcess/WhiteBalance/WhiteBalanceSettingsInterface.h>
#include <JE/Render/Features/PostProcess/Vignette/VignetteSettingsInterface.h>
#include <JE/Render/Features/PostProcess/DepthOfField/DepthOfFieldSettingsInterface.h>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlSettingsInterface.h>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoSettingsInterface.h>
#include <JE/Render/Features/PostProcess/LookModification/LookModificationSettingsInterface.h>
#include <JE/Render/Features/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h>
#include <JE/Render/Features/ScreenSpace/DeferredFogSettingsInterface.h>
#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        //! Abstract interface for PostProcessSettings so it can be access outside of Atom (for example in AtomLyIntegration)
        class PostProcessSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::PostProcessSettingsInterface, "AZ::Render::PostProcessSettingsInterface", "{22949C39-B56B-4920-94FA-6FDFA8273A2C}");

            using ViewBlendWeightMap = AZStd::unordered_map<JE::Render::RPI::View*, float>;

            static constexpr float defaultBlendWeight = 1.0f;

            //! To be called when config settings have been changed
            virtual void OnConfigChanged() = 0;

            //! Function to import blend weight settings
            virtual void CopyViewToBlendWeightSettings(const ViewBlendWeightMap& perCameraBlendWeights) = 0;

            //! Setter for PostFx setting's layer represented by an integer
            virtual void SetLayerCategoryValue(int layerCategoryValue) = 0;

            // Auto generate virtual GetOrCreate and Remove
#define POST_PROCESS_MEMBER(ClassName, MemberName)                                                      \
            virtual ClassName##Interface* GetOrCreate##ClassName##Interface() = 0;                      \
            virtual void Remove##ClassName##Interface() = 0;                                            \

#include <JE/Render/Features/PostProcess/PostProcessSettings.inl>
#undef POST_PROCESS_MEMBER

            // Auto generate virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/PostProcessParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
        };

    }
}
