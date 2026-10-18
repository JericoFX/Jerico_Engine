/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/PostProcess/DepthOfField/DepthOfFieldComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace JE::Render
{
    namespace Scene
    {
        void DepthOfFieldComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DepthOfFieldComponentConfig, AZ::ComponentConfig>()
                    ->Version(0)

                    // Auto-gen serialize context code...
#define SERIALIZE_CLASS DepthOfFieldComponentConfig
#include <JE/Render/Features/ParamMacros/StartParamSerializeContext.inl>
#include <JE/Render/Features/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                    ;
            }
        }

        void DepthOfFieldComponentConfig::CopySettingsFrom(JE::Render::Features::DepthOfFieldSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_SOURCE settings
#include <JE/Render/Features/ParamMacros/StartParamCopySettingsFrom.inl>
#include <JE/Render/Features/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef COPY_SOURCE
        }

        void DepthOfFieldComponentConfig::CopySettingsTo(JE::Render::Features::DepthOfFieldSettingsInterface* settings)
        {
            if (!settings)
            {
                return;
            }

#define COPY_TARGET settings
#include <JE/Render/Features/ParamMacros/StartParamCopySettingsTo.inl>
#include <JE/Render/Features/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef COPY_TARGET
        }

    } // namespace Render
} // namespace AZ
