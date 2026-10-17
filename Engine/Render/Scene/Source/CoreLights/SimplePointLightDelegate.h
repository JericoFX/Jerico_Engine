/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <CoreLights/LightDelegateBase.h>

#include <Atom/Feature/CoreLights/SimplePointLightFeatureProcessorInterface.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace JE::Render::Scene
{
    class SimplePointLightDelegate final : public LightDelegateBase<AZ::Render::SimplePointLightFeatureProcessorInterface>
    {
    public:
        SimplePointLightDelegate(AZ::EntityId entityId, bool isVisible);

        // LightDelegateBase overrides...
        float CalculateAttenuationRadius(float lightThreshold) const override;
        void DrawDebugDisplay(
            const AZ::Transform& transform,
            const AZ::Color& color,
            AzFramework::DebugDisplayRequests& debugDisplay,
            bool isSelected) const override;
        float GetSurfaceArea() const override;
        float GetEffectiveSolidAngle() const override;
        void SetAffectsGI(bool affectsGI) override;
        void SetAffectsGIFactor(float affectsGIFactor) override;
        AZ::Aabb GetLocalVisualizationBounds() const override;

    private:
        void HandleShapeChanged() override;
    };

    inline float SimplePointLightDelegate::GetEffectiveSolidAngle() const
    {
        return AZ::Render::PhotometricValue::OmnidirectionalSteradians;
    }
} // namespace AZ::Render
