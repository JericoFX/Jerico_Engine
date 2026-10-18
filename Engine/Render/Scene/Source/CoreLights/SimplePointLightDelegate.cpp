/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/CoreLights/SimplePointLightFeatureProcessorInterface.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/Scene/CoreLights/AreaLightComponentConfig.h>
#include <CoreLights/SimplePointLightDelegate.h>

namespace JE::Render::Scene
{
    SimplePointLightDelegate::SimplePointLightDelegate(AZ::EntityId entityId, bool isVisible)
        : LightDelegateBase<JE::Render::Features::SimplePointLightFeatureProcessorInterface>(entityId, isVisible)
    {
        InitBase(entityId);
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
        }
    }

    float SimplePointLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        const float intensity = GetPhotometricValue().GetCombinedIntensity(JE::Render::Features::PhotometricUnit::Lumen);
        return AZ::Sqrt(intensity / lightThreshold);
    }

    float SimplePointLightDelegate::GetSurfaceArea() const
    {
        return 0.0f;
    }

    void SimplePointLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
        }
    }

    void SimplePointLightDelegate::DrawDebugDisplay(
        const AZ::Transform& transform, const AZ::Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            debugDisplay.SetColor(color);

            // Draw a sphere for the attenuation radius
            debugDisplay.DrawWireSphere(transform.GetTranslation(), GetConfig()->m_attenuationRadius);
        }
    }

    void SimplePointLightDelegate::SetAffectsGI(bool affectsGI)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGI(GetLightHandle(), affectsGI);
        }
    }

    void SimplePointLightDelegate::SetAffectsGIFactor(float affectsGIFactor)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetAffectsGIFactor(GetLightHandle(), affectsGIFactor);
        }
    }

    AZ::Aabb SimplePointLightDelegate::GetLocalVisualizationBounds() const
    {
        return AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), GetConfig()->m_attenuationRadius);
    }
} // namespace AZ::Render
