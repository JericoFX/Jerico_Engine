/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/CoreLights/PhotometricValue.h>
#include <JE/Render/Scene/CoreLights/DirectionalLightComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/limits.h>

namespace JE::Render
{
    namespace Scene
    {
        void DirectionalLightComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DirectionalLightComponentConfig, AZ::ComponentConfig>()
                    ->Version(10) // Added AffectsGI
                    ->Field("Color", &DirectionalLightComponentConfig::m_color)
                    ->Field("IntensityMode", &DirectionalLightComponentConfig::m_intensityMode)
                    ->Field("Intensity", &DirectionalLightComponentConfig::m_intensity)
                    ->Field("AngularDiameter", &DirectionalLightComponentConfig::m_angularDiameter)
                    ->Field("CameraEntityId", &DirectionalLightComponentConfig::m_cameraEntityId)
                    ->Field("Shadow Enabled", &DirectionalLightComponentConfig::m_shadowEnabled)
                    ->Field("ShadowFarClipDistance", &DirectionalLightComponentConfig::m_shadowFarClipDistance)
                    ->Field("ShadowmapSize", &DirectionalLightComponentConfig::m_shadowmapSize)
                    ->Field("CascadeCount", &DirectionalLightComponentConfig::m_cascadeCount)
                    ->Field("SplitAutomatic", &DirectionalLightComponentConfig::m_isShadowmapFrustumSplitAutomatic)
                    ->Field("SplitRatio", &DirectionalLightComponentConfig::m_shadowmapFrustumSplitSchemeRatio)
                    ->Field("CascadeFarDepths", &DirectionalLightComponentConfig::m_cascadeFarDepths)
                    ->Field("GroundHeight", &DirectionalLightComponentConfig::m_groundHeight)
                    ->Field("IsCascadeCorrectionEnabled", &DirectionalLightComponentConfig::m_isCascadeCorrectionEnabled)
                    ->Field("IsDebugColoringEnabled", &DirectionalLightComponentConfig::m_isDebugColoringEnabled)
                    ->Field("ShadowFilterMethod", &DirectionalLightComponentConfig::m_shadowFilterMethod)
                    ->Field("PcfFilteringSampleCount", &DirectionalLightComponentConfig::m_filteringSampleCount)
                    ->Field("ShadowReceiverPlaneBiasEnabled", &DirectionalLightComponentConfig::m_receiverPlaneBiasEnabled)
                    ->Field("Shadow Bias", &DirectionalLightComponentConfig::m_shadowBias)
                    ->Field("Normal Shadow Bias", &DirectionalLightComponentConfig::m_normalShadowBias)
                    ->Field("CascadeBlendingEnabled", &DirectionalLightComponentConfig::m_cascadeBlendingEnabled)
                    ->Field("FullscreenBlurEnabled", &DirectionalLightComponentConfig::m_fullscreenBlurEnabled)
                    ->Field("FullscreenBlurConstFalloff", &DirectionalLightComponentConfig::m_fullscreenBlurConstFalloff)
                    ->Field("FullscreenBlurDepthFalloffStrength", &DirectionalLightComponentConfig::m_fullscreenBlurDepthFalloffStrength)
                    ->Field("Affects GI", &DirectionalLightComponentConfig::m_affectsGI)
                    ->Field("Affects GI Factor", &DirectionalLightComponentConfig::m_affectsGIFactor)
                    ->Field("LightingChannelConfig", &DirectionalLightComponentConfig::m_lightingChannelConfig)
                    ;
            }
        }

        const char* DirectionalLightComponentConfig::GetIntensitySuffix() const
        {
            return JE::Render::Features::PhotometricValue::GetTypeSuffix(m_intensityMode);
        }

        float DirectionalLightComponentConfig::GetIntensityMin() const
        {
            switch (m_intensityMode)
            {
            case JE::Render::Features::PhotometricUnit::Lux:
                return 0.0f;
            case JE::Render::Features::PhotometricUnit::Ev100Illuminance:
                return AZStd::numeric_limits<float>::lowest();
            }
            return 0.0f;
        }

        float DirectionalLightComponentConfig::GetIntensityMax() const
        {
            // While there is no hard-max, a max must be included when there is a hard min.
            return AZStd::numeric_limits<float>::max();
        }

        float DirectionalLightComponentConfig::GetIntensitySoftMin() const
        {
            switch (m_intensityMode)
            {
            case JE::Render::Features::PhotometricUnit::Lux:
                return 0.0f;
            case JE::Render::Features::PhotometricUnit::Ev100Illuminance:
                return -4.0f;
            }
            return 0.0f;
        }

        float DirectionalLightComponentConfig::GetIntensitySoftMax() const
        {
            switch (m_intensityMode)
            {
            case JE::Render::Features::PhotometricUnit::Lux:
                return 200'000.0f;
            case JE::Render::Features::PhotometricUnit::Ev100Illuminance:
                return 16.0f;
            }
            return 0.0f;
        }

        bool DirectionalLightComponentConfig::IsSplitManual() const
        {
            return !m_isShadowmapFrustumSplitAutomatic;
        }

        bool DirectionalLightComponentConfig::IsSplitAutomatic() const
        {
            return m_isShadowmapFrustumSplitAutomatic;
        }

        bool DirectionalLightComponentConfig::IsShadowDisabled() const
        {
            return !m_shadowEnabled;
        }

        bool DirectionalLightComponentConfig::IsCascadeCorrectionDisabled() const
        {
            return (!m_shadowEnabled || m_cascadeCount == 1 || !m_isCascadeCorrectionEnabled);
        }

        bool DirectionalLightComponentConfig::IsShadowFilteringDisabled() const
        {
            return (!m_shadowEnabled || m_shadowFilterMethod == JE::Render::Features::ShadowFilterMethod::None);
        }

        bool DirectionalLightComponentConfig::IsShadowPcfDisabled() const
        {
            return !m_shadowEnabled || m_shadowFilterMethod != JE::Render::Features::ShadowFilterMethod::Pcf;
        }

        bool DirectionalLightComponentConfig::IsEsmDisabled() const
        {
            return !m_shadowEnabled || (m_shadowFilterMethod != JE::Render::Features::ShadowFilterMethod::Esm && m_shadowFilterMethod != JE::Render::Features::ShadowFilterMethod::EsmPcf);
        }

        AZ::Crc32 DirectionalLightComponentConfig::UpdateCascadeFarDepths()
        {
            if (IsSplitAutomatic())
            {
                for (uint32_t i = 0; i < JE::Render::Features::Shadow::MaxNumberOfCascades; ++i)
                {
                    m_cascadeFarDepths.SetElement(
                        i, i < m_cascadeCount ? m_shadowFarClipDistance * (i + 1) / m_cascadeCount : m_shadowFarClipDistance);
                }
            }
            m_cascadeFarDepths =
                m_cascadeFarDepths.GetClamp(AZ::Vector4(JE::Render::Scene::DirectionalLightConstants::MIN_CASCADE_FAR_DEPTH), AZ::Vector4(m_shadowFarClipDistance));
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
