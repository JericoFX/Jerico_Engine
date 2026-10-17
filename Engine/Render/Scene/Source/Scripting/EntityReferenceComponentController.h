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
#include <JE/Render/Scene/Scripting/EntityReferenceRequestBus.h>
#include <JE/Render/Scene/Scripting/EntityReferenceComponentConfig.h>

namespace JE::Editor::Render::Scene
{
    class EditorEntityReferenceComponent;
}

namespace JE::Render
{
    namespace Scene
    {
        class EntityReferenceComponentController final
            : public EntityReferenceRequestBus::Handler
        {
        public:
            friend class JE::Editor::Render::Scene::EditorEntityReferenceComponent;

            AZ_TYPE_INFO_WITH_NAME(JE::Render::Scene::EntityReferenceComponentController, "AZ::Render::EntityReferenceComponentController", "{89D1D8DE-AC1F-4069-8884-5A04582C2EB1}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            EntityReferenceComponentController() = default;
            EntityReferenceComponentController(const EntityReferenceComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const EntityReferenceComponentConfig& config);
            const EntityReferenceComponentConfig& GetConfiguration() const { return m_configuration; }

            // EntityReferenceRequestBus Overrides
            virtual AZStd::vector<AZ::EntityId> GetEntityReferences() const override;
        private:
            AZ_DISABLE_COPY(EntityReferenceComponentController);

            EntityReferenceComponentConfig m_configuration;

            AZ::EntityId m_entityId;
        };
    }
}
