/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/MotionBlur/MotionBlurSettings.h>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurConstants.h>

#include <PostProcess/PostProcessFeatureProcessor.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/View.h>

namespace JE::Render
{
    namespace Features
    {
        MotionBlurSettings::MotionBlurSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        void MotionBlurSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void MotionBlurSettings::ApplySettingsTo(MotionBlurSettings* target, float alpha) const
        {
            AZ_Assert(target != nullptr, "MotionBlurSettings::ApplySettingsTo called with nullptr as argument.");

            // Auto-gen code to blend individual params based on their override value onto target settings
#define OVERRIDE_TARGET target
#define OVERRIDE_ALPHA alpha
#include <JE/Render/Features/ParamMacros/StartOverrideBlend.inl>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef OVERRIDE_TARGET
#undef OVERRIDE_ALPHA
        }

        void MotionBlurSettings::Simulate(float deltaTime)
        {
            m_deltaTime = deltaTime;
        }

    } // namespace Features
} // namespace JE::Render
