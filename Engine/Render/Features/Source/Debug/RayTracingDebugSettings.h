/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Debug/RayTracingDebugSettingsInterface.h>

namespace JE::Render::Features
{
    // The settings class for the ray tracing debug view
    class RayTracingDebugSettings final : public RayTracingDebugSettingsInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingDebugSettings, AZ::SystemAllocator);
        AZ_RTTI(RayTracingDebugSettings, "{5EED94CE-79FF-4F78-9ADC-A6D186F346E0}");

        // clang-format off
        // Generate getters and setters
        #include <JE/Render/Features/ParamMacros/StartParamFunctionsOverrideImpl.inl>
        #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
        #include <JE/Render/Features/ParamMacros/EndParams.inl>
        // clang-format on

    private:
        // clang-format off
        // Generate members
        #include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
        #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
        #include <JE/Render/Features/ParamMacros/EndParams.inl>
        // clang-format on
    };
} // namespace JE::Render::Features
