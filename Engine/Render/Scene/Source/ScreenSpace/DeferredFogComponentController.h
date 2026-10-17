/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <JE/Render/Scene/ScreenSpace/DeferredFogBus.h>
#include <JE/Render/Scene/ScreenSpace/DeferredFogComponentConfig.h>

#include <Atom/Feature/ScreenSpace/DeferredFogSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

namespace JE::Editor::Render::Scene
{
    class EditorDeferredFogComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class DeferredFogComponentController final
            : public DeferredFogRequestsBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorDeferredFogComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::DeferredFogComponentController, "AZ::Render::DeferredFogComponentController", "{60B71D4C-1655-4C3A-BF14-CF6639B018CA}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DeferredFogComponentController() = default;
            DeferredFogComponentController(const DeferredFogComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DeferredFogComponentConfig& config);
            const DeferredFogComponentConfig& GetConfiguration() const;

            // Getter / Setter methods override declarations
            // Generate Get / Set methods
            
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType Get##Name() const override;                                                           \
        void Set##Name(ValueType val) override;                                                         \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(DeferredFogComponentController);

            void OnConfigChanged();

            AZ::Render::PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            AZ::Render::DeferredFogSettingsInterface* m_settingsInterface = nullptr;    // interface into the single setting structure
            DeferredFogComponentConfig m_configuration;     // configuration / settings per entity component - several can co-exist
            AZ::EntityId m_entityId;
        };
    }
}
