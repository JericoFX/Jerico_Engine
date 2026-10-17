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
#include <Atom/Feature/ParamMacros/StartParamSerializeContext.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef SERIALIZE_CLASS
                ;
        }
    }

    void RenderDebugComponentConfig::CopySettingsFrom(AZ::Render::RenderDebugSettingsInterface* settings)
    {
        if (!settings)
        {
            return;
        }

#define COPY_SOURCE settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsFrom.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_SOURCE
    }

    void RenderDebugComponentConfig::CopySettingsTo(AZ::Render::RenderDebugSettingsInterface* settings)
    {
        if (!settings)
        {
            return;
        }

#define COPY_TARGET settings
#include <Atom/Feature/ParamMacros/StartParamCopySettingsTo.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef COPY_TARGET
    }

}
