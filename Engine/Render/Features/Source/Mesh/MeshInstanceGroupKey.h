/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include<Atom/RHI/DeviceDrawItem.h>
#include<AzCore/Instance/InstanceId.h>

namespace JE::Render
{
    namespace Features
    {
        //! Represents all the data needed to know if a mesh can be instanced
        struct MeshInstanceGroupKey
        {
            AZ::Data::InstanceId m_modelId = AZ::Data::InstanceId::CreateFromAssetId(AZ::Data::AssetId(AZ::Uuid::CreateNull(), 0));
            uint32_t m_lodIndex = 0;
            uint32_t m_meshIndex = 0;
            AZ::Data::InstanceId m_materialId = AZ::Data::InstanceId::CreateFromAssetId(AZ::Data::AssetId(AZ::Uuid::CreateNull(), 0));
            // If anything needs to force instancing off (e.g., if the shader it uses doesn't support instancing),
            // it can set a random uuid here to force it to get a unique key
            AZ::Uuid m_forceInstancingOff = AZ::Uuid::CreateNull();
            AZ::RHI::DrawItemSortKey m_sortKey = 0;

            bool operator<(const MeshInstanceGroupKey& rhs) const;

            bool operator==(const MeshInstanceGroupKey& rhs) const;

            bool operator!=(const MeshInstanceGroupKey& rhs) const;

        private:
            auto MakeTie() const;
        };
    } // namespace Features
} // namespace JE::Render

namespace AZStd
{
    // hash specialization
    template<>
    struct hash<JE::Render::Features::MeshInstanceGroupKey>
    {
        using argument_type = JE::Render::Features::MeshInstanceGroupKey;
        using result_type = size_t;

        result_type operator()(const argument_type& key) const
        {
            size_t h = 0;
            hash_combine(h, key.m_modelId);
            hash_combine(h, key.m_lodIndex);
            hash_combine(h, key.m_meshIndex);
            hash_combine(h, key.m_materialId);
            hash_combine(h, key.m_forceInstancingOff);
            hash_combine(h, key.m_sortKey);
            return h;
        }
    };
} // namespace AZStd
