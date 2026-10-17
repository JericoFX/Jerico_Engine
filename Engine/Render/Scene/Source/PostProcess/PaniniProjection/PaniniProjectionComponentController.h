/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionConstants.h>
#include <Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

#include <JE/Render/Scene/PostProcess/PaniniProjection/PaniniProjectionBus.h>
#include <JE/Render/Scene/PostProcess/PaniniProjection/PaniniProjectionComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>


namespace JE::Editor::Render::Scene
{
    class EditorPaniniProjectionComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class PaniniProjectionComponentController final : public PaniniProjectionRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorPaniniProjectionComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::PaniniProjectionComponentController, "AZ::Render::PaniniProjectionComponentController", "{15B93DBF-D7E4-4F39-94BB-5F97606CF858}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            PaniniProjectionComponentController() = default;
            PaniniProjectionComponentController(const PaniniProjectionComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const PaniniProjectionComponentConfig& config);
            const PaniniProjectionComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(PaniniProjectionComponentController);

            void OnConfigChanged();

            AZ::Render::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            AZ::Render::PaniniProjectionSettingsInterface* m_settingsInterface = nullptr;
            PaniniProjectionComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ
