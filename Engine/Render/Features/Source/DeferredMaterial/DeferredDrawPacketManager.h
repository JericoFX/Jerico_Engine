/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <DeferredMaterial/DeferredDrawPacket.h>

namespace JE::Render::Features
{
    // the DeferredDrawPacketManager holds all DeferredDrawPackets of the scene, and deduplicates
    // the Drawpackets from the Meshes
    class DeferredDrawPacketManager
    {
    public:
        using DeferredDrawPacketId = uint32_t;

        static auto CalculateDrawPacketId(const JE::Render::RPI::Material* material, const JE::Render::RPI::ShaderCollection::Item& shaderItem)
            -> DeferredDrawPacketId;

        auto GetDeferredDrawPacket(DeferredDrawPacketId id) const -> AZ::Data::Instance<DeferredDrawPacket>;

        bool HasDeferredDrawPacket(DeferredDrawPacketId id) const;

        auto GetOrCreateDeferredDrawPacket(
            JE::Render::RPI::Scene* scene, JE::Render::RPI::Material* material, const AZ::Name materialPipelineName, const JE::Render::RPI::ShaderCollection::Item& shaderItem)
            -> AZ::Data::Instance<DeferredDrawPacket>;

        bool HasDrawPacketForDrawList(const AZ::RHI::DrawListTag tag) const;

        auto GetDrawPackets() -> AZStd::unordered_map<DeferredDrawPacketId, AZ::Data::Instance<DeferredDrawPacket>>&
        {
            return m_deferredDrawPackets;
        }
        void SetNeedsUpdate(const bool needsUpdate)
        {
            m_needsUpdate = needsUpdate;
        }
        bool GetNeedsUpdate() const
        {
            return m_needsUpdate;
        }

        void PruneUnusedDrawPackets();

        AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> CreatePassSrg(AZ::RHI::DrawListTag drawListTag);

    private:
        AZStd::unordered_map<DeferredDrawPacketId, AZ::Data::Instance<DeferredDrawPacket>> m_deferredDrawPackets;
        AZ::RHI::DrawListMask m_drawListsWithDrawPackets;
        bool m_needsUpdate = false;
    };
} // namespace JE::Render::Features