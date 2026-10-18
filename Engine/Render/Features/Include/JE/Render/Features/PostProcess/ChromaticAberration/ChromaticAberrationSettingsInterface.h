/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/ChromaticAberration/ChromaticAberrationConstants.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

namespace JE::Render
{
    namespace Features
    {
        class ChromaticAberrationSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ChromaticAberrationSettingsInterface, "AZ::Render::ChromaticAberrationSettingsInterface", "{725CC98B-AE22-4DE5-B68B-5D61A3270264}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    } // namespace Features
} // namespace JE::Render
