/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Debug/RayTracingDebugConstants.h>
#include <AzCore/RTTI/RTTI.h>

namespace JE::Render::Features
{
    class RayTracingDebugSettingsInterface
    {
    public:
        AZ_RTTI(RayTracingDebugSettingsInterface, "{E1C76937-7F82-4553-87AA-D6DC8885DC56}");

        // clang-format off
        // Generate virtual getter and setter functions
        #include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
        #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
        #include <JE/Render/Features/ParamMacros/EndParams.inl>
        // clang-format on
    };
} // namespace JE::Render::Features
