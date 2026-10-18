/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurConstants.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

namespace JE::Render
{
    namespace Features
    {
        namespace  MotionBlur
        {
            enum class SampleQuality : uint8_t
            {
                Low = 0,
                Medium = 1,
                High = 2,
                Ultra = 3,
            };
        }

        class MotionBlurSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::MotionBlurSettingsInterface, "AZ::Render::MotionBlurSettingsInterface", "{BB5B872B-A1EE-4A73-ABA8-0A714D6FC978}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    } // namespace Features
} // namespace JE::Render
