/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <JE/Render/Scene/SkyBox/PhysicalSkyComponentConfig.h>
#include <JE/Render/Scene/SkyBox/PhysicalSkyBus.h>
#include <JE/Render/Features/SkyBox/SkyBoxFogBus.h>
#include <JE/Render/Features/SkyBox/SkyBoxFeatureProcessorInterface.h>

namespace JE::Editor::Render::Scene
{
    class EditorPhysicalSkyComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class PhysicalSkyComponentController final
            : public AZ::TransformNotificationBus::Handler
            , public PhysicalSkyRequestBus::Handler
            , public JE::Render::Features::SkyBoxFogRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorPhysicalSkyComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::PhysicalSkyComponentController, "AZ::Render::PhysicalSkyComponentController", "{C3EEB94D-AEB9-4727-9493-791F86924804}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            PhysicalSkyComponentController() = default;
            PhysicalSkyComponentController(const PhysicalSkyComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const PhysicalSkyComponentConfig& config);
            const PhysicalSkyComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(PhysicalSkyComponentController);

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // PhysicalSkyRequestBus::Handler overrides ...
            void SetTurbidity(int turbidity) override;
            int GetTurbidity() override;
            void SetSunRadiusFactor(float factor) override;
            float GetSunRadiusFactor() override;
            void SetSkyIntensity(float intensity, JE::Render::Features::PhotometricUnit unit) override;
            void SetSkyIntensity(float intensity) override;
            float GetSkyIntensity(JE::Render::Features::PhotometricUnit unit) override;
            float GetSkyIntensity() override;
            void SetSunIntensity(float intensity, JE::Render::Features::PhotometricUnit unit) override;
            void SetSunIntensity(float intensity) override;
            float GetSunIntensity(JE::Render::Features::PhotometricUnit unit) override;
            float GetSunIntensity() override;

            // SkyBoxFogRequestBus::Handler overrides ...
            void SetEnabled(bool enable) override;
            bool IsEnabled() const override;
            void SetColor(const AZ::Color& color) override;
            const AZ::Color& GetColor() const override;
            void SetTopHeight(float topHeight) override;
            float GetTopHeight() const override;
            void SetBottomHeight(float bottomHeight) override;
            float GetBottomHeight() const override;

            //! Get Sun azimuth and altitude from entity transform, without scale
            JE::Render::Features::SunPosition GetSunTransform(const AZ::Transform& world);

            AZ::TransformInterface* m_transformInterface = nullptr;
            JE::Render::Features::SkyBoxFeatureProcessorInterface* m_featureProcessorInterface = nullptr;
            PhysicalSkyComponentConfig m_configuration;
            AZ::EntityId m_entityId;
            bool m_isActive = false;

            // For light unit update on UI
            JE::Render::Features::PhotometricValue m_skyPhotometricValue;
            JE::Render::Features::PhotometricValue m_sunPhotometricValue;
        };
    }
}
