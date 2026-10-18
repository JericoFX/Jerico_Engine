/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Scene/Debug/RayTracingDebugBus.h>
#include <JE/Render/Scene/Debug/RayTracingDebugComponentConfig.h>

namespace JE::Editor::Render::Scene
{
    class RayTracingDebugEditorComponent;
}

namespace JE::Render::Scene
{
    class RayTracingDebugComponentController final : public RayTracingDebugRequestBus::Handler
    {
        friend class JE::Editor::Render::Scene::RayTracingDebugEditorComponent;

    public:
        AZ_TYPE_INFO(RayTracingDebugComponentController, "{7B1CAB96-6B9E-46C4-BBDE-B140E8082CEB}");
        static void Reflect(AZ::ReflectContext* context);

        RayTracingDebugComponentController() = default;
        RayTracingDebugComponentController(const RayTracingDebugComponentConfig& config);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void SetConfiguration(const RayTracingDebugComponentConfig& config);
        const RayTracingDebugComponentConfig& GetConfiguration() const;

        // clang-format off
        // Generate function override declarations (functions definitions in .cpp)
        #include <JE/Render/Features/ParamMacros/StartParamFunctionsOverride.inl>
        #include <JE/Render/Features/Debug/RayTracingDebugParams.inl>
        #include <JE/Render/Features/ParamMacros/EndParams.inl>
        // clang-format on

    private:
        void OnConfigurationChanged();
        bool IsRayTracingSupported() const;
        bool IsRayTracingNotSupported() const;

        JE::Render::Features::RayTracingDebugSettingsInterface* m_rayTracingDebugSettingsInterface{ nullptr };
        AZ::EntityId m_entityId{ AZ::EntityId::InvalidEntityId };
        RayTracingDebugComponentConfig m_configuration;
    };
} // namespace AZ::Render
