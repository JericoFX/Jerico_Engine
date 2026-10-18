/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/Features/Mesh/MeshFeatureProcessorInterface.h>
#include <JE/Render/Features/Mesh/MeshInfo.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>


namespace JE::Render::Features
{
    //! Ebus to receive scene's notifications
    class MeshInfoNotification : public AZ::EBusTraits
    {
    public:
        // EBus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = JE::Render::RPI::SceneId;
        using MutexType = AZStd::mutex;

        //////////////////////////////////////////////////////////////////////////

        //! Notifies when a MeshInfo entry was allocated, but not yet initialized
        //! @param meshInfoHandle The MeshInfoIndex (as int32_t handle) of the new MeshInfo Entry
        virtual void OnAcquireMeshInfoEntry([[maybe_unused]] const MeshInfoHandle meshInfoHandle) {};

        //! Notifies when a MeshInfo is about to be deleted
        //! @param meshInfoHandle The MeshInfoIndex (as int32_t handle) of the released MeshInfo Entry
        virtual void OnReleaseMeshInfoEntry([[maybe_unused]] const MeshInfoHandle meshInfoHandle) {};

        //! Notifies when a newly acquired MeshInfo entry was filled with data (usually by the MeshFeatureProcessor)
        //! @param meshInfoHandle The MeshInfoIndex (as int32_t handle) of the new MeshInfo Entry
        //! @param meshHandle The MeshHandle for the mesh of the new MeshInfo Entry
        //! @param lodIndex The Index of the ModelLod of the mesh
        //! @param lodMeshIndex The Index of the Mesh in the ModelLod
        virtual void OnPopulateMeshInfoEntry(
            [[maybe_unused]] const MeshInfoHandle meshInfoHandle,
            [[maybe_unused]] ModelDataInstanceInterface* modelData,
            [[maybe_unused]] const size_t lodIndex,
            [[maybe_unused]] const size_t lodMeshIndex) {};
    };

    using MeshInfoNotificationBus = AZ::EBus<MeshInfoNotification>;

} // namespace JE::Render::Features
