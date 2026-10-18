/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/Vignette/VignetteSettings.h>
#include <JE/Render/Features/PostProcess/Vignette/VignetteConstants.h>

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
        VignetteSettings::VignetteSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        void VignetteSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void VignetteSettings::ApplySettingsTo(VignetteSettings* target, float alpha) const
        {
            AZ_Assert(target != nullptr, "VignetteSettings::ApplySettingsTo called with nullptr as argument.");

            // Auto-gen code to blend individual params based on their override value onto target settings
#define OVERRIDE_TARGET target
#define OVERRIDE_ALPHA alpha
#include <JE/Render/Features/ParamMacros/StartOverrideBlend.inl>
#include <JE/Render/Features/PostProcess/Vignette/VignetteParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef OVERRIDE_TARGET
#undef OVERRIDE_ALPHA
        }

        void VignetteSettings::Simulate(float deltaTime)
        {
            m_deltaTime = deltaTime;
        }

    } // namespace Features
} // namespace JE::Render
