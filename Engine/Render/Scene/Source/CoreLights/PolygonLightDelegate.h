/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CoreLights/LightDelegateBase.h>

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <LmbrCentral/Shape/PolygonPrismShapeComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <JE/Render/Features/CoreLights/PolygonLightFeatureProcessorInterface.h>

namespace JE::Render::Scene
{
    //! Manages rendering a Polygon light through the Polygon light feature processor and communication with a Polygon shape bus for the
    //! area light component.
    class PolygonLightDelegate final : public LightDelegateBase<JE::Render::Features::PolygonLightFeatureProcessorInterface>
    {
    public:
        PolygonLightDelegate(LmbrCentral::PolygonPrismShapeComponentRequests* shapeBus, AZ::EntityId entityId, bool isVisible);

        // LightDelegateBase overrides...
        void SetLightEmitsBothDirections(bool lightEmitsBothDirections) override;
        float GetSurfaceArea() const override;
        float GetEffectiveSolidAngle() const override;
        float CalculateAttenuationRadius(float lightThreshold) const override;
        void DrawDebugDisplay(
            const AZ::Transform& transform,
            const AZ::Color& color,
            AzFramework::DebugDisplayRequests& debugDisplay,
            bool isSelected) const override;
        AZ::Aabb GetLocalVisualizationBounds() const override;

    private:
        // LightDelegateBase overrides...
        void HandleShapeChanged() override;

        LmbrCentral::PolygonPrismShapeComponentRequests* m_shapeBus = nullptr;
    };

    inline float PolygonLightDelegate::GetEffectiveSolidAngle() const
    {
        return JE::Render::Features::PhotometricValue::DirectionalEffectiveSteradians;
    }
} // namespace AZ::Render
