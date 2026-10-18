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

#include <JE/Render/Scene/PostProcess/AmbientOcclusion/AoBus.h>
#include <JE/Render/Scene/PostProcess/AmbientOcclusion/AoComponentConfiguration.h>

#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoSettingsInterface.h>
#include <JE/Render/Features/PostProcess/PostProcessSettingsInterface.h>
#include <JE/Render/Features/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace JE::Editor::Render::Scene
{
    class EditorAoComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class AoComponentController final
            : public AoRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorAoComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::AoComponentController, "AZ::Render::AoComponentController", "{B53B6F29-C803-46AD-83E1-526457BDFBAE}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            AoComponentController() = default;
            AoComponentController(const AoComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const AoComponentConfig& config);
            const AoComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <JE/Render/Features/ParamMacros/StartParamFunctionsOverride.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/AoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/SsaoParams.inl>
#include <JE/Render/Features/PostProcess/AmbientOcclusion/GtaoParams.inl>

#include <JE/Render/Features/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(AoComponentController);

            void OnConfigChanged();

            JE::Render::Features::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            JE::Render::Features::AoSettingsInterface* m_ssaoSettingsInterface = nullptr;
            AoComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    }
}
