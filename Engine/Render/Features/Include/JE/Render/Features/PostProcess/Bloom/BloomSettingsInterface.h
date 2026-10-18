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
#include <JE/Render/Features/PostProcess/Bloom/BloomConstants.h>

namespace JE::Render
{
    namespace Features
    {
        class BloomSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::BloomSettingsInterface, "AZ::Render::BloomSettingsInterface", "{1FCEEB59-7ED5-4FB7-9884-BC7865A152C5}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/Bloom/BloomParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    }
}
