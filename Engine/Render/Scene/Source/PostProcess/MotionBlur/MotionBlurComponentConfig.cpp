/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/PostProcess/MotionBlur/MotionBlurComponentConfig.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace JE::Render
{
    namespace Scene
    {
        void MotionBlurComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MotionBlurComponentConfig, AZ::ComponentConfig>()->Version(0)

                // Auto-gen serialize context code...
#define SERIALIZE_CLASS MotionBlurComponentConfig
#include <Atom/Feature/ParamMacros/StartParamSerializeContext.inl>
#include <Atom/Feature/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                    ;
            }
        }

        void MotionBlurComponentConfig::CopySettingsFrom(AZ::Render::MotionBlurSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_SOURCE settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsFrom.inl>
#include <Atom/Feature/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_SOURCE
        }

        void MotionBlurComponentConfig::CopySettingsTo(AZ::Render::MotionBlurSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_TARGET settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsTo.inl>
#include <Atom/Feature/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_TARGET
        }

    } // namespace Render
} // namespace AZ
