/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/SphereLightDelegate.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/Features/CoreLights/PointLightFeatureProcessorInterface.h>
#include <JE/Render/Scene/CoreLights/AreaLightComponentConfig.h>

namespace JE::Render::Scene
{
    SphereLightDelegate::SphereLightDelegate(LmbrCentral::SphereShapeComponentRequests* shapeBus, AZ::EntityId entityId, bool isVisible)
        : LightDelegateBase<JE::Render::Features::PointLightFeatureProcessorInterface>(entityId, isVisible)
        , m_sphereShapeBus(shapeBus)
    {
        InitBase(entityId);
    }

    float SphereLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        float intensity = GetPhotometricValue().GetCombinedIntensity(JE::Render::Features::PhotometricUnit::Lumen);
        return sqrt(intensity / lightThreshold);
    }
        
    void SphereLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
            GetFeatureProcessor()->SetBulbRadius(GetLightHandle(), GetRadius());
        }
    }

    float SphereLightDelegate::GetSurfaceArea() const
    {
        float radius = GetRadius();
        return 4.0f * AZ::Constants::Pi * radius * radius;
    }

    float SphereLightDelegate::GetRadius() const
    {
        return m_sphereShapeBus->GetRadius() * GetTransform().GetUniformScale();
    }

    void SphereLightDelegate::DrawDebugDisplay(const AZ::Transform& transform, const AZ::Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            debugDisplay.SetColor(color);
                
            // Draw a sphere for the attenuation radius
            debugDisplay.DrawWireSphere(transform.GetTranslation(), GetConfig()->m_attenuationRadius);
        }
    }

    void SphereLightDelegate::SetEnableShadow(bool enabled)
    {
        Base::SetEnableShadow(enabled);

        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowsEnabled(GetLightHandle(), enabled);
        }
    }
        
    void SphereLightDelegate::SetShadowBias(float bias)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowBias(GetLightHandle(), bias);
        }
    }

    void SphereLightDelegate::SetShadowmapMaxSize(JE::Render::Features::ShadowmapSize size)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowmapMaxResolution(GetLightHandle(), size);
        }
    }

    void SphereLightDelegate::SetShadowFilterMethod(JE::Render::Features::ShadowFilterMethod method)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetShadowFilterMethod(GetLightHandle(), method);
        }
    }

    void SphereLightDelegate::SetFilteringSampleCount(uint32_t count)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetFilteringSampleCount(GetLightHandle(), static_cast<uint16_t>(count));
        }
    }

    void SphereLightDelegate::SetEsmExponent(float esmExponent)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetEsmExponent(GetLightHandle(), esmExponent);
        }
    }

    void SphereLightDelegate::SetNormalShadowBias(float bias)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetNormalShadowBias(GetLightHandle(), bias);
        }
    }

    void SphereLightDelegate::SetShadowCachingMode(AreaLightComponentConfig::ShadowCachingMode cachingMode)
    {
        if (GetShadowsEnabled() && GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetUseCachedShadows(GetLightHandle(),
                cachingMode == AreaLightComponentConfig::ShadowCachingMode::UpdateOnChange);
        }
    }

    void SphereLightDelegate::SetAffectsGI(bool affectsGI)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGI(GetLightHandle(), affectsGI);
        }
    }

    void SphereLightDelegate::SetAffectsGIFactor(float affectsGIFactor)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGIFactor(GetLightHandle(), affectsGIFactor);
        }
    }

    AZ::Aabb SphereLightDelegate::GetLocalVisualizationBounds() const
    {
        const AZ::Vector3 translationOffset = m_shapeBus ? m_shapeBus->GetTranslationOffset() : AZ::Vector3::CreateZero();
        return AZ::Aabb::CreateCenterRadius(translationOffset, GetConfig()->m_attenuationRadius);
    }
} // namespace AZ::Render
