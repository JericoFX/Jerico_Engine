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

#include <JE/Render/Scene/PostProcess/ExposureControl/ExposureControlBus.h>
#include <JE/Render/Scene/PostProcess/ExposureControl/ExposureControlComponentConfig.h>

#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlConstants.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace JE::Editor::Render::Scene
{
    class EditorExposureControlComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class ExposureControlComponentController final
            : public ExposureControlRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorExposureControlComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::ExposureControlComponentController, "AZ::Render::ExposureControlComponentController", "{A9D74E65-D1EE-416E-9108-B321526B049C}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            ExposureControlComponentController() = default;
            ExposureControlComponentController(const ExposureControlComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ExposureControlComponentConfig& config);
            const ExposureControlComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(ExposureControlComponentController);

            void OnConfigChanged();

            AZ::Render::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            AZ::Render::ExposureControlSettingsInterface* m_settingsInterface = nullptr;
            ExposureControlComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    }
}
