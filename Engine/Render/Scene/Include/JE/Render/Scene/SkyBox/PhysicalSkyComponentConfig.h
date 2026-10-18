/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Features/CoreLights/PhotometricValue.h>
#include <JE/Render/Features/SkyBox/SkyboxConstants.h>
#include <JE/Render/Features/SkyBox/SkyBoxFogSettings.h>

namespace JE::Render
{
    namespace Scene
    {
        class PhysicalSkyComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(PhysicalSkyComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::PhysicalSkyComponentConfig, "AZ::Render::PhysicalSkyComponentConfig", "{D0A40D6B-F838-46AB-A79C-CC2218C0146C}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            JE::Render::Features::PhotometricUnit m_intensityMode = JE::Render::Features::PhotometricUnit::Ev100Luminance;
            float m_skyIntensity = JE::Render::Features::PhysicalSkyDefaultIntensity;
            float m_sunIntensity = JE::Render::Features::PhysicalSunDefaultIntensity;

            int m_turbidity = 1;
            float m_sunRadiusFactor = 1.0f;

            JE::Render::Features::SkyBoxFogSettings m_skyBoxFogSettings;

            //! Returns characters for a suffix for the light type including a space. " lm" for lumens for example.
            const char* GetIntensitySuffix() const;

            //! Returns the minimum intensity value allowed depending on the m_intensityMode
            float GetSkyIntensityMin() const;
            float GetSunIntensityMin() const;

            //! Returns the maximum intensity value allowed depending on the m_intensityMode
            float GetSkyIntensityMax() const;
            float GetSunIntensityMax() const;

            bool IsFogDisabled() const { return !m_skyBoxFogSettings.m_enable; }
        };
    }
}
