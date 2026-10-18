/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/Vignette/VignetteConstants.h>
#include <JE/Render/Features/PostProcess/Vignette/VignetteSettingsInterface.h>
#include <JE/Render/Features/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <JE/Render/Features/PostProcess/PostProcessSettingsInterface.h>

#include <JE/Render/Scene/PostProcess/Vignette/VignetteBus.h>
#include <JE/Render/Scene/PostProcess/Vignette/VignetteComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>


namespace JE::Editor::Render::Scene
{
    class EditorVignetteComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class VignetteComponentController final : public VignetteRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorVignetteComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::VignetteComponentController, "AZ::Render::VignetteComponentController", "{98B2F7E6-A8E3-443B-B301-E180FFE710F5}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            VignetteComponentController() = default;
            VignetteComponentController(const VignetteComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const VignetteComponentConfig& config);
            const VignetteComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverride.inl>
#include <JE/Render/Features/PostProcess/Vignette/VignetteParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(VignetteComponentController);

            void OnConfigChanged();

            JE::Render::Features::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            JE::Render::Features::VignetteSettingsInterface* m_settingsInterface = nullptr;
            VignetteComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ
