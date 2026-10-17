/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Scripting/EntityReferenceComponentController.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace JE::Render
{
    namespace Scene
    {
        void EntityReferenceComponentController::Reflect(AZ::ReflectContext* context)
        {
            EntityReferenceComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EntityReferenceComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &EntityReferenceComponentController::m_configuration);
            }
        }

        void EntityReferenceComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("EntityReferenceService"));
        }

        void EntityReferenceComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EntityReferenceService"));
        }

        EntityReferenceComponentController::EntityReferenceComponentController(const EntityReferenceComponentConfig& config)
            : m_configuration(config)
        {

        }

        void EntityReferenceComponentController::Activate(AZ::EntityId entityId)
        {
            m_entityId = entityId;

            EntityReferenceRequestBus::Handler::BusConnect(m_entityId);
        }

        void EntityReferenceComponentController::Deactivate()
        {
            EntityReferenceRequestBus::Handler::BusDisconnect(m_entityId);

            m_entityId.SetInvalid();
        }

        void EntityReferenceComponentController::SetConfiguration(const EntityReferenceComponentConfig& config)
        {
            m_configuration = config;
        }

        AZStd::vector<AZ::EntityId> EntityReferenceComponentController::GetEntityReferences() const
        {
            return m_configuration.m_entityIdReferences;
        }
    }
}
