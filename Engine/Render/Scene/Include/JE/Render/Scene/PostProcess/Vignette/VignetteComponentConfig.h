/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/Vignette/VignetteSettingsInterface.h>
#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        class VignetteComponentConfig final : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::VignetteComponentConfig, "AZ::Render::VignetteComponentConfig", "{2CB8446B-1410-4885-A3DE-11BB0590D91A}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/Vignette/VignetteParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
#include <JE/Render/Features/PostProcess/Vignette/VignetteParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void CopySettingsFrom(JE::Render::Features::VignetteSettingsInterface* settings);
            void CopySettingsTo(JE::Render::Features::VignetteSettingsInterface* settings);

            bool ArePropertiesReadOnly() const
            {
                return !m_enabled;
            }
        };
    } // namespace Render
} // namespace AZ
