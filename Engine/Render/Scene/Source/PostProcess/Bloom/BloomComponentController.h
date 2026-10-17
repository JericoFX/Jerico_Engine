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

#include <JE/Render/Scene/PostProcess/Bloom/BloomBus.h>
#include <JE/Render/Scene/PostProcess/Bloom/BloomComponentConfig.h>

#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>
#include <Atom/Feature/PostProcess/Bloom/BloomSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace JE::Editor::Render::Scene
{
    class EditorBloomComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class BloomComponentController final
            : public BloomRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorBloomComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::BloomComponentController, "AZ::Render::BloomComponentController", "{502896C1-FF04-4BA7-833B-BA80946FA0DD}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            BloomComponentController() = default;
            BloomComponentController(const BloomComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const BloomComponentConfig& config);
            const BloomComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(BloomComponentController);

            void OnConfigChanged();

            AZ::Render::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            AZ::Render::BloomSettingsInterface* m_settingsInterface = nullptr;
            BloomComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    }
}
