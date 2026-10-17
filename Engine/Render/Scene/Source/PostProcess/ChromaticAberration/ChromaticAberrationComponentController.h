/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationConstants.h>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

#include <JE/Render/Scene/PostProcess/ChromaticAberration/ChromaticAberrationBus.h>
#include <JE/Render/Scene/PostProcess/ChromaticAberration/ChromaticAberrationComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>


namespace JE::Editor::Render::Scene
{
    class EditorChromaticAberrationComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class ChromaticAberrationComponentController final : public ChromaticAberrationRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorChromaticAberrationComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::ChromaticAberrationComponentController, "AZ::Render::ChromaticAberrationComponentController", "{776770B4-03BA-491D-BE5B-CBF3948BF078}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            ChromaticAberrationComponentController() = default;
            ChromaticAberrationComponentController(const ChromaticAberrationComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ChromaticAberrationComponentConfig& config);
            const ChromaticAberrationComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(ChromaticAberrationComponentController);

            void OnConfigChanged();

            AZ::Render::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            AZ::Render::ChromaticAberrationSettingsInterface* m_settingsInterface = nullptr;
            ChromaticAberrationComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ
