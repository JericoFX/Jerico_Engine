/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Model/ModelLodAsset.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/containers/unordered_map.h>

namespace JE::Render
{
    namespace RPI
    {
        void ModelSurfaceIdentity::Reflect(AZ::ReflectContext* context)
        {
            if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
            {
                sc->Class<ModelSurfaceIdentity>()->Version(1)
                    ->Field("SurfaceId", &ModelSurfaceIdentity::m_surfaceId)
                    ->Field("FaceId", &ModelSurfaceIdentity::m_faceId)
                    ->Field("RegionId", &ModelSurfaceIdentity::m_regionId);
            }
        }

        void ModelSurfaceIdentityTable::Reflect(AZ::ReflectContext* context)
        {
            ModelSurfaceIdentity::Reflect(context);
            if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
            {
                sc->Class<ModelSurfaceIdentityTable>()->Version(1)
                    ->Field("Schema", &ModelSurfaceIdentityTable::m_schema)
                    ->Field("Entries", &ModelSurfaceIdentityTable::m_entries);
            }
        }

        bool ModelSurfaceIdentityTable::Validate(size_t triangleCount) const
        {
            if (m_schema == 0) { return m_entries.empty(); }
            if (m_schema != Schema || triangleCount == 0 || m_entries.size() != triangleCount) { return false; }
            AZStd::unordered_map<AZ::Uuid, AZ::Uuid> ownership;
            for (const auto& entry : m_entries)
            {
                if (entry.m_surfaceId.IsNull() || entry.m_faceId.IsNull() ||
                    (entry.m_regionId && entry.m_regionId->IsNull())) { return false; }
                const auto inserted = ownership.emplace(entry.m_faceId, entry.m_surfaceId);
                if (!inserted.second && inserted.first->second != entry.m_surfaceId) { return false; }
            }
            return true;
        }

        bool ModelLodAsset::Mesh::ValidateSurfaceIdentityTable() const
        {
            if (m_surfaceIdentityTable.m_schema == 0 && m_surfaceIdentityTable.m_entries.empty()) { return true; }
            return GetIndexCount() % 3 == 0 && m_surfaceIdentityTable.Validate(GetIndexCount() / 3);
        }

        const ModelSurfaceIdentity* ModelLodAsset::Mesh::GetSurfaceIdentity(uint32_t primitiveIndex) const
        {
            const auto& table = m_surfaceIdentityTable;
            if (table.m_schema != ModelSurfaceIdentityTable::Schema || GetIndexCount() % 3 != 0 ||
                table.m_entries.size() != GetIndexCount() / 3 || primitiveIndex >= table.m_entries.size()) { return nullptr; }
            const auto& entry = table.m_entries[primitiveIndex];
            return entry.m_surfaceId.IsNull() || entry.m_faceId.IsNull() ||
                (entry.m_regionId && entry.m_regionId->IsNull()) ? nullptr : &entry;
        }

        bool ModelLodAsset::ValidateSurfaceIdentityTables() const
        {
            AZStd::unordered_map<AZ::Uuid, AZ::Uuid> ownership;
            for (const auto& mesh : m_meshes)
            {
                if (!mesh.ValidateSurfaceIdentityTable()) { return false; }
                for (const auto& entry : mesh.GetSurfaceIdentityTable().m_entries)
                {
                    const auto inserted = ownership.emplace(entry.m_faceId, entry.m_surfaceId);
                    if (!inserted.second && inserted.first->second != entry.m_surfaceId) { return false; }
                }
            }
            return true;
        }

        void ModelLodAsset::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelLodAsset>()
                    ->Version(0)
                    ->Field("Meshes", &ModelLodAsset::m_meshes)
                    ->Field("Aabb", &ModelLodAsset::m_aabb)
                    ->Field("StreamBuffers", &ModelLodAsset::m_streamBuffers)
                    ->Field("IndexBufferView", &ModelLodAsset::m_indexBuffer)
                    ;
            }

            Mesh::Reflect(context);
        }
        
        void ModelLodAsset::Mesh::Reflect(AZ::ReflectContext* context)
        {
            ModelSurfaceIdentityTable::Reflect(context);
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelLodAsset::Mesh>()
                    ->Version(2)
                    ->Field("Name", &ModelLodAsset::Mesh::m_name)
                    ->Field("AABB", &ModelLodAsset::Mesh::m_aabb)
                    ->Field("MaterialSlotId", &ModelLodAsset::Mesh::m_materialSlotId)
                    ->Field("IndexBufferAssetView", &ModelLodAsset::Mesh::m_indexBufferAssetView)
                    ->Field("StreamBufferInfo", &ModelLodAsset::Mesh::m_streamBufferInfo)
                    ->Field("SurfaceIdentityTable", &ModelLodAsset::Mesh::m_surfaceIdentityTable)
                    ;
            }

            StreamBufferInfo::Reflect(context);
        }

        void ModelLodAsset::Mesh::StreamBufferInfo::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ModelLodAsset::Mesh::StreamBufferInfo>()
                    ->Version(1)
                    ->Field("Semantic", &ModelLodAsset::Mesh::StreamBufferInfo::m_semantic)
                    ->Field("CustomName", &ModelLodAsset::Mesh::StreamBufferInfo::m_customName)
                    ->Field("BufferAssetView", &ModelLodAsset::Mesh::StreamBufferInfo::m_bufferAssetView)
                    ;
            }
        }

        uint32_t ModelLodAsset::Mesh::GetVertexCount() const
        {
            // Index 0 here is not special. All stream buffer views owned by this mesh should
            // view the same number of vertices. It doesn't make sense to be viewing 30 positions
            // but only 20 normals since we're using an index buffer model. 
            return m_streamBufferInfo[0].m_bufferAssetView.GetBufferViewDescriptor().m_elementCount;
        }

        uint32_t ModelLodAsset::Mesh::GetIndexCount() const
        {
            return m_indexBufferAssetView.GetBufferViewDescriptor().m_elementCount;
        }

        ModelMaterialSlot::StableId ModelLodAsset::Mesh::GetMaterialSlotId() const
        {
            return m_materialSlotId;
        }

        const AZ::Name& ModelLodAsset::Mesh::GetName() const
        {
            return m_name;
        }

        const AZ::Aabb& ModelLodAsset::Mesh::GetAabb() const
        {
            return m_aabb;
        }

        const BufferAssetView& ModelLodAsset::Mesh::GetIndexBufferAssetView() const
        {
            return m_indexBufferAssetView;
        }

        AZStd::span<const ModelLodAsset::Mesh::StreamBufferInfo> ModelLodAsset::Mesh::GetStreamBufferInfoList() const
        {
            return AZStd::span<const ModelLodAsset::Mesh::StreamBufferInfo>(m_streamBufferInfo);
        }

        void ModelLodAsset::AddMesh(const Mesh& mesh)
        {
            m_meshes.push_back(mesh);

            AZ::Aabb meshAabb = mesh.GetAabb();

            m_aabb.AddAabb(meshAabb);
        }

        AZStd::span<const ModelLodAsset::Mesh> ModelLodAsset::GetMeshes() const
        {
            return AZStd::span<const ModelLodAsset::Mesh>(m_meshes);
        }

        const AZ::Aabb& ModelLodAsset::GetAabb() const
        {
            return m_aabb;
        }
        
        const BufferAssetView* ModelLodAsset::GetSemanticBufferAssetView(const AZ::Name& semantic, uint32_t meshIndex) const
        {
            AZ_Assert(meshIndex < m_meshes.size(), "Mesh index out of range");
            return m_meshes[meshIndex].GetSemanticBufferAssetView(semantic);
        }

        void ModelLodAsset::LoadBufferAssets()
        {
            m_indexBuffer.QueueLoad();

            for (auto& streamBuffer : m_streamBuffers)
            {
                streamBuffer.QueueLoad();
            }

            m_indexBuffer.BlockUntilLoadComplete();
            for (auto& streamBuffer : m_streamBuffers)
            {
                streamBuffer.BlockUntilLoadComplete();
            }

            // update buffer asset references in meshes
            for (auto& mesh : m_meshes)
            {
                mesh.LoadBufferAssets();
            }
        }

        void ModelLodAsset::ReleaseBufferAssets()
        {
            m_indexBuffer.Release();

            for (auto& streamBuffer : m_streamBuffers)
            {
                streamBuffer.Release();
            }
             
            for (auto& mesh : m_meshes)
            {
                mesh.ReleaseBufferAssets();
            }
        }

        void ModelLodAsset::Mesh::LoadBufferAssets()
        {
            m_indexBufferAssetView.LoadBufferAsset();

            for (auto& bufferInfo : m_streamBufferInfo)
            {
                bufferInfo.m_bufferAssetView.LoadBufferAsset();
            }
        }
        
        void ModelLodAsset::Mesh::ReleaseBufferAssets()
        {
            m_indexBufferAssetView.ReleaseBufferAsset();

            for (auto& bufferInfo : m_streamBufferInfo)
            {
                bufferInfo.m_bufferAssetView.ReleaseBufferAsset();
            }
        }

        const BufferAssetView* ModelLodAsset::Mesh::GetSemanticBufferAssetView(const AZ::Name& semantic) const
        {
            const AZStd::span<const ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferList = GetStreamBufferInfoList();

            for (const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : streamBufferList)
            {
                if (streamBufferInfo.m_semantic.m_name == semantic)
                {
                    return &streamBufferInfo.m_bufferAssetView;
                }
            }

            return nullptr;
        }

        void ModelLodAsset::SetReady()
        {
            m_status = AZ::Data::AssetData::AssetStatus::Ready;
        }
    } // namespace RPI
} // namespace AZ
