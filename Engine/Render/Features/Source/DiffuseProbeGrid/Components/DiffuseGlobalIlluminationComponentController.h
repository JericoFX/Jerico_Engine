/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/RenderFeaturesExports.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <JE/Render/Features/DiffuseProbeGrid/DiffuseGlobalIlluminationFeatureProcessorInterface.h>
#include <Components/DiffuseGlobalIlluminationComponentConfig.h>

namespace JE::Editor::Render::Features
{
    class EditorDiffuseGlobalIlluminationComponent;
}

namespace JE::Render
{
    namespace Features
    {
        class JE_RENDER_FEATURES_API DiffuseGlobalIlluminationComponentController final
        {
        public:
            friend class JE::Editor::Render::Features::EditorDiffuseGlobalIlluminationComponent;

            AZ_TYPE_INFO_WITH_NAME(DiffuseGlobalIlluminationComponentController, "AZ::Render::DiffuseGlobalIlluminationComponentController", "{7DE7D2A0-2526-447C-A11F-C31EE1332C26}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DiffuseGlobalIlluminationComponentController() = default;
            DiffuseGlobalIlluminationComponentController(const DiffuseGlobalIlluminationComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DiffuseGlobalIlluminationComponentConfig& config);
            const DiffuseGlobalIlluminationComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(DiffuseGlobalIlluminationComponentController);

            void OnConfigChanged();

            DiffuseGlobalIlluminationComponentConfig m_configuration;
            DiffuseGlobalIlluminationFeatureProcessorInterface* m_featureProcessor = nullptr;
        };
    }
}
