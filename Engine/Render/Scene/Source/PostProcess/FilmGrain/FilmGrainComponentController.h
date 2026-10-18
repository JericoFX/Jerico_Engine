/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainConstants.h>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainSettingsInterface.h>
#include <JE/Render/Features/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <JE/Render/Features/PostProcess/PostProcessSettingsInterface.h>

#include <JE/Render/Scene/PostProcess/FilmGrain/FilmGrainBus.h>
#include <JE/Render/Scene/PostProcess/FilmGrain/FilmGrainComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>


namespace JE::Editor::Render::Scene
{
    class EditorFilmGrainComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class FilmGrainComponentController final : public FilmGrainRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorFilmGrainComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::FilmGrainComponentController, "AZ::Render::FilmGrainComponentController", "{732714F9-7CEF-4874-8614-0F5F50BBC6C1}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            FilmGrainComponentController() = default;
            FilmGrainComponentController(const FilmGrainComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const FilmGrainComponentConfig& config);
            const FilmGrainComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverride.inl>
#include <JE/Render/Features/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(FilmGrainComponentController);

            void OnConfigChanged();

            JE::Render::Features::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            JE::Render::Features::FilmGrainSettingsInterface* m_settingsInterface = nullptr;
            FilmGrainComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ
