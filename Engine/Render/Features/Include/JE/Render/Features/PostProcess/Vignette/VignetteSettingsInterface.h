/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/Vignette/VignetteConstants.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

namespace JE::Render
{
    namespace Features
    {
        class VignetteSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::VignetteSettingsInterface, "AZ::Render::VignetteSettingsInterface", "{FDBB7B33-DD8B-48A7-BB01-6558984F6771}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/Vignette/VignetteParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    } // namespace Features
} // namespace JE::Render
