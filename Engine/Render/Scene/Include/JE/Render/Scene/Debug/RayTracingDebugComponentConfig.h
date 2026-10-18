/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Debug/RayTracingDebugSettingsInterface.h>
#include <AzCore/Component/ComponentBus.h>

namespace JE::Render::Scene
{
    class RayTracingDebugComponentConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingDebugComponentConfig, AZ::SystemAllocator)
        AZ_RTTI(RayTracingDebugComponentConfig, "{856B64FB-A019-47B8-A9D4-F74E18BE847A}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        // clang-format off
        // Generate members
        #include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
        #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
        #include <JE/Render/Features/ParamMacros/EndParams.inl>

        // Generate Getters/Setters
        #include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
        #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
        #include <JE/Render/Features/ParamMacros/EndParams.inl>
        // clang-format on

        void CopySettingsFrom(JE::Render::Features::RayTracingDebugSettingsInterface* settings);
        void CopySettingsTo(JE::Render::Features::RayTracingDebugSettingsInterface* settings);
    };
} // namespace AZ::Render
