/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Draw/DrawableMeshEntity.h>

#include <JE/Render/RPI/Public/Model/Model.h>
#include <JE/Render/RPI/Public/Model/ModelLodUtils.h>
#include <JE/Render/RPI/Public/DynamicDraw/DynamicDrawInterface.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/ViewportContext.h>
#include <JE/Render/RPI/Public/ViewportContextBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AZ::Render
{
    // Gets the view for the specified scene
    static const JE::Render::RPI::ViewPtr GetViewFromScene(const JE::Render::RPI::Scene* scene)
    {
        const auto viewportContextRequests = JE::Render::RPI::ViewportContextRequests::Get();
        const auto viewportContext = viewportContextRequests->GetViewportContextByScene(scene);
        const JE::Render::RPI::ViewPtr viewPtr = viewportContext->GetDefaultView();
        return viewPtr;
    }

    // Utility class for common drawable data
    class DrawableMetaData
    {
    public:
        DrawableMetaData(EntityId entityId);
        JE::Render::RPI::Scene* GetScene() const;
        const JE::Render::RPI::ViewPtr GetView() const;
        const MeshFeatureProcessorInterface* GetFeatureProcessor() const;

    private:
        JE::Render::RPI::Scene* m_scene = nullptr;
        JE::Render::RPI::ViewPtr m_view = nullptr;
        MeshFeatureProcessorInterface* m_featureProcessor = nullptr;
    };

    DrawableMetaData::DrawableMetaData(EntityId entityId)
    {
        m_scene = JE::Render::RPI::Scene::GetSceneForEntityId(entityId);
        m_view = GetViewFromScene(m_scene);
        m_featureProcessor = m_scene->GetFeatureProcessor<MeshFeatureProcessorInterface>();
    }

    JE::Render::RPI::Scene* DrawableMetaData::GetScene() const
    {
        return m_scene;
    }

    const JE::Render::RPI::ViewPtr DrawableMetaData::GetView() const
    {
        return m_view;
    }

    const MeshFeatureProcessorInterface* DrawableMetaData::GetFeatureProcessor() const
    {
        return m_featureProcessor;
    }

    DrawableMeshEntity::DrawableMeshEntity(EntityId entityId, Data::Instance<JE::Render::RPI::Material> maskMaterial, Name drawList)
        : m_entityId(entityId)
        , m_maskMaterial(maskMaterial)
        , m_drawList(drawList)
    {
        JE::Render::Scene::MeshHandleStateNotificationBus::Handler::BusConnect(m_entityId);
    }

    DrawableMeshEntity::~DrawableMeshEntity()
    {
        JE::Render::Scene::MeshHandleStateNotificationBus::Handler::BusDisconnect();
    }

    void DrawableMeshEntity::ClearDrawData()
    {
        m_modelLodIndex = JE::Render::RPI::ModelLodIndex::Null;
        m_meshDrawPackets.clear();
    }

    bool DrawableMeshEntity::CanDraw() const
    {
        return !m_meshDrawPackets.empty();
    }

    void DrawableMeshEntity::Draw()
    {
        if (!CanDraw())
        {
            AZ_Warning(
                "EditorModeFeedbackSystemComponent",
                false,
                "Attempted to draw entity '%s' but entity has no draw data!",
                m_entityId.ToString().c_str());

            return;
        }

        const DrawableMetaData drawabaleMetaData(m_entityId);

        // If the mesh level of detail index has changed, rebuild the mesh draw packets with the new index
        if (!m_meshHandle || !m_meshHandle->IsValid())
        {
            return;
        }

        const auto model = drawabaleMetaData.GetFeatureProcessor()->GetModel(*m_meshHandle);

        if (!model)
        {
            return;
        }

        if (const auto modelLodIndex = GetModelLodIndex(drawabaleMetaData.GetView(), model);
            m_modelLodIndex != modelLodIndex)
        {
            CreateOrUpdateMeshDrawPackets(drawabaleMetaData.GetFeatureProcessor(), modelLodIndex, model);
        }

        JE::Render::RPI::DynamicDrawInterface* dynamicDraw = JE::Render::RPI::GetDynamicDraw();
        for (auto& drawPacket : m_meshDrawPackets)
        {
            drawPacket.Update(*drawabaleMetaData.GetScene());
            if (auto* rhiDrawPacket = drawPacket.GetRHIDrawPacket();
                rhiDrawPacket != nullptr)
            {
                dynamicDraw->AddDrawPacket(drawabaleMetaData.GetScene(), rhiDrawPacket);
            }
        }
    }

    JE::Render::RPI::ModelLodIndex DrawableMeshEntity::GetModelLodIndex(const JE::Render::RPI::ViewPtr view, Data::Instance<JE::Render::RPI::Model> model) const
    {
        const auto worldTM = AzToolsFramework::GetWorldTransform(m_entityId);
        return JE::Render::RPI::ModelLodUtils::SelectLod(view.get(), worldTM, *model);
    }

    void DrawableMeshEntity::OnMeshHandleSet(const MeshFeatureProcessorInterface::MeshHandle* meshHandle)
    {
        m_meshHandle = meshHandle;
        if (!m_meshHandle || !m_meshHandle->IsValid())
        {
            ClearDrawData();
            return;
        }

        const DrawableMetaData drawabaleMetaData(m_entityId);
        if (const auto model = drawabaleMetaData.GetFeatureProcessor()->GetModel(*m_meshHandle))
        {
            const auto modelLodIndex = GetModelLodIndex(drawabaleMetaData.GetView(), model);
            CreateOrUpdateMeshDrawPackets(drawabaleMetaData.GetFeatureProcessor(), modelLodIndex, model);
        }
    }

    void DrawableMeshEntity::CreateOrUpdateMeshDrawPackets(
        const MeshFeatureProcessorInterface* featureProcessor, const JE::Render::RPI::ModelLodIndex modelLodIndex, Data::Instance<JE::Render::RPI::Model> model)
    {
        ClearDrawData();

        if (!m_meshHandle || !m_meshHandle->IsValid())
        {
            return;
        }

        if (m_meshHandle->IsValid())
        {
            const auto maskMeshObjectSrg = CreateMaskShaderResourceGroup(featureProcessor);
            m_modelLodIndex = modelLodIndex;
            BuildMeshDrawPackets(model->GetModelAsset(), maskMeshObjectSrg);
        }
    }

    void DrawableMeshEntity::BuildMeshDrawPackets(
        const Data::Asset<JE::Render::RPI::ModelAsset> modelAsset, Data::Instance<JE::Render::RPI::ShaderResourceGroup> meshObjectSrg)
    {
        const auto modelLodAssets = modelAsset->GetLodAssets();
        const Data::Asset<JE::Render::RPI::ModelLodAsset>& modelLodAsset = modelLodAssets[m_modelLodIndex.m_index];
        Data::Instance<JE::Render::RPI::ModelLod> modelLod = JE::Render::RPI::ModelLod::FindOrCreate(modelLodAsset, modelAsset).get();

        for (size_t i = 0; i < modelLod->GetMeshes().size(); i++)
        {
            EditorStateMeshDrawPacket drawPacket(*modelLod, i, m_maskMaterial, m_drawList, meshObjectSrg);
            m_meshDrawPackets.emplace_back(drawPacket);
        }
    }

    Data::Instance<JE::Render::RPI::ShaderResourceGroup> DrawableMeshEntity::CreateMaskShaderResourceGroup(
        const MeshFeatureProcessorInterface* featureProcessor) const
    {
        const auto& shaderAsset = m_maskMaterial->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
        const auto& objectSrgLayout = m_maskMaterial->GetAsset()->GetObjectSrgLayout();
        const auto maskMeshObjectSrg = JE::Render::RPI::ShaderResourceGroup::Create(shaderAsset, objectSrgLayout->GetName());

        // Set the object id so the correct MVP matrices can be selected in the shader
        const auto objectId = featureProcessor->GetObjectId(*m_meshHandle).GetIndex();
        RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
        maskMeshObjectSrg->SetConstant(objectIdIndex, objectId);
        maskMeshObjectSrg->Compile();

        return maskMeshObjectSrg;
    }
} // namespace AZ::Render
