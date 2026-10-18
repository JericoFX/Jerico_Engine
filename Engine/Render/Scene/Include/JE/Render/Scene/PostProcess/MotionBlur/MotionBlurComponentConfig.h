/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurSettingsInterface.h>
#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        class MotionBlurComponentConfig final : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::MotionBlurComponentConfig, "AZ::Render::MotionBlurComponentConfig", "{FA2B4083-D397-4B05-A644-B4CFF6372D88}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
#include <JE/Render/Features/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void CopySettingsFrom(JE::Render::Features::MotionBlurSettingsInterface* settings);
            void CopySettingsTo(JE::Render::Features::MotionBlurSettingsInterface* settings);

            bool ArePropertiesReadOnly() const
            {
                return !m_enabled;
            }
        };
    } // namespace Render
} // namespace AZ
