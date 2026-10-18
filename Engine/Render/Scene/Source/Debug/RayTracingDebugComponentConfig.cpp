/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/Debug/RayTracingDebugComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace JE::Render::Scene
{
    void RayTracingDebugComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext{ azrtti_cast<AZ::SerializeContext*>(context) })
        {
            // clang-format off
            serializeContext->Class<RayTracingDebugComponentConfig, AZ::ComponentConfig>()
                ->Version(0)
                // Auto-gen serialize context code
                #define SERIALIZE_CLASS RayTracingDebugComponentConfig
                #include <JE/Render/Features/ParamMacros/StartParamSerializeContext.inl>
                #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
                #include <JE/Render/Features/ParamMacros/EndParams.inl>
                #undef SERIALIZE_CLASS
            ;
            // clang-format on
        }
    }

    void RayTracingDebugComponentConfig::CopySettingsFrom(JE::Render::Features::RayTracingDebugSettingsInterface* settings)
    {
        if (!settings)
        {
            return;
        }

        // clang-format off
        #define COPY_SOURCE settings
        #include <JE/Render/Features/ParamMacros/StartParamCopySettingsFrom.inl>
        #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
        #include <JE/Render/Features/ParamMacros/EndParams.inl>
        #undef COPY_SOURCE
        // clang-format on
    }

    void RayTracingDebugComponentConfig::CopySettingsTo(JE::Render::Features::RayTracingDebugSettingsInterface* settings)
    {
        if (!settings)
        {
            return;
        }

        // clang-format off
        #define COPY_TARGET settings
        #include <JE/Render/Features/ParamMacros/StartParamCopySettingsTo.inl>
        #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
        #include <JE/Render/Features/ParamMacros/EndParams.inl>
        #undef COPY_TARGET
        // clang-format on
    }
} // namespace AZ::Render
