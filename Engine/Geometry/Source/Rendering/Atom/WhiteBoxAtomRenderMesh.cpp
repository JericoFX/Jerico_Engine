/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxAtomRenderMesh.h"

#include <Rendering/Atom/WhiteBoxMeshAtomData.h>
#include <Rendering/WhiteBoxRenderData.h>
#include <Util/WhiteBoxMathUtil.h>
#include <Viewport/WhiteBoxViewportConstants.h>

#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Render/RPI/Reflect/Model/ModelAssetCreator.h>
#include <JE/Render/RPI/Reflect/Model/ModelLodAssetCreator.h>
#include <JE/Render/RPI/Reflect/ResourcePoolAssetCreator.h>
#include <JE/Render/Materials/MaterialComponentBus.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/std/limits.h>

namespace JE::Geometry
{
    AtomRenderMesh::AtomRenderMesh(AZ::EntityId entityId)
        : m_entityId(entityId)
    {
        JE::Render::Scene::MeshHandleStateRequestBus::Handler::BusConnect(m_entityId);
        JE::Render::MaterialConsumerRequestBus::Handler::BusConnect(m_entityId);
        JE::Render::MaterialComponentNotificationBus::Handler::BusConnect(m_entityId);
    }

    AtomRenderMesh::~AtomRenderMesh()
    {
        m_materialInstance = {};
        if (m_meshHandle.IsValid() && m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
            JE::Render::Scene::MeshHandleStateNotificationBus::Event(
                m_entityId, &JE::Render::Scene::MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &m_meshHandle);
        }
        
        JE::Render::MaterialComponentNotificationBus::Handler::BusDisconnect();
        JE::Render::MaterialConsumerRequestBus::Handler::BusDisconnect();
        JE::Render::Scene::MeshHandleStateRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    bool AtomRenderMesh::AreAttributesValid() const
    {
        bool attributesAreValid = true;

        for (const auto& attribute : m_attributes)
        {
            AZStd::visit(
                [&attributesAreValid](const auto& att)
                {
                    if (!att->IsValid())
                    {
                        attributesAreValid = false;
                    }
                },
                attribute);
        }

        return attributesAreValid;
    }

    bool AtomRenderMesh::CreateMeshBuffers(const WhiteBoxMeshAtomData& meshData)
    {
        m_indexBuffer = AZStd::make_unique<IndexBuffer>(meshData.GetIndices());

        CreateAttributeBuffer<AttributeType::Position>(meshData.GetPositions());
        CreateAttributeBuffer<AttributeType::Normal>(meshData.GetNormals());
        CreateAttributeBuffer<AttributeType::Tangent>(meshData.GetTangents());
        CreateAttributeBuffer<AttributeType::Bitangent>(meshData.GetBitangents());
        CreateAttributeBuffer<AttributeType::UV>(meshData.GetUVs());
        CreateAttributeBuffer<AttributeType::Color>(meshData.GetColors());

        return AreAttributesValid();
    }

    bool AtomRenderMesh::UpdateMeshBuffers(const WhiteBoxMeshAtomData& meshData)
    {
        UpdateAttributeBuffer<AttributeType::Position>(meshData.GetPositions());
        UpdateAttributeBuffer<AttributeType::Normal>(meshData.GetNormals());
        UpdateAttributeBuffer<AttributeType::Tangent>(meshData.GetTangents());
        UpdateAttributeBuffer<AttributeType::Bitangent>(meshData.GetBitangents());
        UpdateAttributeBuffer<AttributeType::UV>(meshData.GetUVs());
        UpdateAttributeBuffer<AttributeType::Color>(meshData.GetColors());

        return AreAttributesValid();
    }

    void AtomRenderMesh::AddLodBuffers(JE::Render::RPI::ModelLodAssetCreator& modelLodCreator)
    {
        modelLodCreator.SetLodIndexBuffer(m_indexBuffer->GetBuffer());

        for (auto& attribute : m_attributes)
        {
            AZStd::visit(
                [&modelLodCreator](auto& att)
                {
                    att->AddLodStreamBuffer(modelLodCreator);
                },
                attribute);
        }
    }

    void AtomRenderMesh::AddMeshBuffers(JE::Render::RPI::ModelLodAssetCreator& modelLodCreator, const JE::BSP::MaterialIndexRange& range)
    {
        const auto indexView = AZ::RHI::BufferViewDescriptor::CreateTyped(
            range.m_firstIndex, range.m_indexCount, AZ::RHI::Format::R32_UINT);
        modelLodCreator.SetMeshIndexBuffer(JE::Render::RPI::BufferAssetView{m_indexBuffer->GetBuffer(), indexView});

        for (auto& attribute : m_attributes)
        {
            AZStd::visit(
                [&modelLodCreator](auto&& att)
                {
                    att->AddMeshStreamBuffer(modelLodCreator);
                },
                attribute);
        }
    }

    bool AtomRenderMesh::CreateLodAsset(const WhiteBoxMeshAtomData& meshData)
    {
        if (!CreateMeshBuffers(meshData))
        {
            return false;
        }

        JE::Render::RPI::ModelLodAssetCreator modelLodCreator;
        modelLodCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        AddLodBuffers(modelLodCreator);
        for (const auto& range : m_materialRanges)
        {
            modelLodCreator.BeginMesh();
            modelLodCreator.SetMeshAabb(meshData.GetAabb());
            modelLodCreator.SetMeshMaterialSlot(range.m_materialSlot);
            // Submeshes share the same position, normal, tangent and exact corner-UV buffers.
            AddMeshBuffers(modelLodCreator, range);
            modelLodCreator.EndMesh();
        }

        if (!modelLodCreator.End(m_lodAsset))
        {
            AZ_Error("CreateLodAsset", false, "Couldn't create LoD asset.");
            return false;
        }

        if (!m_lodAsset.IsReady())
        {
            AZ_Error("CreateLodAsset", false, "LoD asset is not ready.");
            return false;
        }

        if (!m_lodAsset.Get())
        {
            AZ_Error("CreateLodAsset", false, "LoD asset is nullptr.");
            return false;
        }

        return true;
    }

    void AtomRenderMesh::CreateModelAsset()
    {
        JE::Render::RPI::ModelAssetCreator modelCreator;
        modelCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        modelCreator.SetName(ModelName);
        modelCreator.AddLodAsset(AZStd::move(m_lodAsset));
        
        if (auto materialAsset = JE::Render::RPI::AssetUtils::LoadAssetByProductPath<JE::Render::RPI::MaterialAsset>(TexturedMaterialPath.data()))
        {
            // The fallback instance is private to this entity; native MaterialComponent owns overrides.
            m_materialInstance = JE::Render::RPI::Material::Create(materialAsset);
            for (const auto& range : m_materialRanges)
            {
                JE::Render::RPI::ModelMaterialSlot materialSlot;
                materialSlot.m_stableId = range.m_materialSlot;
                materialSlot.m_displayName = AZ::Name(AZStd::string::format("Surface slot %u", range.m_materialSlot));
                materialSlot.m_defaultMaterialAsset = materialAsset;
                modelCreator.AddMaterialSlot(materialSlot);
            }
        }
        else
        {
            AZ_Error("CreateLodAsset", false, "Could not load material.");
            return;
        }

        modelCreator.End(m_modelAsset);
    }

    bool AtomRenderMesh::CreateModel()
    {
        m_model = JE::Render::RPI::Model::FindOrCreate(m_modelAsset);
        m_meshFeatureProcessor =
            JE::Render::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::MeshFeatureProcessorInterface>(m_entityId);

        if (!m_meshFeatureProcessor)
        {
            AZ_Error(
                "MeshComponentController", m_meshFeatureProcessor,
                "Unable to find a MeshFeatureProcessorInterface on the entityId.");
            return false;
        }

        m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
        m_meshHandle = m_meshFeatureProcessor->AcquireMesh(AZ::Render::MeshHandleDescriptor(m_modelAsset));
        JE::Render::MaterialConsumerNotificationBus::Event(
            m_entityId, &JE::Render::MaterialConsumerNotifications::OnMaterialAssignmentSlotsChanged);
        JE::Render::MaterialAssignmentMap materials;
        JE::Render::MaterialComponentRequestBus::EventResult(
            materials, m_entityId, &JE::Render::MaterialComponentRequests::GetMaterialMapCopy);
        ApplyNativeMaterials(materials);
        JE::Render::Scene::MeshHandleStateNotificationBus::Event(m_entityId, &JE::Render::Scene::MeshHandleStateNotificationBus::Events::OnMeshHandleSet, &m_meshHandle);

        return true;
    }

    bool AtomRenderMesh::MeshRequiresFullRebuild([[maybe_unused]] const WhiteBoxMeshAtomData& meshData) const
    {
        return meshData.VertexCount() != m_vertexCount;
    }

    bool AtomRenderMesh::CreateMesh(const WhiteBoxMeshAtomData& meshData)
    {
        if (!CreateLodAsset(meshData))
        {
            return false;
        }

        CreateModelAsset();

        if (!CreateModel())
        {
            return false;
        }

        m_vertexCount = meshData.VertexCount();

        return true;
    }

    bool AtomRenderMesh::DoesMeshRequireFullRebuild([[maybe_unused]] const WhiteBoxMeshAtomData& meshData) const
    {
        // this has been disabled due to a some recent updates with Atom that a) cause visual artefacts
        // when updating the buffers and b) have a big performance boost when rebuilding from scratch anyway.
        //
        // this method for building the mesh will probably be replace anyway when the Atom DynamicDraw support
        // comes online.
        return true; // meshData.VertexCount() != m_vertexCount;
    }

    void AtomRenderMesh::BuildMesh(const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal)
    {
        WhiteBoxFaces culledFaceList = BuildCulledWhiteBoxFaces(renderData.m_faces);
        if (culledFaceList.size() > AZStd::numeric_limits<uint32_t>::max() / 3)
        {
            AZ_Error("WhiteBox", false, "White Box material partition exceeds the Atom index range.");
            return;
        }
        auto materialRanges = JE::BSP::PartitionFacesByMaterial(culledFaceList);
        if (materialRanges.empty()) { return; }
        m_materialRanges = AZStd::move(materialRanges);
        const WhiteBoxMeshAtomData meshData(culledFaceList);

        if (DoesMeshRequireFullRebuild(meshData))
        {
            if (!CreateMesh(meshData))
            {
                return;
            }
        }
        else
        {
            if (!UpdateMeshBuffers(meshData))
            {
                return;
            }
        }

        UpdateTransform(worldFromLocal);
    }

    void AtomRenderMesh::UpdateTransform(const AZ::Transform& worldFromLocal)
    {
        m_meshFeatureProcessor->SetTransform(m_meshHandle, worldFromLocal);
    }

    void AtomRenderMesh::UpdateMaterial(const WhiteBoxMaterial& material)
    {
        if (m_meshFeatureProcessor && m_materialInstance)
        {            
            if (const auto& materialPropertyIndex = m_materialInstance->FindPropertyIndex(AZ::Name("baseColor.color"));
                materialPropertyIndex.IsValid())
            {
                m_materialInstance->SetPropertyValue(materialPropertyIndex, AZ::Color(material.m_tint));
            }

            if (const auto& materialPropertyIndex = m_materialInstance->FindPropertyIndex(AZ::Name("baseColor.useTexture"));
                materialPropertyIndex.IsValid())
            {
                m_materialInstance->SetPropertyValue(materialPropertyIndex, material.m_useTexture);
            }

            JE::Render::MaterialAssignmentMap materials;
            JE::Render::MaterialComponentRequestBus::EventResult(
                materials, m_entityId, &JE::Render::MaterialComponentRequests::GetMaterialMapCopy);
            ApplyNativeMaterials(materials);

            // If the material changes were successfully applied then disconnect from the tick bus. Otherwise, make another attempt on the next tick.
            if (!m_materialInstance->NeedsCompile() || m_materialInstance->Compile())
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
            else if (!AZ::TickBus::Handler::BusIsConnected())
            {
                AZ::TickBus::Handler::BusConnect();
            }
        }
    }

    JE::Render::MaterialAssignmentId AtomRenderMesh::FindMaterialAssignmentId(
        const JE::Render::MaterialAssignmentLodIndex lod, const AZStd::string& label) const
    {
        return JE::Render::GetMaterialSlotIdFromModelAsset(m_modelAsset, lod, label);
    }

    JE::Render::MaterialAssignmentLabelMap AtomRenderMesh::GetMaterialLabels() const
    {
        return JE::Render::GetMaterialSlotLabelsFromModelAsset(m_modelAsset);
    }

    JE::Render::MaterialAssignmentMap AtomRenderMesh::GetDefaultMaterialMap() const
    {
        return JE::Render::GetDefaultMaterialMapFromModelAsset(m_modelAsset);
    }

    AZStd::unordered_set<AZ::Name> AtomRenderMesh::GetModelUvNames() const
    {
        return m_model ? m_model->GetUvNames() : AZStd::unordered_set<AZ::Name>{};
    }

    void AtomRenderMesh::ApplyNativeMaterials(const JE::Render::MaterialAssignmentMap& materials)
    {
        if (!m_meshFeatureProcessor || !m_meshHandle.IsValid()) { return; }
        auto customMaterials = JE::Render::ConvertToCustomMaterialMap(materials);
        // Keep only the historical fallback on slots not covered by native Atom instances.
        for (const auto& range : m_materialRanges)
        {
            const auto& assignment = JE::Render::GetMaterialAssignmentFromMapWithFallback(
                materials, JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(0, range.m_materialSlot));
            if (!assignment.m_materialInstance && m_materialInstance)
            {
                customMaterials[{0, range.m_materialSlot}].m_material = m_materialInstance;
            }
        }
        m_meshFeatureProcessor->SetCustomMaterials(m_meshHandle, customMaterials);
    }

    void AtomRenderMesh::OnMaterialsUpdated(const JE::Render::MaterialAssignmentMap& materials)
    {
        ApplyNativeMaterials(materials);
    }

    void AtomRenderMesh::OnMaterialPropertiesUpdated([[maybe_unused]] const JE::Render::MaterialAssignmentMap& materials)
    {
        if (m_meshFeatureProcessor && m_meshHandle.IsValid()) { m_meshFeatureProcessor->SetRayTracingDirty(m_meshHandle); }
    }
    void AtomRenderMesh::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_materialInstance || !m_materialInstance->NeedsCompile() || m_materialInstance->Compile())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void AtomRenderMesh::SetVisiblity(bool visibility)
    {
        m_visible = visibility;
        m_meshFeatureProcessor->SetVisible(m_meshHandle, m_visible);
    }

    bool AtomRenderMesh::IsVisible() const
    {
        return m_visible;
    }

    const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* AtomRenderMesh::GetMeshHandle() const
    {
        return &m_meshHandle;
    }
} // namespace JE::Geometry
