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
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoConstants.h>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/SsaoConstants.h>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/GtaoConstants.h>

namespace JE::Render
{
    namespace Features
    {
        namespace Ao
        {
            enum class AoMethodType : uint8_t
            {
                SSAO,
                GTAO,
            };

            enum class GtaoQualityLevel : uint8_t
            {
                SuperLow,
                Low,
                Medium,
                High,
                SuperHigh,
            };
        }


        class AoSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::AoSettingsInterface, "AZ::Render::AoSettingsInterface", "{3316BA94-CCCA-4088-BAC4-91CFA8149533}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    }
}
