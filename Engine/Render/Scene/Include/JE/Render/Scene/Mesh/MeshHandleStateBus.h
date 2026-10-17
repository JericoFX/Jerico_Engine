/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>

namespace JE::Render
{
    namespace Scene
    {
        //! Bus for retrieving data about a given entity's mesh handle state.
        class MeshHandleStateRequests
        : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;

            //! Returns the handle to the mesh.
            virtual const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* GetMeshHandle() const = 0;
        };

        using MeshHandleStateRequestBus = AZ::EBus<MeshHandleStateRequests>;
        
        //! Bus for receiving notifications about a given entity's mesh handle state.
        class MeshHandleStateNotifications
        : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;

            //! Notification for when the mesh handle has been set (and thus ready for use).
            virtual void OnMeshHandleSet(const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* meshHandle) = 0;

            //! When connecting to this bus, if the handle is ready you will immediately get an OnMeshHandleAcquire notification.
            template<class Bus>
            struct ConnectionPolicy
                : public AZ::EBusConnectionPolicy<Bus>
            {
                static void Connect(
                    typename Bus::BusPtr& busPtr,
                    typename Bus::Context& context,
                    typename Bus::HandlerNode& handler,
                    typename Bus::Context::ConnectLockGuard& connectLock,
                    const typename Bus::BusIdType& id = 0)
                {
                    AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                    // If this entity has no MeshHandleStateRequests handler treat this call as a no-op.
                    if(!MeshHandleStateRequestBus::HasHandlers(id))
                    {
                        return;
                    }

                    const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* meshHandle = nullptr;
                    MeshHandleStateRequestBus::EventResult(meshHandle, id, &MeshHandleStateRequestBus::Events::GetMeshHandle);

                    if (meshHandle && meshHandle->IsValid())
                    {
                        handler->OnMeshHandleSet(meshHandle);
                    }
                }
            };
        };

        using MeshHandleStateNotificationBus = AZ::EBus<MeshHandleStateNotifications>;
    } // namespace Render
} // namespace Render
