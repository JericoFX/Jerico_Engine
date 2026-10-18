/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/WhiteBalance/WhiteBalanceSettingsInterface.h>
#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        class WhiteBalanceComponentConfig final : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::WhiteBalanceComponentConfig, "AZ::Render::WhiteBalanceComponentConfig", "{FA4DD42D-2BEA-4719-9F11-87E1D4CC165E}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/WhiteBalance/WhiteBalanceParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
#include <JE/Render/Features/PostProcess/WhiteBalance/WhiteBalanceParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void CopySettingsFrom(JE::Render::Features::WhiteBalanceSettingsInterface* settings);
            void CopySettingsTo(JE::Render::Features::WhiteBalanceSettingsInterface* settings);

            bool ArePropertiesReadOnly() const
            {
                return !m_enabled;
            }
        };
    } // namespace Render
} // namespace AZ
