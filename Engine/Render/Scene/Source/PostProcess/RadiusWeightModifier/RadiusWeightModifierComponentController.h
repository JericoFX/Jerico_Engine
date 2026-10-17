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
#include <JE/Render/Scene/PostProcess/RadiusWeightModifier/RadiusWeightModifierComponentConfig.h>
#include <JE/Render/Scene/PostProcess/PostFxWeightRequestBus.h>

namespace JE::Editor::Render::Scene
{
    class EditorRadiusWeightModifierComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class RadiusWeightModifierComponentController final
            : public PostFxWeightRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorRadiusWeightModifierComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::RadiusWeightModifierComponentController, "AZ::Render::RadiusWeightModifierComponentController", "{29565EC9-8DE1-46A5-B20C-328AA6ED23C6}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            RadiusWeightModifierComponentController() = default;
            RadiusWeightModifierComponentController(const RadiusWeightModifierComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const RadiusWeightModifierComponentConfig& config);
            const RadiusWeightModifierComponentConfig& GetConfiguration() const { return m_configuration; }
            virtual float GetWeightAtPosition(const AZ::Vector3& influencerPosition) const override;
        private:
            AZ_DISABLE_COPY(RadiusWeightModifierComponentController);

            RadiusWeightModifierComponentConfig m_configuration;

            AZ::EntityId m_entityId;
        };
    }
}
