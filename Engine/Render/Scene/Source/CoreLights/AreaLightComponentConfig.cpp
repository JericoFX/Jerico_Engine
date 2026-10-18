/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Scene/CoreLights/AreaLightComponentConfig.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/limits.h>

namespace JE::Render
{
    namespace Scene
    {
        void AreaLightComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AreaLightComponentConfig, AZ::ComponentConfig>()
                    ->Version(10) // Added m_goboImageAsset
                    ->Field("LightType", &AreaLightComponentConfig::m_lightType)
                    ->Field("Color", &AreaLightComponentConfig::m_color)
                    ->Field("IntensityMode", &AreaLightComponentConfig::m_intensityMode)
                    ->Field("Intensity", &AreaLightComponentConfig::m_intensity)
                    ->Field("AttenuationRadiusMode", &AreaLightComponentConfig::m_attenuationRadiusMode)
                    ->Field("AttenuationRadius", &AreaLightComponentConfig::m_attenuationRadius)
                    ->Field("LightEmitsBothDirections", &AreaLightComponentConfig::m_lightEmitsBothDirections)
                    ->Field("UseFastApproximation", &AreaLightComponentConfig::m_useFastApproximation)
                    ->Field("GoboAsset", &AreaLightComponentConfig::m_goboImageAsset)
                    // Shutters
                    ->Field("EnableShutters", &AreaLightComponentConfig::m_enableShutters)
                    ->Field("InnerShutterAngleDegrees", &AreaLightComponentConfig::m_innerShutterAngleDegrees)
                    ->Field("OuterShutterAngleDegrees", &AreaLightComponentConfig::m_outerShutterAngleDegrees)
                    // Shadows
                    ->Field("Enable Shadow", &AreaLightComponentConfig::m_enableShadow)
                    ->Field("Shadow Bias", &AreaLightComponentConfig::m_bias)
                    ->Field("Normal Shadow Bias", &AreaLightComponentConfig::m_normalShadowBias)
                    ->Field("Shadowmap Max Size", &AreaLightComponentConfig::m_shadowmapMaxSize)
                    ->Field("Shadow Filter Method", &AreaLightComponentConfig::m_shadowFilterMethod)
                    ->Field("Filtering Sample Count", &AreaLightComponentConfig::m_filteringSampleCount)
                    ->Field("Esm Exponent", &AreaLightComponentConfig::m_esmExponent)
                    ->Field("Shadow Caching Mode", &AreaLightComponentConfig::m_shadowCachingMode)
                    ->Field("Shadow Caching Enabled", &AreaLightComponentConfig::m_cacheShadows) // temporary attribute that is used for edit context but ignored in serialize context.
                    // Global Illumination
                    ->Field("Affects GI", &AreaLightComponentConfig::m_affectsGI)
                    ->Field("Affects GI Factor", &AreaLightComponentConfig::m_affectsGIFactor)
                    // Lighting channel
                    ->Field("LightingChannelConfig", &AreaLightComponentConfig::m_lightingChannelConfig)
                    ;
            }
        }
        
        AZStd::vector<AZ::Edit::EnumConstant<JE::Render::Features::PhotometricUnit>> AreaLightComponentConfig::GetValidPhotometricUnits() const
        {
            AZStd::vector<AZ::Edit::EnumConstant<JE::Render::Features::PhotometricUnit>> enumValues =
            {
                // Candela & lumen always supported.
                AZ::Edit::EnumConstant<JE::Render::Features::PhotometricUnit>(JE::Render::Features::PhotometricUnit::Candela, "Candela"),
                AZ::Edit::EnumConstant<JE::Render::Features::PhotometricUnit>(JE::Render::Features::PhotometricUnit::Lumen, "Lumen"),
            };

            if (RequiresShapeComponent())
            {
                // Lights with surface area also support nits and ev100.
                enumValues.push_back(AZ::Edit::EnumConstant<JE::Render::Features::PhotometricUnit>(JE::Render::Features::PhotometricUnit::Nit, "Nit"));
                enumValues.push_back(AZ::Edit::EnumConstant<JE::Render::Features::PhotometricUnit>(JE::Render::Features::PhotometricUnit::Ev100Luminance, "Ev100"));
            }
            return enumValues;
        }

        bool AreaLightComponentConfig::RequiresShapeComponent() const
        {
            return m_lightType == LightType::Sphere
                || m_lightType == LightType::SpotDisk
                || m_lightType == LightType::Capsule
                || m_lightType == LightType::Quad
                || m_lightType == LightType::Polygon;
        }

        bool AreaLightComponentConfig::LightTypeIsSelected() const
        {
            return m_lightType != LightType::Unknown;
        }

        bool AreaLightComponentConfig::IsAttenuationRadiusModeAutomatic() const
        {
            return m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic;
        }

        bool AreaLightComponentConfig::SupportsBothDirections() const
        {
            return m_lightType == LightType::Quad
                || m_lightType == LightType::Polygon;
        }

        bool AreaLightComponentConfig::SupportsFastApproximation() const
        {
            return m_lightType == LightType::Quad;
        }
        
        bool AreaLightComponentConfig::SupportsShutters() const
        {
            return m_lightType == LightType::SimpleSpot
                || m_lightType == LightType::SpotDisk;
        }

        bool AreaLightComponentConfig::ShuttersMustBeEnabled() const
        {
            return m_lightType == LightType::SpotDisk;
        }

        bool AreaLightComponentConfig::ShuttersDisabled() const
        {
            return  m_lightType == LightType::SpotDisk && !m_enableShutters;
        }

        bool AreaLightComponentConfig::SupportsShadows() const
        {
            return m_lightType == LightType::SpotDisk || m_lightType == LightType::Sphere || m_lightType == LightType::SimpleSpot;
        }

        bool AreaLightComponentConfig::SupportsAffectsGI() const
        {
            return m_lightType == LightType::Sphere
                || m_lightType == LightType::SpotDisk
                || m_lightType == LightType::Capsule
                || m_lightType == LightType::Quad
                || m_lightType == LightType::SimplePoint
                || m_lightType == LightType::SimpleSpot;
        }
        
        bool AreaLightComponentConfig::ShadowsDisabled() const
        {
            return !m_enableShadow;
        }

        const char* AreaLightComponentConfig::GetIntensitySuffix() const
        {
            return JE::Render::Features::PhotometricValue::GetTypeSuffix(m_intensityMode);
        }

        float AreaLightComponentConfig::GetIntensityMin() const
        {
            switch (m_intensityMode)
            {
            case JE::Render::Features::PhotometricUnit::Candela:
            case JE::Render::Features::PhotometricUnit::Lumen:
            case JE::Render::Features::PhotometricUnit::Nit:
                return 0.0f;
            case JE::Render::Features::PhotometricUnit::Ev100Luminance:
                return AZStd::numeric_limits<float>::lowest();
            }
            return 0.0f;
        }

        float AreaLightComponentConfig::GetIntensityMax() const
        {
            // While there is no hard-max, a max must be included when there is a hard min.
            return AZStd::numeric_limits<float>::max();
        }

        float AreaLightComponentConfig::GetIntensitySoftMin() const
        {
            switch (m_intensityMode)
            {
            case JE::Render::Features::PhotometricUnit::Candela:
            case JE::Render::Features::PhotometricUnit::Lumen:
            case JE::Render::Features::PhotometricUnit::Nit:
            case JE::Render::Features::PhotometricUnit::Ev100Luminance:
                return 0.0f;
            }
            return 0.0f;
        }

        float AreaLightComponentConfig::GetIntensitySoftMax() const
        {
            switch (m_intensityMode)
            {
            case JE::Render::Features::PhotometricUnit::Candela:
            case JE::Render::Features::PhotometricUnit::Lumen:
            case JE::Render::Features::PhotometricUnit::Nit:
                return 1'000.0f;
            case JE::Render::Features::PhotometricUnit::Ev100Luminance:
                return 16.0f;
            }
            return 0.0f;
        }
        
        bool AreaLightComponentConfig::IsShadowFilteringDisabled() const
        {
            return (m_shadowFilterMethod == JE::Render::Features::ShadowFilterMethod::None);
        }

        bool AreaLightComponentConfig::IsShadowPcfDisabled() const
        {
            return ShadowsDisabled() || !(m_shadowFilterMethod == JE::Render::Features::ShadowFilterMethod::Pcf ||
                m_shadowFilterMethod == JE::Render::Features::ShadowFilterMethod::EsmPcf);
        }

        bool AreaLightComponentConfig::IsEsmDisabled() const
        {
            return ShadowsDisabled() ||
                !(m_shadowFilterMethod == JE::Render::Features::ShadowFilterMethod::Esm || m_shadowFilterMethod == JE::Render::Features::ShadowFilterMethod::EsmPcf);
        }

        bool AreaLightComponentConfig::SupportsGobo() const
        {
            return m_lightType == LightType::SimpleSpot;
        }
    }
}
