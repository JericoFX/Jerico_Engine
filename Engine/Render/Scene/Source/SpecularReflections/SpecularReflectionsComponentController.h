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
#include <JE/Render/Features/SpecularReflections/SpecularReflectionsFeatureProcessorInterface.h>
#include <SpecularReflections/SpecularReflectionsComponentConfig.h>

namespace JE::Editor::Render::Scene
{
    class EditorSpecularReflectionsComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class SpecularReflectionsComponentController final
        {
        public:
            friend class JE::Editor::Render::Scene::EditorSpecularReflectionsComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::SpecularReflectionsComponentController, "AZ::Render::SpecularReflectionsComponentController", "{8ED8A722-AB11-4603-9C78-E882B544A7EF}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            SpecularReflectionsComponentController() = default;
            SpecularReflectionsComponentController(const SpecularReflectionsComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const SpecularReflectionsComponentConfig& config);
            const SpecularReflectionsComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(SpecularReflectionsComponentController);

            void OnConfigChanged();

            SpecularReflectionsComponentConfig m_configuration;
            JE::Render::Features::SpecularReflectionsFeatureProcessorInterface* m_featureProcessor = nullptr;
        };
    }
}
