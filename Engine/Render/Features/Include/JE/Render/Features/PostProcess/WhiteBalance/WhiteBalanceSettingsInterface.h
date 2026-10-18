/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/WhiteBalance/WhiteBalanceConstants.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

namespace JE::Render
{
    namespace Features
    {
        class WhiteBalanceSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::WhiteBalanceSettingsInterface, "AZ::Render::WhiteBalanceSettingsInterface", "{D63E0B9A-13BA-42BC-AAE8-C9D2C08AE42D}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/WhiteBalance/WhiteBalanceParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    } // namespace Features
} // namespace JE::Render
