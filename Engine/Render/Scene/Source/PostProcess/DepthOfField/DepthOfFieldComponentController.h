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

#include <JE/Render/Scene/PostProcess/DepthOfField/DepthOfFieldBus.h>
#include <JE/Render/Scene/PostProcess/DepthOfField/DepthOfFieldComponentConfig.h>

#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace JE::Editor::Render::Scene
{
    class EditorDepthOfFieldComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class DepthOfFieldComponentController final
            : public DepthOfFieldRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorDepthOfFieldComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::DepthOfFieldComponentController, "AZ::Render::DepthOfFieldComponentController", "{D0E1675C-7E6F-472D-B037-E0D7ED5AFBE8}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DepthOfFieldComponentController() = default;
            DepthOfFieldComponentController(const DepthOfFieldComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DepthOfFieldComponentConfig& config);
            const DepthOfFieldComponentConfig& GetConfiguration() const;

            // Updates members that are inferred from the values of other parameters
            void UpdateInferredParams();

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(DepthOfFieldComponentController);

            void OnConfigChanged();

            AZ::Render::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            AZ::Render::DepthOfFieldSettingsInterface* m_depthOfFieldSettingsInterface = nullptr;
            DepthOfFieldComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    }
}
