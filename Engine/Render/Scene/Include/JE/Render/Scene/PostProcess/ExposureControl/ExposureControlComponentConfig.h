/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlConstants.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlSettingsInterface.h>

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
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Generate Getters/Setters...
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void CopySettingsFrom(AZ::Render::ExposureControlSettingsInterface* settings);
            void CopySettingsTo(AZ::Render::ExposureControlSettingsInterface* settings);

            bool ArePropertiesReadOnly() const { return !m_enabled; }
            bool IsEyeAdaptation() const { return m_exposureControlType == AZ::Render::ExposureControl::ExposureControlType::EyeAdaptation; }
        };
    }
}
