/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Scene/Debug/RenderDebugBus.h>
#include <JE/Render/Scene/Debug/RenderDebugComponentConfig.h>

#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>
#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>
#include <Atom/Feature/Debug/RenderDebugFeatureProcessorInterface.h>

namespace JE::Editor::Render::Scene
{
    class RenderDebugEditorComponent;
}

namespace JE::Render::Scene
{
    class RenderDebugComponentController final
        : public RenderDebugRequestBus::Handler
    {
    public:
        friend class JE::Editor::Render::Scene::RenderDebugEditorComponent;

        AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::RenderDebugComponentController, "AZ::Render::RenderDebugComponentController", "{365E4B90-7145-4803-B990-B6D3E0C4B80B}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        RenderDebugComponentController() = default;
        RenderDebugComponentController(const RenderDebugComponentConfig& config);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void SetConfiguration(const RenderDebugComponentConfig& config);
        const RenderDebugComponentConfig& GetConfiguration() const;

        // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

    private:
        AZ_DISABLE_COPY(RenderDebugComponentController);

        void OnConfigChanged();

        AZ::Render::RenderDebugSettingsInterface* m_renderDebugSettingsInterface = nullptr;
        RenderDebugComponentConfig m_configuration;
        AZ::EntityId m_entityId;
    };

}
