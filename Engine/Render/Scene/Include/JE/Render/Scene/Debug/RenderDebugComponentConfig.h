/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/Debug/RenderDebugSettingsInterface.h>

namespace JE::Render::Scene
{
    class RenderDebugComponentConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RenderDebugComponentConfig, AZ::SystemAllocator)
        AZ_RTTI_WITH_NAME(JE::Render::Scene::RenderDebugComponentConfig, "AZ::Render::RenderDebugComponentConfig", "{07362463-95C9-40CE-89E7-76FD25978E96}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/Debug/RenderDebugParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        // Generate Getters/Setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
#include <JE/Render/Features/Debug/RenderDebugParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        void CopySettingsFrom(JE::Render::Features::RenderDebugSettingsInterface* settings);
        void CopySettingsTo(JE::Render::Features::RenderDebugSettingsInterface* settings);

        bool IsBaseColorReadOnly() { return !GetOverrideBaseColor(); }
        bool IsRoughnessReadOnly() { return !GetOverrideRoughness(); }
        bool IsMetallicReadOnly() { return !GetOverrideMetallic(); }
        bool IsDebugLightReadOnly() { return GetRenderDebugLightingSource() != JE::Render::Features::RenderDebugLightingSource::DebugLight; }

    };

}
