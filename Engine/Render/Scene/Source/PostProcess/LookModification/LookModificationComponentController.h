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

#include <JE/Render/Scene/PostProcess/LookModification/LookModificationBus.h>
#include <JE/Render/Scene/PostProcess/LookModification/LookModificationComponentConfig.h>

#include <ACES/Aces.h>
#include <Atom/Feature/PostProcess/LookModification/LookModificationSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <AzCore/Asset/AssetCommon.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>

namespace JE::Editor::Render::Scene
{
    class EditorLookModificationComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class LookModificationComponentController final
            : public LookModificationRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorLookModificationComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::LookModificationComponentController, "AZ::Render::LookModificationComponentController", "{66912D19-CAB2-457C-A4EF-88FE4AF592B1}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            LookModificationComponentController() = default;
            LookModificationComponentController(const LookModificationComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const LookModificationComponentConfig& config);
            const LookModificationComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(LookModificationComponentController);

            void OnConfigChanged();

            AZ::Render::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            AZ::Render::LookModificationSettingsInterface* m_settingsInterface = nullptr;
            LookModificationComponentConfig m_configuration;
            AZ::EntityId m_entityId;
        };
    }
}
