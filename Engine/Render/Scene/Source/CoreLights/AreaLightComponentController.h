/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <JE/Render/Scene/CoreLights/AreaLightBus.h>
#include <JE/Render/Scene/CoreLights/AreaLightComponentConfig.h>

#include <CoreLights/LightDelegateInterface.h>

namespace JE::Editor::Render::Scene
{
    class EditorAreaLightComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class AreaLightComponentController final
            : private AreaLightRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorAreaLightComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::AreaLightComponentController, "AZ::Render::AreaLightComponentController", "{C185C0F7-0923-4EF7-94F7-B41D60FE535B}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            AreaLightComponentController() = default;
            AreaLightComponentController(const AreaLightComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const AreaLightComponentConfig& config);
            const AreaLightComponentConfig& GetConfiguration() const;

            //! Used by the editor to control visibility - the controller must remain active while invisible to handle light unit conversions.
            void SetVisibiliy(bool isVisible);

        private:

            AZ_DISABLE_COPY(AreaLightComponentController);

            // AreaLightRequestBus::Handler overrides ...
            const AZ::Color& GetColor() const override;
            void SetColor(const AZ::Color& color) override;
            bool GetLightEmitsBothDirections() const override;
            void SetLightEmitsBothDirections(bool value) override;
            bool GetUseFastApproximation() const override;
            void SetUseFastApproximation(bool useFastApproximation) override;
            AZ::Render::PhotometricUnit GetIntensityMode() const override;
            float GetIntensity() const override;
            void SetIntensityAndMode(float intensity, AZ::Render::PhotometricUnit intensityMode) override;
            void SetIntensity(float intensity, AZ::Render::PhotometricUnit intensityMode) override;
            void SetIntensity(float intensity) override;
            float GetAttenuationRadius() const override;
            void SetAttenuationRadius(float radius) override;
            void SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode) override;
            void ConvertToIntensityMode(AZ::Render::PhotometricUnit intensityMode) override;
            float GetSurfaceArea() const override;

            bool GetEnableShutters() const override;
            void SetEnableShutters(bool enabled) override;
            float GetInnerShutterAngle() const override;
            void SetInnerShutterAngle(float degrees) override;
            float GetOuterShutterAngle() const override;
            void SetOuterShutterAngle(float degrees) override;

            bool GetEnableShadow() const override;
            void SetEnableShadow(bool enabled) override;
            float GetShadowBias() const override;
            void SetShadowBias(float bias) override;
            AZ::Render::ShadowmapSize GetShadowmapMaxSize() const override;
            void SetShadowmapMaxSize(AZ::Render::ShadowmapSize size) override;
            AZ::Render::ShadowFilterMethod GetShadowFilterMethod() const override;
            void SetShadowFilterMethod(AZ::Render::ShadowFilterMethod method) override;
            uint32_t GetFilteringSampleCount() const override;
            void SetFilteringSampleCount(uint32_t count) override;
            float GetEsmExponent() const override;
            void SetEsmExponent(float exponent) override;
            float GetNormalShadowBias() const override;
            void SetNormalShadowBias(float bias) override;
            AreaLightComponentConfig::ShadowCachingMode GetShadowCachingMode() const override;
            void SetShadowCachingMode(AreaLightComponentConfig::ShadowCachingMode cachingMode) override;

            bool GetAffectsGI() const override;
            void SetAffectsGI(bool affectsGI) const override;
            float GetAffectsGIFactor() const override;
            void SetAffectsGIFactor(float affectsGIFactor) const override;

            uint32_t GetLightingChannelMask() const;
            void SetLightingChannelMask(uint32_t lightingChannelMask) override;

            AZ::Aabb GetLocalVisualizationBounds() const override;

            void HandleDisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay,
                bool isSelected);

            void VerifyLightTypeAndShapeComponent();

            void ConfigurationChanged();
            void IntensityChanged();
            void ChromaChanged();
            void AttenuationRadiusChanged();
            void ShuttersChanged();
            void ShadowsChanged();
            void LightingChannelMaskChanged();
            
            //! Handles calculating the attenuation radius when LightAttenuationRadiusMode is auto
            void AutoCalculateAttenuationRadius();
            //! Handles creating the light shape delegate. Separate function to allow for early returns once the correct shape interface is found.
            void CreateLightShapeDelegate();

            AZStd::unique_ptr<LightDelegateInterface> m_lightShapeDelegate;
            AreaLightComponentConfig m_configuration;
            AZ::EntityId m_entityId;
            bool m_isVisible = true;
        };

    } // namespace Render
} // AZ namespace
