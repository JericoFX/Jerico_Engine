/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/Scene/CoreLights/AreaLightComponentConfig.h>
#include <CoreLights/PolygonLightDelegate.h>

namespace JE::Render::Scene
{
    PolygonLightDelegate::PolygonLightDelegate(LmbrCentral::PolygonPrismShapeComponentRequests* shapeBus, AZ::EntityId entityId, bool isVisible)
        : LightDelegateBase<JE::Render::Features::PolygonLightFeatureProcessorInterface>(entityId, isVisible)
        , m_shapeBus(shapeBus)
    {
        InitBase(entityId);
        shapeBus->SetHeight(0.0f);
    }

    void PolygonLightDelegate::SetLightEmitsBothDirections(bool lightEmitsBothDirections)
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetLightEmitsBothDirections(GetLightHandle(), lightEmitsBothDirections);
        }
    }

    float PolygonLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
    {
        // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
        const float intensity = GetPhotometricValue().GetCombinedIntensity(JE::Render::Features::PhotometricUnit::Lumen);
        return AZ::Sqrt(intensity / lightThreshold);
    }

    void PolygonLightDelegate::HandleShapeChanged()
    {
        if (GetLightHandle().IsValid())
        {
            GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());

            AZStd::vector<AZ::Vector2> vertices = m_shapeBus->GetPolygonPrism()->m_vertexContainer.GetVertices();

            AZ::Transform transform = GetTransform();
            transform.SetUniformScale(transform.GetUniformScale()); // Polygon Prism only supports uniform scale.

            AZStd::vector<AZ::Vector3> transformedVertices;
            transformedVertices.reserve(vertices.size());
            for (const AZ::Vector2& vertex : vertices)
            {
                transformedVertices.push_back(transform.TransformPoint(AZ::Vector3(vertex, 0.0f)));
            }

            GetFeatureProcessor()->SetPolygonPoints(
                GetLightHandle(),
                transformedVertices.data(),
                static_cast<uint32_t>(transformedVertices.size()),
                GetTransform().GetBasisZ());
        }
    }

    float PolygonLightDelegate::GetSurfaceArea() const
    {
        // Surprisingly simple to calculate area of arbitrary polygon assuming no self-intersection. See
        // https://en.wikipedia.org/wiki/Shoelace_formula
        float twiceArea = 0.0f;
        AZStd::vector<AZ::Vector2> vertices = m_shapeBus->GetPolygonPrism()->m_vertexContainer.GetVertices();
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            size_t j = (i + 1) % vertices.size();
            twiceArea += vertices.at(i).GetX() * vertices.at(j).GetY();
            twiceArea -= vertices.at(i).GetY() * vertices.at(j).GetX();
        }
        float scale = GetTransform().GetUniformScale();
        return AZ::GetAbs(twiceArea * 0.5f * scale * scale);
    }

    void PolygonLightDelegate::DrawDebugDisplay(
        const AZ::Transform& transform, const AZ::Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
    {
        if (isSelected)
        {
            debugDisplay.SetColor(color);

            // Draw a Polygon for the attenuation radius
            debugDisplay.DrawWireSphere(transform.GetTranslation(), GetConfig()->m_attenuationRadius);
            debugDisplay.DrawArrow(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisZ());
        }
    }

    AZ::Aabb PolygonLightDelegate::GetLocalVisualizationBounds() const
    {
        return AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), GetConfig()->m_attenuationRadius);
    }
} // namespace AZ::Render
