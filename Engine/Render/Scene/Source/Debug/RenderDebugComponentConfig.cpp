/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/Debug/RenderDebugComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace JE::Render::Scene
{
    void RenderDebugComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RenderDebugComponentConfig, AZ::ComponentConfig>()
                ->Version(0)

                // Auto-gen serialize context code...
#define SERIALIZE_CLASS RenderDebugComponentConfig
#include <JE/Render/Features/ParamMacros/StartParamSerializeContext.inl>
#include <JE/Render/Features/Debug/RenderDebugParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                ;
        }
    }

    void RenderDebugComponentConfig::CopySettingsFrom(JE::Render::Features::RenderDebugSettingsInterface* settings)
    {
        if (!settings)
        {
            return;
        }

#define COPY_SOURCE settings
#include <JE/Render/Features/ParamMacros/StartParamCopySettingsFrom.inl>
#include <JE/Render/Features/Debug/RenderDebugParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef COPY_SOURCE
    }

    void RenderDebugComponentConfig::CopySettingsTo(JE::Render::Features::RenderDebugSettingsInterface* settings)
    {
        if (!settings)
        {
            return;
        }

#define COPY_TARGET settings
#include <JE/Render/Features/ParamMacros/StartParamCopySettingsTo.inl>
#include <JE/Render/Features/Debug/RenderDebugParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>
#undef COPY_TARGET
    }

}
