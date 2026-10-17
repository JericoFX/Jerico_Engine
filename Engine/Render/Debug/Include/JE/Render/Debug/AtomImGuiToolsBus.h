/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <JE/Render/Debug/RenderDebugExports.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <JE/Render/RPI/Public/MeshDrawPacket.h>

namespace JE::Render
{
    namespace RPI
    {
        class Material;
    }
}

namespace JE::Render::Debug
{
    //! AtomImGuiToolsRequestBus provides an interface to interact with Atom ImGui debug tools
    class JE_RENDER_DEBUG_API AtomImGuiToolsRequests : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Change which entity is displayed in the material shader details tool. If @autoOpenDialog is true, it will
        //! open the dialog if it was not already visible.
        virtual void ShowMaterialShaderDetailsForEntity(AZ::EntityId entity, bool autoOpenDialog) = 0;
    };
    using AtomImGuiToolsRequestBus = AZ::EBus<AtomImGuiToolsRequests>;
    
    class JE_RENDER_DEBUG_API AtomImGuiMeshCallbacks : public AZ::ComponentBus
    {
    public:
        virtual const JE::Render::RPI::MeshDrawPacketLods* GetDrawPackets() const = 0;
    };
    using AtomImGuiMeshCallbackBus = AZ::EBus<AtomImGuiMeshCallbacks>;

} 

AZ_DECLARE_EBUS_SINGLE_ADDRESS(JE_RENDER_DEBUG_API, JE::Render::Debug::AtomImGuiToolsRequests);
AZ_DECLARE_EBUS_MULTI_ADDRESS(JE_RENDER_DEBUG_API, JE::Render::Debug::AtomImGuiMeshCallbacks);
