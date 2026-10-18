/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h>

namespace JE::Render
{
    namespace Scene
    {
        class HDRColorGradingComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(HDRColorGradingComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::HDRColorGradingComponentConfig, "AZ::Render::HDRColorGradingComponentConfig", "{8613EA4A-6E1C-49AD-87F3-9FECCB7EA36D}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
#include <JE/Render/Features/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void CopySettingsTo(JE::Render::Features::HDRColorGradingSettingsInterface* settings);
        };
    } // namespace Render
} // namespace AZ
