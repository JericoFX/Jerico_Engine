/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>
#include <JE/Render/Features/ColorGrading/LutResolution.h>
#include <ACES/Aces.h>

namespace JE::Render
{
    namespace Features
    {
        class HDRColorGradingSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::HDRColorGradingSettingsInterface, "AZ::Render::HDRColorGradingSettingsInterface", "{CB5ADF78-27DE-438C-A991-1E5433046A42}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };
    }
}
