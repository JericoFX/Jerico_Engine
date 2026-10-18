
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Model/ModelLod.h>
#include <DeferredMaterial/DeferredDrawPacket.h>
#include <DeferredMaterial/DeferredDrawPacketManager.h>

namespace JE::Render::Features
{
    // the DeferredMeshDrawPacket is not a draw-packet as such, but it holds a reference to the actual
    // deferred Drawpacket for the Material, since that is generally shared between multiple meshes
    // We use the reference count to figure out if the DeferredDrawPacket is still needed.
    class DeferredMeshDrawPacket
    {
    public:
        DeferredMeshDrawPacket() = default;
        DeferredMeshDrawPacket(
            AZ::Data::Instance<JE::Render::RPI::ModelLod> modelLod, size_t modelLodMeshIndex, AZ::Data::Instance<JE::Render::RPI::Material> materialOverride);

        ~DeferredMeshDrawPacket();

        // disable copy
        DeferredMeshDrawPacket(const DeferredMeshDrawPacket&) = delete;
        DeferredMeshDrawPacket& operator=(const DeferredMeshDrawPacket&) = delete;

        // allow move
        DeferredMeshDrawPacket(DeferredMeshDrawPacket&&) = default;
        DeferredMeshDrawPacket& operator=(DeferredMeshDrawPacket&&) = default;

        auto GetDeferredDrawPacket(const AZ::RHI::DrawListTag& drawList) -> AZ::Data::Instance<DeferredDrawPacket>;

        void Update(JE::Render::RPI::Scene* scene, DeferredDrawPacketManager* manager, bool forceRebuild);

    private:
        // a mesh can have a DeferredDrawPacket for multiple DrawListTags
        AZStd::unordered_map<AZ::RHI::DrawListTag, AZ::Data::Instance<DeferredDrawPacket>> m_deferredDrawPackets;
        AZ::Data::Instance<JE::Render::RPI::ModelLod> m_modelLod;
        size_t m_modelLodMeshIndex;
        AZ::Data::Instance<JE::Render::RPI::Material> m_material;
        // Tracks whether the Material has change since the DrawPacket was last built
        JE::Render::RPI::Material::ChangeId m_materialChangeId = JE::Render::RPI::Material::DEFAULT_CHANGE_ID;
    };

} // namespace JE::Render::Features
