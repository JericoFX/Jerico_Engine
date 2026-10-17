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

#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <Atom/Feature/CoreLights/DiskLightFeatureProcessorInterface.h>

namespace JE::Render::Scene
{
    //! Manages rendering a disk light through the disk light feature processor and communication with a disk shape bus for the area light
    //! component.
    class DiskLightDelegate final : public LightDelegateBase<AZ::Render::DiskLightFeatureProcessorInterface>
    {
        using Base = LightDelegateBase<AZ::Render::DiskLightFeatureProcessorInterface>;

    public:
        DiskLightDelegate(LmbrCentral::DiskShapeComponentRequests* shapeBus, AZ::EntityId entityId, bool isVisible);

        // LightDelegateBase overrides...
        float GetSurfaceArea() const override;
        float GetEffectiveSolidAngle() const override;
        float CalculateAttenuationRadius(float lightThreshold) const override;
        void DrawDebugDisplay(
            const AZ::Transform& transform,
            const AZ::Color& color,
            AzFramework::DebugDisplayRequests& debugDisplay,
            bool isSelected) const override;
        void SetEnableShutters(bool enabled) override;
        void SetShutterAngles(float innerAngleDegrees, float outerAngleDegrees) override;
        void SetEnableShadow(bool enabled) override;
        void SetShadowBias(float bias) override;
        void SetShadowmapMaxSize(AZ::Render::ShadowmapSize size) override;
        void SetShadowFilterMethod(AZ::Render::ShadowFilterMethod method) override;
        void SetFilteringSampleCount(uint32_t count) override;
        void SetEsmExponent(float exponent) override;
        void SetNormalShadowBias(float bias) override;
        void SetShadowCachingMode(AreaLightComponentConfig::ShadowCachingMode cachingMode) override;
        void SetAffectsGI(bool affectsGI) override;
        void SetAffectsGIFactor(float affectsGIFactor) override;
        AZ::Aabb GetLocalVisualizationBounds() const override;

    private:
        // LightDelegateBase overrides...
        void HandleShapeChanged() override;

        float GetRadius() const;

        struct ConeVisualizationDimensions
        {
            float m_topRadius;
            float m_bottomRadius;
            float m_height;
        };

        ConeVisualizationDimensions CalculateConeVisualizationDimensions(float degrees) const;

        LmbrCentral::DiskShapeComponentRequests* m_shapeBus = nullptr;
    };

    inline float DiskLightDelegate::GetEffectiveSolidAngle() const
    {
        return AZ::Render::PhotometricValue::DirectionalEffectiveSteradians;
    }
} // namespace AZ::Render
