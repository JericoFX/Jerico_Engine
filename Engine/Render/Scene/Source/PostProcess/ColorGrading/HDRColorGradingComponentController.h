/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <JE/Render/Scene/PostProcess/ColorGrading/HDRColorGradingComponentConfig.h>
#include <JE/Render/Scene/PostProcess/ColorGrading/HDRColorGradingBus.h>

#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

namespace JE::Editor::Render::Scene
{
    class EditorHDRColorGradingComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class HDRColorGradingComponentController final
            : public HDRColorGradingRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorHDRColorGradingComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::HDRColorGradingComponentController, "AZ::Render::HDRColorGradingComponentController", "{CA1D635C-64E9-42C7-A8E0-36C6B825B15D}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            HDRColorGradingComponentController() = default;
            HDRColorGradingComponentController(const HDRColorGradingComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const HDRColorGradingComponentConfig& config);
            const HDRColorGradingComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(HDRColorGradingComponentController);

            void OnConfigChanged();

            AZ::Render::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            AZ::Render::HDRColorGradingSettingsInterface* m_settingsInterface = nullptr;
            HDRColorGradingComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ
