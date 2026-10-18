/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionConstants.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

namespace JE::Render
{
    namespace Features
    {
        class PaniniProjectionSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::PaniniProjectionSettingsInterface, "AZ::Render::PaniniProjectionSettingsInterface", "{755D76BA-75EE-4519-A1FC-4D882DED8752}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    } // namespace Features
} // namespace JE::Render
