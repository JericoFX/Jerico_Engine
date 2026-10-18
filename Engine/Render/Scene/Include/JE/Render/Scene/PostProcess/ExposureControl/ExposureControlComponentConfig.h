/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlConstants.h>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlSettingsInterface.h>

namespace JE::Render
{
    namespace Scene
    {
        class ExposureControlComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(ExposureControlComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::ExposureControlComponentConfig, "AZ::Render::ExposureControlComponentConfig", "{3FBB712B-EA05-43DA-A2F5-9C8DA61AA787}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            // Generate members...
#include <JE/Render/Features/ParamMacros/StartParamMembers.inl>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <JE/Render/Features/ParamMacros/StartParamFunctions.inl>
#include <JE/Render/Features/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

            void CopySettingsFrom(JE::Render::Features::ExposureControlSettingsInterface* settings);
            void CopySettingsTo(JE::Render::Features::ExposureControlSettingsInterface* settings);

            bool ArePropertiesReadOnly() const { return !m_enabled; }
            bool IsEyeAdaptation() const { return m_exposureControlType == JE::Render::Features::ExposureControl::ExposureControlType::EyeAdaptation; }
        };
    }
}
