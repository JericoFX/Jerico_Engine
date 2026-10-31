/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "WhiteBoxTestFixtures.h"
#include <JE/Editor/Materials/EditorMaterialComponent.h>
#include <JE/Editor/Materials/MaterialBrowserInteractions.h>
#include <JE/Render/Materials/MaterialComponentBus.h>
#include <JE/Render/Materials/MaterialComponentController.h>
#include <JE/Render/Materials/MaterialsRegistration.h>
#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAssetCreator.h>
#include <JE/Render/RPI/Reflect/Material/MaterialTypeAssetCreator.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Render/IntersectorInterface.h>
#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesManager.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <QDropEvent>
#include <QMimeData>
#include <QTemporaryDir>

namespace UnitTest
{
    namespace Api = JE::Geometry::Api;

    // Real editor entity context, prefab transactions, composition and Undo; no composition bus double.
    class MaterialBspDropIntegrationTests : public EditorWhiteBoxComponentTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override
        {
            EditorWhiteBoxComponentTestFixture::SetUpEditorFixtureImpl();
            m_runtimeMaterialDescriptor.reset(JE::Render::CreateMaterialComponentDescriptor());
            GetApplication()->RegisterComponentDescriptor(m_runtimeMaterialDescriptor.get());
            m_materialDescriptor.reset(JE::Editor::EditorMaterialComponent::CreateDescriptor());
            GetApplication()->RegisterComponentDescriptor(m_materialDescriptor.get());
            auto& manager = AZ::Data::AssetManager::Instance();
            if (!manager.GetHandler(azrtti_typeid<JE::Render::RPI::MaterialAsset>()))
            {
                m_materialHandler = JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::MaterialAssetHandler>();
            }
            if (!manager.GetHandler(azrtti_typeid<JE::Render::RPI::MaterialTypeAsset>()))
            {
                m_typeHandler = JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::MaterialTypeAssetHandler>();
            }
            ASSERT_TRUE(m_directory.isValid());
            ASSERT_EQ(AzToolsFramework::AssetBrowser::EntryCache::GetInstance(), nullptr);
            m_root = AZStd::make_shared<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry>();
            m_ownsBrowserCache = true;
            m_model = AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserModel>();
            m_model->SetRootEntry(m_root);
            m_root->Update(m_directory.path().toUtf8().constData());
            AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry folder;
            folder.m_scanFolderID = 1;
            folder.m_scanFolder = m_directory.path().toUtf8().constData();
            folder.m_displayName = "BSP material drop integration";
            folder.m_isRoot = true;
            m_root->AddScanFolder(folder);
            m_interactions = AZStd::make_unique<JE::Editor::MaterialBrowserInteractions>();
        }

        void TearDownEditorFixtureImpl() override
        {
            m_interactions.reset();
            m_model.reset();
            m_root.reset();
            if (m_ownsBrowserCache)
            {
                AzToolsFramework::AssetBrowser::EntryCache::DestroyInstance();
                AzToolsFramework::AssetBrowser::AssetBrowserFavoritesManager::DestroyInstance();
            }
            if (m_otherEntityId.IsValid())
            {
                AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                    &AzToolsFramework::EditorEntityContextRequests::DestroyEditorEntity, m_otherEntityId);
            }
            EditorWhiteBoxComponentTestFixture::TearDownEditorFixtureImpl();
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::FlushUndo);
            if (m_materialDescriptor)
            {
                GetApplication()->UnregisterComponentDescriptor(m_materialDescriptor.get());
                m_materialDescriptor.reset();
            }
            if (m_runtimeMaterialDescriptor)
            {
                GetApplication()->UnregisterComponentDescriptor(m_runtimeMaterialDescriptor.get());
                m_runtimeMaterialDescriptor.reset();
            }
            m_materialHandler.reset();
            m_typeHandler.reset();
        }

        JE::Geometry::EditorWhiteBoxComponent* CurrentWhiteBox()
        {
            auto* entity = GetApplication()->FindEntity(m_whiteBoxEntityId);
            return entity ? entity->FindComponent<JE::Geometry::EditorWhiteBoxComponent>() : nullptr;
        }

        AZ::Data::Asset<JE::Render::RPI::MaterialAsset> CreateMaterial()
        {
            JE::Render::RPI::MaterialTypeAssetCreator typeCreator;
            typeCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            typeCreator.SetVersion(1);
            typeCreator.BeginMaterialProperty(AZ::Name("roughness.factor"), JE::Render::RPI::MaterialPropertyDataType::Float);
            typeCreator.EndMaterialProperty();
            typeCreator.SetPropertyValue(AZ::Name("roughness.factor"), 0.5f);
            AZ::Data::Asset<JE::Render::RPI::MaterialTypeAsset> type;
            EXPECT_TRUE(typeCreator.End(type));
            if (!type.IsReady()) { return {}; }
            JE::Render::RPI::MaterialAssetCreator creator;
            creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()), type);
            AZ::Data::Asset<JE::Render::RPI::MaterialAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            return asset;
        }

        const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* AddMaterialProduct(
            AZ::s64 fileId, const AZ::Data::Asset<JE::Render::RPI::MaterialAsset>& material)
        {
            const auto name = AZStd::string::format("surface%lld.material", fileId);
            AzToolsFramework::AssetDatabase::FileDatabaseEntry file;
            file.m_fileID = fileId;
            file.m_scanFolderPK = 1;
            file.m_fileName = name;
            file.m_isFolder = false;
            m_root->AddFile(file);
            AzToolsFramework::AssetBrowser::SourceWithFileID source;
            source.first = fileId;
            source.second.m_sourceID = fileId;
            source.second.m_scanFolderPK = 1;
            source.second.m_sourceName = name;
            source.second.m_sourceGuid = material.GetId().m_guid;
            EXPECT_TRUE(m_root->AddSource(source));
            AzToolsFramework::AssetBrowser::ProductWithUuid product;
            product.first = material.GetId().m_guid;
            product.second.m_productID = fileId;
            product.second.m_subID = material.GetId().m_subId;
            product.second.m_assetType = azrtti_typeid<JE::Render::RPI::MaterialAsset>();
            product.second.m_productName = AZStd::string::format("pc/surface%lld.azmaterial", fileId);
            EXPECT_TRUE(m_root->AddProduct(product));
            return AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry::GetProductByAssetId(material.GetId());
        }

        AzFramework::RenderGeometry::RayResult Pick(const AZ::Vector3& start, const AZ::Vector3& end)
        {
            AzFramework::RenderGeometry::RayRequest ray;
            ray.m_startWorldPosition = start;
            ray.m_endWorldPosition = end;
            AzFramework::RenderGeometry::RayResult hit;
            AzFramework::RenderGeometry::IntersectorBus::EventResult(hit, AzToolsFramework::GetEntityContextId(),
                &AzFramework::RenderGeometry::IntersectorInterface::RayIntersect, ray);
            return hit;
        }

        void Drop(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product,
            const AzFramework::RenderGeometry::RayResult& hit)
        {
            QMimeData mime;
            AzToolsFramework::AssetBrowser::Utils::ToMimeData(&mime, {product});
            AzQtComponents::ViewportDragContext context;
            context.m_hitLocation = hit.m_worldPosition;
            context.m_hitEntityId = hit.m_entityAndComponent.GetEntityId();
            context.m_hitComponentId = hit.m_entityAndComponent.GetComponentId();
            context.m_hitSurfaceId = hit.m_surfaceId;
            context.m_hitMaterialSlotId = hit.m_materialSlotId;
            QDropEvent event(QPointF(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
            event.setAccepted(false);
            AzQtComponents::DragAndDropEventsBus::Event(AzQtComponents::DragAndDropContexts::EditorViewport,
                &AzQtComponents::DragAndDropEvents::Drop, &event, context);
            EXPECT_TRUE(event.isAccepted());
            EXPECT_EQ(event.dropAction(), Qt::CopyAction);
        }

        AzToolsFramework::EntityIdList CurrentSelection()
        {
            AzToolsFramework::EntityIdList selected;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selected,
                &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
            return selected;
        }

        void ExpectMaterial(const AZ::Uuid& surface, const AZ::Data::AssetId& material)
        {
            auto* whiteBox = CurrentWhiteBox();
            ASSERT_NE(whiteBox, nullptr);
            const auto slot = JE::BSP::FindSurfaceMaterialSlot(whiteBox->GetSurfaceMaterialBindings(), surface);
            ASSERT_NE(slot, 0u);
            AZ::Data::AssetId actual;
            JE::Render::MaterialComponentRequestBus::EventResult(actual, m_whiteBoxEntityId,
                &JE::Render::MaterialComponentRequests::GetMaterialAssetId,
                JE::Render::MaterialAssignmentId::CreateFromStableIdOnly(slot));
            EXPECT_EQ(actual, material);
        }

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_runtimeMaterialDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_materialDescriptor;
        AZStd::unique_ptr<JE::Render::RPI::MaterialAssetHandler> m_materialHandler;
        AZStd::unique_ptr<JE::Render::RPI::MaterialTypeAssetHandler> m_typeHandler;
        QTemporaryDir m_directory;
        AZStd::shared_ptr<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry> m_root;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserModel> m_model;
        AZStd::unique_ptr<JE::Editor::MaterialBrowserInteractions> m_interactions;
        bool m_ownsBrowserCache = false;
        AZ::EntityId m_otherEntityId;
    };

    TEST_F(MaterialBspDropIntegrationTests, TwoDeferredFaceDropsCreateOneOwnerPreserveSelectionAndUndoRedo)
    {
        auto* whiteBox = CurrentWhiteBox();
        ASSERT_NE(whiteBox, nullptr);
        auto* mesh = whiteBox->GetWhiteBoxMesh();
        ASSERT_NE(mesh, nullptr);
        const auto topId = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555555}");
        const auto sideId = AZ::Uuid::CreateString("{11111111-2222-3333-4444-555555555556}");
        bool foundTop = false, foundSide = false;
        for (const auto& polygon : Api::MeshPolygonHandles(*mesh))
        {
            ASSERT_FALSE(polygon.m_faceHandles.empty());
            const auto normal = Api::FaceNormal(*mesh, polygon.m_faceHandles.front());
            if (normal.IsClose(AZ::Vector3::CreateAxisZ()))
            {
                for (const auto face : polygon.m_faceHandles) { Api::SetFaceSurfaceId(*mesh, face, topId); }
                foundTop = true;
            }
            if (normal.IsClose(AZ::Vector3::CreateAxisX()))
            {
                for (const auto face : polygon.m_faceHandles) { Api::SetFaceSurfaceId(*mesh, face, sideId); }
                foundSide = true;
            }
        }
        ASSERT_TRUE(foundTop);
        ASSERT_TRUE(foundSide);
        const auto componentId = whiteBox->GetId();
        whiteBox->SerializeWhiteBox();
        whiteBox->RebuildWhiteBox();
        auto* prefab = AZ::Interface<AzToolsFramework::Prefab::PrefabPublicInterface>::Get();
        ASSERT_NE(prefab, nullptr);
        {
            AzToolsFramework::ScopedUndoBatch setup("Prepare BSP drop fixture");
            prefab->GenerateUndoNodesForEntityChangeAndUpdateCache(m_whiteBoxEntityId, setup.GetUndoBatch());
        }
        m_otherEntityId = CreateDefaultEditorEntity("Selected other entity");
        ASSERT_TRUE(m_otherEntityId.IsValid());
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
            AzToolsFramework::EntityIdList{m_otherEntityId});
        const auto selection = CurrentSelection();
        // Complete setup propagation before either deferred drop is submitted.
        auto* updates = AZ::Interface<AzToolsFramework::Prefab::InstanceUpdateExecutorInterface>::Get();
        ASSERT_NE(updates, nullptr);
        ASSERT_TRUE(updates->UpdateTemplateInstancesInQueue(true));
        whiteBox = CurrentWhiteBox();
        ASSERT_NE(whiteBox, nullptr);
        ASSERT_EQ(whiteBox->GetId(), componentId);
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::FlushUndo);
        ASSERT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(m_whiteBoxEntityId));
        ASSERT_TRUE(AzFramework::RenderGeometry::IntersectorBus::HasHandlers(AzToolsFramework::GetEntityContextId()));
        const auto topHit = Pick(AZ::Vector3(0.125f, 0.2f, 2.0f), AZ::Vector3(0.125f, 0.2f, -2.0f));
        const auto sideHit = Pick(AZ::Vector3(2.0f, 0.125f, 0.2f), AZ::Vector3(-2.0f, 0.125f, 0.2f));
        ASSERT_TRUE(topHit);
        ASSERT_TRUE(sideHit);
        ASSERT_TRUE(topHit.m_surfaceId.has_value());
        ASSERT_TRUE(sideHit.m_surfaceId.has_value());
        ASSERT_EQ(*topHit.m_surfaceId, topId);
        ASSERT_EQ(*sideHit.m_surfaceId, sideId);
        ASSERT_EQ(topHit.m_entityAndComponent.GetEntityId(), m_whiteBoxEntityId);
        ASSERT_EQ(sideHit.m_entityAndComponent, topHit.m_entityAndComponent);
        const auto a = CreateMaterial();
        const auto b = CreateMaterial();
        ASSERT_TRUE(a.IsReady());
        ASSERT_TRUE(b.IsReady());
        const auto* productA = AddMaterialProduct(77, a);
        const auto* productB = AddMaterialProduct(88, b);
        ASSERT_NE(productA, nullptr);
        ASSERT_NE(productB, nullptr);
        Drop(productA, topHit);
        Drop(productB, sideHit);
        // The assets are Ready; only delivery of the two authoring intents is deferred. No disk-loading claim.
        EXPECT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(m_whiteBoxEntityId));
        AZ::Data::AssetManager::Instance().DispatchEvents();
        AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
        EXPECT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(m_whiteBoxEntityId));
        AZ::SystemTickBus::ExecuteQueuedEvents();
        ASSERT_TRUE(JE::Render::MaterialComponentRequestBus::HasHandlers(m_whiteBoxEntityId));
        ASSERT_NO_FATAL_FAILURE(ExpectMaterial(topId, a.GetId()));
        ASSERT_NO_FATAL_FAILURE(ExpectMaterial(sideId, b.GetId()));
        AZ::Entity::ComponentArrayType components;
        AzToolsFramework::GetAllComponentsForEntity(m_whiteBoxEntityId, components);
        size_t materialOwners = 0;
        for (const auto* component : components)
        {
            materialOwners += component->RTTI_IsTypeOf(AZ::Uuid(JE::Render::EditorMaterialComponentTypeId)) ? 1u : 0u;
        }
        EXPECT_EQ(materialOwners, 1);
        EXPECT_EQ(CurrentSelection(), selection);
        AzToolsFramework::UndoSystem::UndoStack* undo = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(undo, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);
        ASSERT_NE(undo, nullptr);
        ASSERT_TRUE(undo->CanUndo());
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);
        ASSERT_NE(CurrentWhiteBox(), nullptr);
        const auto undoOneTopSlot = JE::BSP::FindSurfaceMaterialSlot(CurrentWhiteBox()->GetSurfaceMaterialBindings(), topId);
        const auto undoOneSideSlot = JE::BSP::FindSurfaceMaterialSlot(CurrentWhiteBox()->GetSurfaceMaterialBindings(), sideId);
        // Ready loaders need not complete in drop order. Capture the actual first Undo state.
        ASSERT_NE(undoOneTopSlot != 0u, undoOneSideSlot != 0u);
        if (undoOneTopSlot != 0u)
        {
            ASSERT_NO_FATAL_FAILURE(ExpectMaterial(topId, a.GetId()));
        }
        else
        {
            ASSERT_NO_FATAL_FAILURE(ExpectMaterial(sideId, b.GetId()));
        }
        ASSERT_TRUE(undo->CanUndo());
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);
        ASSERT_NE(CurrentWhiteBox(), nullptr);
        EXPECT_EQ(JE::BSP::FindSurfaceMaterialSlot(CurrentWhiteBox()->GetSurfaceMaterialBindings(), topId), 0u);
        EXPECT_EQ(JE::BSP::FindSurfaceMaterialSlot(CurrentWhiteBox()->GetSurfaceMaterialBindings(), sideId), 0u);
        EXPECT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(m_whiteBoxEntityId));
        ASSERT_TRUE(undo->CanRedo());
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);
        ASSERT_NE(CurrentWhiteBox(), nullptr);
        EXPECT_EQ(JE::BSP::FindSurfaceMaterialSlot(CurrentWhiteBox()->GetSurfaceMaterialBindings(), topId), undoOneTopSlot);
        EXPECT_EQ(JE::BSP::FindSurfaceMaterialSlot(CurrentWhiteBox()->GetSurfaceMaterialBindings(), sideId), undoOneSideSlot);
        if (undoOneTopSlot != 0u)
        {
            ASSERT_NO_FATAL_FAILURE(ExpectMaterial(topId, a.GetId()));
        }
        else
        {
            ASSERT_NO_FATAL_FAILURE(ExpectMaterial(sideId, b.GetId()));
        }
        ASSERT_TRUE(undo->CanRedo());
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);
        ASSERT_NO_FATAL_FAILURE(ExpectMaterial(topId, a.GetId()));
        ASSERT_NO_FATAL_FAILURE(ExpectMaterial(sideId, b.GetId()));
        EXPECT_EQ(CurrentSelection(), selection);
        // Destroy the native entities in teardown before any GPU/material-instance processing tick.
    }

    TEST_F(MaterialBspDropIntegrationTests, ReadyReconnectPreservesEachAuthoredReferenceLoadBehavior)
    {
        using Behavior = AZ::Data::AssetLoadBehavior;
        using Assignment = JE::Render::MaterialAssignment;
        using AssignmentId = JE::Render::MaterialAssignmentId;
        using AssignmentMap = JE::Render::MaterialAssignmentMap;
        const auto asset = CreateMaterial();
        ASSERT_TRUE(asset.IsReady());

        // ConnectionPolicy supplies a real already-Ready notification with Default=QueueLoad.
        class ReadyObserver final : public AZ::Data::AssetBus::Handler
        {
        public:
            ~ReadyObserver() override { BusDisconnect(); }
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> ready) override
            {
                ++m_calls;
                m_ready = ready.IsReady();
                m_behavior = ready.GetAutoLoadBehavior();
            }
            size_t m_calls = 0;
            bool m_ready = false;
            Behavior m_behavior = Behavior::PreLoad;
        } observer;
        observer.BusConnect(asset.GetId());
        ASSERT_EQ(observer.m_calls, 1u);
        ASSERT_TRUE(observer.m_ready);
        ASSERT_EQ(observer.m_behavior, Behavior::QueueLoad);

        class Consumer final : public JE::Render::MaterialConsumerRequestBus::Handler
        {
        public:
            explicit Consumer(AZ::EntityId id)
            {
                m_defaults.emplace(AssignmentId::CreateFromStableIdOnly(11), Assignment{});
                m_defaults.emplace(AssignmentId::CreateFromStableIdOnly(22), Assignment{});
                BusConnect(id);
            }
            ~Consumer() override { BusDisconnect(); }
            AssignmentId FindMaterialAssignmentId(JE::Render::MaterialAssignmentLodIndex, const AZStd::string&) const override { return {}; }
            JE::Render::MaterialAssignmentLabelMap GetMaterialLabels() const override { return {}; }
            AssignmentMap GetDefaultMaterialMap() const override { return m_defaults; }
            AZStd::unordered_set<AZ::Name> GetModelUvNames() const override { return {}; }
            AssignmentMap m_defaults;
        };
        const AZ::EntityId entityId = AZ::Entity::MakeId();
        ASSERT_FALSE(JE::Render::MaterialConsumerRequestBus::HasHandlers(entityId));
        ASSERT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(entityId));
        Consumer consumer(entityId);
        struct ScopedController
        {
            ~ScopedController() { if (m_active) { m_value.Deactivate(); } }
            JE::Render::MaterialComponentController m_value;
            bool m_active = false;
        } controller;
        const auto preloadId = AssignmentId::CreateFromStableIdOnly(11);
        const auto noLoadId = AssignmentId::CreateFromStableIdOnly(22);
        JE::Render::MaterialComponentConfig config;
        for (const auto id : {preloadId, noLoadId})
        {
            Assignment assignment;
            assignment.m_materialAsset = asset;
            assignment.m_materialAsset.SetAutoLoadBehavior(id == preloadId ? Behavior::PreLoad : Behavior::NoLoad);
            // This CPU reference-policy test excludes material-instance/GPU creation.
            assignment.m_materialInstancePreCreated = true;
            config.m_materials.emplace(id, assignment);
        }
        controller.m_value.SetConfiguration(config);
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            SCOPED_TRACE(cycle);
            controller.m_value.Activate(entityId);
            controller.m_active = true;
            AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
            const auto& actual = controller.m_value.GetMaterialMap();
            ASSERT_EQ(actual.size(), 2u);
            ASSERT_TRUE(actual.contains(preloadId));
            ASSERT_TRUE(actual.contains(noLoadId));
            for (const auto id : {preloadId, noLoadId})
            {
                EXPECT_EQ(actual.at(id).m_materialAsset.GetId(), asset.GetId());
                EXPECT_EQ(actual.at(id).m_materialAsset.Get(), asset.Get());
                EXPECT_TRUE(actual.at(id).m_materialAsset.IsReady());
            }
            EXPECT_EQ(actual.at(preloadId).m_materialAsset.GetAutoLoadBehavior(), Behavior::PreLoad);
            EXPECT_EQ(actual.at(noLoadId).m_materialAsset.GetAutoLoadBehavior(), Behavior::NoLoad);
            controller.m_value.Deactivate();
            controller.m_active = false;
        }
    }
}
