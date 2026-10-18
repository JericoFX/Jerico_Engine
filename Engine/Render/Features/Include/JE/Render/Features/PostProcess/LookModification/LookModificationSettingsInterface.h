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
#include <ACES/Aces.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>

namespace JE::Render
{
    namespace Features
    {
        class LookModificationSettingsInterface
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::LookModificationSettingsInterface, "AZ::Render::LookModificationSettingsInterface", "{0F6D09C7-1312-42C8-8B7A-1D2BDF36AB01}");

            // Auto-gen virtual getter and setter functions...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsVirtual.inl>
#include <JE/Render/Features/PostProcess/LookModification/LookModificationParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            virtual void OnConfigChanged() = 0;
        };

    }
}
