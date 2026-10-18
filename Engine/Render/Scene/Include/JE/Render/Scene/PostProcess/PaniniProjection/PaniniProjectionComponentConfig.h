/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionSettingsInterface.h>
#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        class PaniniProjectionComponentConfig final : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::PaniniProjectionComponentConfig, "AZ::Render::PaniniProjectionComponentConfig", "{AB9BE317-2A16-4737-9BB9-E39C0EF0F444}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
#include <JE/Render/Features/PostProcess/PaniniProjection/PaniniProjectionParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void CopySettingsFrom(JE::Render::Features::PaniniProjectionSettingsInterface* settings);
            void CopySettingsTo(JE::Render::Features::PaniniProjectionSettingsInterface* settings);

            bool ArePropertiesReadOnly() const
            {
                return !m_enabled;
            }
        };
    } // namespace Render
} // namespace AZ
