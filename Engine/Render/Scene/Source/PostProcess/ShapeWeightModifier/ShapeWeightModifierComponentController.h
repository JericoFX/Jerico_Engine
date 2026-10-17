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
#include <JE/Render/Scene/PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentConfig.h>
#include <JE/Render/Scene/PostProcess/PostFxWeightRequestBus.h>

namespace JE::Editor::Render::Scene
{
    class EditorShapeWeightModifierComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class ShapeWeightModifierComponentController final
            : public PostFxWeightRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorShapeWeightModifierComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::ShapeWeightModifierComponentController, "AZ::Render::ShapeWeightModifierComponentController", "{5EF82EB8-8A7F-4B6C-BD40-8BABA1ABE0E5}");
            static void Reflect(AZ::ReflectContext* context);

            ShapeWeightModifierComponentController() = default;
            ShapeWeightModifierComponentController(const ShapeWeightModifierComponentConfig& config);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ShapeWeightModifierComponentConfig& config);
            const ShapeWeightModifierComponentConfig& GetConfiguration() const { return m_configuration; }

            //! PostFxWeightRequestBus::Handler Override
            virtual float GetWeightAtPosition(const AZ::Vector3& influencerPosition) const override;
        private:
            float GetRatio(float maxRange, float minRange, float distance) const;
            AZ_DISABLE_COPY(ShapeWeightModifierComponentController);

            ShapeWeightModifierComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    }
}
