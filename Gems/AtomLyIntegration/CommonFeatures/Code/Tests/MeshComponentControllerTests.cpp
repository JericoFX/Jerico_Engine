/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInternalInterface.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <Mesh/EditorMeshComponent.h>
#include <Mesh/MeshComponentController.h>

namespace UnitTest
{
    class IntersectionNotificationDetector : public AzFramework::RenderGeometry::IntersectionNotificationBus::Handler
    {
    public:
        void Connect(const AzFramework::EntityContextId& entityContextId);
        void Disconnect();

        // IntersectionNotificationBus overrides ...
        void OnEntityConnected(AZ::EntityId entityId) override;
        void OnEntityDisconnected(AZ::EntityId entityId) override;
        void OnGeometryChanged(AZ::EntityId entityId) override;

        AZ::EntityId m_lastEntityIdChanged;
    };

    void IntersectionNotificationDetector::Connect(const AzFramework::EntityContextId& entityContextId)
    {
        AzFramework::RenderGeometry::IntersectionNotificationBus::Handler::BusConnect(entityContextId);
    }

    void IntersectionNotificationDetector::Disconnect()
    {
        AzFramework::RenderGeometry::IntersectionNotificationBus::Handler::BusDisconnect();
    }

    void IntersectionNotificationDetector::OnEntityConnected([[maybe_unused]] AZ::EntityId entityId)
    {
    }

    void IntersectionNotificationDetector::OnEntityDisconnected([[maybe_unused]] AZ::EntityId entityId)
    {
    }

    void IntersectionNotificationDetector::OnGeometryChanged(AZ::EntityId entityId)
    {
        m_lastEntityIdChanged = entityId;
    }

    class MeshComponentControllerFixture : public ToolsApplicationFixture<false>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            m_meshComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AZ::Render::MeshComponent::CreateDescriptor());
            m_meshComponentDescriptor->Reflect(GetApplication()->GetSerializeContext());

            m_editorMeshComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(AZ::Render::EditorMeshComponent::CreateDescriptor());
            m_editorMeshComponentDescriptor->Reflect(GetApplication()->GetSerializeContext());

            m_entity = AZStd::make_unique<AZ::Entity>();
            m_entity->Init();
            m_entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            m_entity->Activate();

            AzFramework::EntityContextId contextId(AZStd::string_view{"123456"});
            m_intersectionNotificationDetector.Connect(contextId);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_entity.reset();

            m_intersectionNotificationDetector.Disconnect();

            auto* serializeContext = GetApplication()->GetSerializeContext();
            serializeContext->EnableRemoveReflection();
            m_editorMeshComponentDescriptor->Reflect(serializeContext);
            m_meshComponentDescriptor->Reflect(serializeContext);
            serializeContext->DisableRemoveReflection();

            m_meshComponentDescriptor.reset();
            m_editorMeshComponentDescriptor.reset();
        }

        AZStd::unique_ptr<AZ::Entity> m_entity;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_meshComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorMeshComponentDescriptor;
        IntersectionNotificationDetector m_intersectionNotificationDetector;
    };

    TEST_F(MeshComponentControllerFixture, InvalidModelAssetIdDoesNotDereferenceMissingAssetData)
    {
        UnitTest::ErrorHandler loadFailure("Could not load");
        UnitTest::ErrorHandler notQueued("BlockUntilLoadComplete must be called after an asset has been queued for load");
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a MeshFeatureProcessorInterface on the entityId.");
        m_entity->Deactivate();
        m_entity->CreateComponent<AZ::Render::MeshComponent>();
        m_entity->Activate();
        ASSERT_TRUE(AZ::Render::MeshComponentRequestBus::HasHandlers(m_entity->GetId()));

        AZ::Render::MeshComponentRequestBus::Event(
            m_entity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::SetModelAssetId, AZ::Data::AssetId{});

        AZ::Data::AssetId actualAssetId;
        AZ::Data::Asset<const AZ::RPI::ModelAsset> actualAsset;
        AZ::Render::MeshComponentRequestBus::EventResult(
            actualAssetId, m_entity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);
        AZ::Render::MeshComponentRequestBus::EventResult(
            actualAsset, m_entity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);
        EXPECT_FALSE(actualAssetId.IsValid());
        EXPECT_FALSE(actualAsset.IsReady());
        m_entity->Deactivate();
        EXPECT_EQ(loadFailure.GetExpectedErrorCount(), 1);
        EXPECT_EQ(notQueued.GetExpectedErrorCount(), 1);
        // AssetData assertions remain unsuppressed: the old operator-> readiness query must fail this test.
    }

    TEST_F(MeshComponentControllerFixture, UnavailableModelProductPreservesRequestedAssetIdWithoutDereferencingAssetData)
    {
        const AZ::Data::AssetId modelAssetId(AZ::Uuid::CreateRandom(), 0);
        ASSERT_EQ(AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<AZ::RPI::ModelAsset>()), nullptr);
        // Use the native generic handler and AssetManager, with no catalog entry or fallback preview model.
        auto modelHandler = AZ::RPI::MakeAssetHandler<AZ::RPI::AssetHandler<AZ::RPI::ModelAsset>>();
        UnitTest::ErrorHandler missingProduct("GetAsset called for asset which does not exist in asset catalog");
        UnitTest::ErrorHandler loadFailure("Could not load");
        UnitTest::ErrorHandler notQueued("BlockUntilLoadComplete must be called after an asset has been queued for load");
        {
            UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a MeshFeatureProcessorInterface on the entityId.");
            m_entity->Deactivate();
            m_entity->CreateComponent<AZ::Render::MeshComponent>();
            m_entity->Activate();
            ASSERT_TRUE(AZ::Render::MeshComponentRequestBus::HasHandlers(m_entity->GetId()));
            AZ::Render::MeshComponentRequestBus::Event(
                m_entity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::SetModelAssetId, modelAssetId);

            AZ::Data::AssetId actualAssetId;
            AZ::Data::Asset<const AZ::RPI::ModelAsset> actualAsset;
            AZ::Render::MeshComponentRequestBus::EventResult(
                actualAssetId, m_entity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);
            AZ::Render::MeshComponentRequestBus::EventResult(
                actualAsset, m_entity->GetId(), &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);
            EXPECT_EQ(actualAssetId, modelAssetId);
            EXPECT_FALSE(actualAsset.IsReady());
            m_entity->Deactivate();
            EXPECT_EQ(loadFailure.GetExpectedErrorCount(), 1);
            EXPECT_EQ(notQueued.GetExpectedErrorCount(), 1);
            EXPECT_GT(missingProduct.GetExpectedWarningCount(), 0);
        }
        AZ::Data::AssetManager::Instance().DispatchEvents();
    }

    TEST_F(MeshComponentControllerFixture, IntersectionNotificationBusIsNotifiedWhenMeshComponentControllerTransformIsModified)
    {
        // suppress warning when feature process is not created in test environment
        UnitTest::ErrorHandler featureProcessorNotFound("Unable to find a MeshFeatureProcessorInterface on the entityId.");

        m_entity->Deactivate();
        m_entity->CreateComponent<AZ::Render::EditorMeshComponent>();
        m_entity->Activate();

        AZ::TransformBus::Event(
            m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, AZ::Transform::CreateTranslation(AZ::Vector3(1.0f, 2.0f, 3.0f)));

        using ::testing::Eq;
        EXPECT_THAT(m_entity->GetId(), Eq(m_intersectionNotificationDetector.m_lastEntityIdChanged));
    }
} // namespace UnitTest
