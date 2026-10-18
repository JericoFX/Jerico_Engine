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
#include <JE/Render/Features/PostProcess/DepthOfField/DepthOfFieldConstants.h>

namespace JE::Render
{
    namespace Features
    {
        class DepthOfFieldSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::DepthOfFieldSettingsInterface, "AZ::Render::DepthOfFieldSettingsInterface", "{AC6DF992-DE82-4DF6-B071-75E81EFF5A65}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    }
}
