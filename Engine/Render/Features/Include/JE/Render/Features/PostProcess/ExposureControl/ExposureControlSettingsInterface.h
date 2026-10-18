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
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlConstants.h>

namespace JE::Render
{
    namespace Features
    {
        class ExposureControlSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ExposureControlSettingsInterface, "AZ::Render::ExposureControlSettingsInterface", "{CE2AAFB3-137A-490F-B5D8-461D92812E92}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    }
}
