/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>
#include <JE/Render/Features/Debug/RenderDebugConstants.h>

namespace JE::Render::Features
{
    class RenderDebugSettingsInterface
    {
    public:
        AZ_RTTI_WITH_NAME(JE::Render::Features::RenderDebugSettingsInterface, "AZ::Render::RenderDebugSettingsInterface", "{03E799EC-E682-4C09-BE62-B8055B5FD936}");

        // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/Debug/RenderDebugParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

    };

}
