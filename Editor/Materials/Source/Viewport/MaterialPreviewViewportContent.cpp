/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/Features/SkyBox/SkyboxConstants.h>
#include <JE/Render/Features/Utils/ModelPreset.h>
#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Render/Scene/Grid/GridComponentBus.h>
#include <JE/Render/Scene/Grid/GridComponentConfig.h>
#include <JE/Render/Scene/Grid/GridComponentConstants.h>
#include <JE/Render/Scene/ImageBasedLights/ImageBasedLightComponentBus.h>
#include <JE/Render/Scene/ImageBasedLights/ImageBasedLightComponentConstants.h>
#include <JE/Render/Materials/MaterialComponentBus.h>
#include <JE/Render/Materials/MaterialComponentConstants.h>
#include <JE/Render/Scene/Mesh/MeshComponentBus.h>
#include <JE/Render/Scene/Mesh/MeshComponentConstants.h>
#include <JE/Render/Scene/PostProcess/DisplayMapper/DisplayMapperComponentBus.h>
#include <JE/Render/Scene/PostProcess/DisplayMapper/DisplayMapperComponentConstants.h>
#include <JE/Render/Scene/PostProcess/ExposureControl/ExposureControlBus.h>
#include <JE/Render/Scene/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <JE/Render/Scene/PostProcess/PostFxLayerComponentConstants.h>
#include <JE/Render/Scene/SkyBox/HDRiSkyboxBus.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentRequestBus.h>
#include <JE/Editor/Tools/Graph/GraphDocumentRequestBus.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <JE/Editor/Materials/Document/MaterialDocumentRequestBus.h>
#include <JE/Editor/Materials/Viewport/MaterialPreviewViewportContent.h>

namespace JE::Editor
{
    MaterialPreviewViewportContent::MaterialPreviewViewportContent(
        const AZ::Crc32& toolId,
        JE::Editor::Tools::RenderViewportWidget* widget,
        AZStd::shared_ptr<AzFramework::EntityContext> entityContext, bool followOpenedDocuments)
        : JE::Editor::Tools::EntityPreviewViewportContent(toolId, widget, entityContext)
        , m_followOpenedDocuments(followOpenedDocuments)
    {
        // Configure tone mapper
        m_postFxEntity = CreateEntity(
            "PostFxEntity",
            { JE::Render::Scene::PostFxLayerComponentTypeId, JE::Render::Scene::DisplayMapperComponentTypeId, JE::Render::Scene::ExposureControlComponentTypeId,
              azrtti_typeid<AzFramework::TransformComponent>() });

        // Create IBL
        m_environmentEntity = CreateEntity(
            "EnvironmentEntity",
            { JE::Render::Features::HDRiSkyboxComponentTypeId, JE::Render::Scene::ImageBasedLightComponentTypeId,
              azrtti_typeid<AzFramework::TransformComponent>() });

        // Create model
        m_objectEntity = CreateEntity(
            "ObjectEntity",
            { JE::Render::Scene::MeshComponentTypeId, JE::Render::MaterialComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>() });

        // Create shadow catcher
        m_shadowCatcherEntity = CreateEntity(
            "ShadowCatcherEntity",
            { JE::Render::Scene::MeshComponentTypeId, JE::Render::MaterialComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>(),
              azrtti_typeid<AzFramework::NonUniformScaleComponent>() });

        AZ::NonUniformScaleRequestBus::Event(
            GetShadowCatcherEntityId(), &AZ::NonUniformScaleRequests::SetScale, AZ::Vector3(100, 100, 1.0));

        // Avoid z-fighting with the cube model when double-sided rendering is enabled
        AZ::TransformBus::Event(
            GetShadowCatcherEntityId(), &AZ::TransformInterface::SetWorldZ, -0.01f);

        JE::Render::Scene::MeshComponentRequestBus::Event(
            GetShadowCatcherEntityId(), &JE::Render::Scene::MeshComponentRequestBus::Events::SetModelAssetId,
            JE::Render::RPI::AssetUtils::GetAssetIdForProductPath("materialeditor/viewportmodels/plane_1x1.fbx.azmodel"));

        JE::Render::MaterialComponentRequestBus::Event(
            GetShadowCatcherEntityId(), &JE::Render::MaterialComponentRequestBus::Events::SetMaterialAssetId,
            JE::Render::DefaultMaterialAssignmentId,
            JE::Render::RPI::AssetUtils::GetAssetIdForProductPath("materials/special/shadowcatcher.azmaterial"));

        // Create grid
        m_gridEntity = CreateEntity("GridEntity", { JE::Render::Scene::GridComponentTypeId, azrtti_typeid<AzFramework::TransformComponent>() });

        JE::Render::Scene::GridComponentRequestBus::Event(
            GetGridEntityId(),
            [&](JE::Render::Scene::GridComponentRequests* gridComponentRequests)
            {
                gridComponentRequests->SetSize(4.0f);
                gridComponentRequests->SetAxisColor(AZ::Color(0.1f, 0.1f, 0.1f, 1.0f));
                gridComponentRequests->SetPrimaryColor(AZ::Color(0.1f, 0.1f, 0.1f, 1.0f));
                gridComponentRequests->SetSecondaryColor(AZ::Color(0.1f, 0.1f, 0.1f, 1.0f));
            });

        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
        OnDocumentOpened(AZ::Uuid::CreateNull());
        OnViewportSettingsChanged();
    }

    MaterialPreviewViewportContent::~MaterialPreviewViewportContent()
    {
        SetDocumentId(AZ::Uuid::CreateNull());
        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    AZ::EntityId MaterialPreviewViewportContent::GetObjectEntityId() const
    {
        return m_objectEntity ? m_objectEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId MaterialPreviewViewportContent::GetEnvironmentEntityId() const
    {
        return m_environmentEntity ? m_environmentEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId MaterialPreviewViewportContent::GetPostFxEntityId() const
    {
        return m_postFxEntity ? m_postFxEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId MaterialPreviewViewportContent::GetShadowCatcherEntityId() const
    {
        return m_shadowCatcherEntity ? m_shadowCatcherEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId MaterialPreviewViewportContent::GetGridEntityId() const
    {
        return m_gridEntity ? m_gridEntity->GetId() : AZ::EntityId();
    }

    void MaterialPreviewViewportContent::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        if (m_followOpenedDocuments)
        {
            SetDocumentId(documentId);
        }
    }

    void MaterialPreviewViewportContent::SetDocumentId(const AZ::Uuid& documentId)
    {
        JE::Editor::Tools::GraphDocumentNotificationBus::Handler::BusDisconnect();
        m_graphDocumentId = AZ::Uuid::CreateNull();
        AZ::Data::Instance<JE::Render::RPI::Material> materialInstance;
        JE::Editor::MaterialDocumentRequestBus::EventResult(
            materialInstance, documentId, &JE::Editor::MaterialDocumentRequestBus::Events::GetInstance);

        JE::Render::MaterialAssignmentMap materials;
        auto& materialAssignment = materials[JE::Render::DefaultMaterialAssignmentId];
        materialAssignment.m_materialInstance = materialInstance;
        materialAssignment.m_materialInstancePreCreated = true;

        JE::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &JE::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materials);
    }

    void MaterialPreviewViewportContent::SetGraphDocumentId(const AZ::Uuid& documentId)
    {
        JE::Editor::Tools::DocumentTypeInfo type;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            type, documentId, &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        const AZ::Uuid selectedId = type.m_documentTypeName == "Material Graph" ? documentId : AZ::Uuid::CreateNull();
        if (!selectedId.IsNull() && selectedId == m_graphDocumentId)
        {
            return;
        }
        JE::Editor::Tools::GraphDocumentNotificationBus::Handler::BusDisconnect();
        m_graphDocumentId = selectedId;
        if (!m_graphDocumentId.IsNull())
        {
            JE::Editor::Tools::GraphDocumentNotificationBus::Handler::BusConnect(m_toolId);
        }
        ApplyGeneratedMaterial(m_graphDocumentId);
    }

    void MaterialPreviewViewportContent::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        if (!m_graphDocumentId.IsNull() && documentId == m_graphDocumentId)
        {
            SetGraphDocumentId(AZ::Uuid::CreateNull());
        }
    }

    void MaterialPreviewViewportContent::OnDocumentDestroyed(const AZ::Uuid& documentId)
    {
        OnDocumentClosed(documentId);
    }

    void MaterialPreviewViewportContent::OnCompileGraphStarted(const AZ::Uuid& documentId)
    {
        if (!m_graphDocumentId.IsNull() && documentId == m_graphDocumentId &&
            JE::Editor::Tools::GetSettingsValue("/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphStarted", true))
        {
            ApplyGeneratedMaterial(AZ::Uuid::CreateNull());
        }
    }

    void MaterialPreviewViewportContent::OnCompileGraphCompleted(const AZ::Uuid& documentId)
    {
        if (!m_graphDocumentId.IsNull() && documentId == m_graphDocumentId)
        {
            ApplyGeneratedMaterial(documentId);
        }
    }

    void MaterialPreviewViewportContent::OnCompileGraphFailed(const AZ::Uuid& documentId)
    {
        if (!m_graphDocumentId.IsNull() && documentId == m_graphDocumentId &&
            JE::Editor::Tools::GetSettingsValue("/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphFailed", true))
        {
            ApplyGeneratedMaterial(AZ::Uuid::CreateNull());
        }
    }

    void MaterialPreviewViewportContent::ApplyGeneratedMaterial(const AZ::Uuid& documentId)
    {
        AZ::Data::AssetId assetId;
        AZStd::vector<AZStd::string> generatedFiles;
        if (!documentId.IsNull())
        {
            JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
                generatedFiles, documentId, &JE::Editor::Tools::GraphDocumentRequests::GetGeneratedFilePaths);
        }
        for (const auto& path : generatedFiles)
        {
            if (path.ends_with(".material"))
            {
                if (auto outcome = JE::Render::RPI::AssetUtils::MakeAssetId(path, 0))
                {
                    assetId = outcome.GetValue();
                    break;
                }
            }
        }
        // Material graph defaults may change only the materialtype, so each completion needs a unique instance.
        JE::Render::MaterialAssignment assignment;
        assignment.m_materialAsset.Create(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        assignment.m_materialInstanceMustBeUnique = true;
        JE::Render::MaterialAssignmentMap materials;
        materials.emplace(JE::Render::DefaultMaterialAssignmentId, assignment);
        JE::Render::MaterialComponentRequestBus::Event(
            GetObjectEntityId(), &JE::Render::MaterialComponentRequestBus::Events::SetMaterialMap, materials);
    }

    void MaterialPreviewViewportContent::OnViewportSettingsChanged()
    {
        JE::Editor::Tools::EntityPreviewViewportContent::OnViewportSettingsChanged();

        JE::Editor::Tools::EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](JE::Editor::Tools::EntityPreviewViewportSettingsRequestBus::Events* viewportRequests)
            {
                const auto& modelPreset = viewportRequests->GetModelPreset();
                const auto& lightingPreset = viewportRequests->GetLightingPreset();

                JE::Render::Scene::MeshComponentRequestBus::Event(
                    GetObjectEntityId(),
                    [&](JE::Render::Scene::MeshComponentRequests* meshComponentRequests)
                    {
                        if (meshComponentRequests->GetModelAsset() != modelPreset.m_modelAsset)
                        {
                            meshComponentRequests->SetModelAsset(modelPreset.m_modelAsset);
                        }
                    });

                JE::Render::Scene::HDRiSkyboxRequestBus::Event(
                    GetEnvironmentEntityId(),
                    [&](JE::Render::Scene::HDRiSkyboxRequests* skyboxComponentRequests)
                    {
                        skyboxComponentRequests->SetExposure(lightingPreset.m_skyboxExposure);
                        skyboxComponentRequests->SetCubemapAsset(
                            viewportRequests->GetAlternateSkyboxEnabled() ? lightingPreset.m_alternateSkyboxImageAsset
                                                                          : lightingPreset.m_skyboxImageAsset);
                    });

                JE::Render::Scene::MeshComponentRequestBus::Event(
                    GetShadowCatcherEntityId(), &JE::Render::Scene::MeshComponentRequestBus::Events::SetVisibility,
                    viewportRequests->GetShadowCatcherEnabled());

                JE::Render::MaterialComponentRequestBus::Event(
                    GetShadowCatcherEntityId(), &JE::Render::MaterialComponentRequestBus::Events::SetPropertyValue,
                    JE::Render::DefaultMaterialAssignmentId, "settings.opacity", AZStd::any(lightingPreset.m_shadowCatcherOpacity));

                JE::Render::Scene::DisplayMapperComponentRequestBus::Event(
                    GetPostFxEntityId(), &JE::Render::Scene::DisplayMapperComponentRequestBus::Events::SetDisplayMapperOperationType,
                    viewportRequests->GetDisplayMapperOperationType());

                JE::Render::Scene::GridComponentRequestBus::Event(
                    GetGridEntityId(), &JE::Render::Scene::GridComponentRequestBus::Events::SetSize,
                    viewportRequests->GetGridEnabled() ? 4.0f : 0.0f);
            });
    }
} // namespace JE::Editor
