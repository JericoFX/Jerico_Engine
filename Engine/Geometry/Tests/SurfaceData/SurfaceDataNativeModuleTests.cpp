/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <JE/Geometry/SurfaceData/SurfaceDataModule.h>
#include <JE/Geometry/SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <JE/Geometry/SurfaceData/SurfaceDataProviderRequestBus.h>
#include <JE/Geometry/SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <JE/Geometry/SurfaceData/SurfaceDataTagProviderRequestBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    namespace Surface = JE::Geometry::SurfaceData;

    // Only the provider boundary is controlled; the module, registry, query/filter and notifications are real.
    class NativeSurfaceProvider final
        : public Surface::SurfaceDataProviderRequestBus::Handler
    {
    public:
        explicit NativeSurfaceProvider(Surface::SurfaceDataSystem& system)
            : m_system(system)
        {
            m_entry.m_entityId = AZ::EntityId(8123);
            m_entry.m_tags = {Surface::SurfaceTag("native-surface")};
            m_entry.m_maxPointsCreatedPerInput = 1;
            m_entry.m_bounds = AZ::Aabb::CreateCenterRadius(m_position, 0.5f);
            m_handle = m_system.RegisterSurfaceDataProvider(m_entry);
            BusConnect(m_handle);
        }

        ~NativeSurfaceProvider() override
        {
            BusDisconnect();
            m_system.UnregisterSurfaceDataProvider(m_handle);
        }

        void Move(const AZ::Vector3& position)
        {
            m_position = position;
            m_entry.m_bounds = AZ::Aabb::CreateCenterRadius(m_position, 0.5f);
            m_system.UpdateSurfaceDataProvider(m_handle, m_entry);
        }

        void GetSurfacePoints(const AZ::Vector3& input, Surface::SurfacePointList& points) const override
        {
            Surface::SurfaceTagWeights weights;
            weights.AssignSurfaceTagWeights(m_entry.m_tags, 0.75f);
            points.AddSurfacePoint(m_entry.m_entityId, input, m_position, AZ::Vector3::CreateAxisZ(), weights);
        }

        Surface::SurfaceDataSystem& m_system;
        Surface::SurfaceDataRegistryEntry m_entry;
        Surface::SurfaceDataRegistryHandle m_handle = Surface::InvalidSurfaceDataRegistryHandle;
        AZ::Vector3 m_position{2.0f, 3.0f, 4.0f};
    };

    class NativeSurfaceNotifications final
        : public Surface::SurfaceDataSystemNotificationBus::Handler
    {
    public:
        NativeSurfaceNotifications() { BusConnect(); }
        ~NativeSurfaceNotifications() override { BusDisconnect(); }

        void OnSurfaceChanged(const AZ::EntityId& entityId, const AZ::Aabb& oldBounds, const AZ::Aabb& newBounds,
            const Surface::SurfaceTagSet& tags) override
        {
            m_entityId = entityId;
            m_oldBounds = oldBounds;
            m_newBounds = newBounds;
            m_tags = tags;
            ++m_changes;
        }

        AZ::EntityId m_entityId;
        AZ::Aabb m_oldBounds = AZ::Aabb::CreateNull();
        AZ::Aabb m_newBounds = AZ::Aabb::CreateNull();
        Surface::SurfaceTagSet m_tags;
        size_t m_changes = 0;
    };

    void VerifySurfaceDataModuleCycles(bool authoring)
    {
        const AZ::Uuid systemId("{6F334BAA-7BD5-45F8-A9BA-760667D25FA0}");
        const AZ::Uuid editorSystemId("{F3EE5137-856B-4E29-AADD-84F358AEA75F}");
        const AZ::Uuid tagAssetId("{A471B2A9-85FC-4993-842D-1881CBC03A2B}");
        const AZStd::vector<AZ::Uuid> runtimeIds{
            systemId,
            AZ::Uuid("{8BECC930-9B2A-442D-A291-8A3F6B6D1071}"),
            AZ::Uuid("{F746C7F6-EF59-45C3-AB5C-011F7AC43415}")};
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            EXPECT_EQ(AZ::Interface<Surface::SurfaceDataSystem>::Get(), nullptr);
            EXPECT_FALSE(Surface::SurfaceDataSystemRequestBus::HasHandlers());
            EXPECT_FALSE(Surface::SurfaceDataTagProviderRequestBus::HasHandlers());
            AzToolsFramework::ToolsApplication application;
            AZ::ComponentApplication::StartupParameters startup;
            startup.m_loadSettingsRegistry = false;
            startup.m_loadDynamicModules = false;
            application.Start(AZ::ComponentApplication::Descriptor(), startup);
            ASSERT_TRUE(AZ::Data::AssetManager::IsReady());
            auto* assetManager = &AZ::Data::AssetManager::Instance();
            EXPECT_EQ(assetManager->GetHandler(tagAssetId), nullptr);
            {
                AZ::ModuleManagerRequests::LoadModulesResult modules;
                AZ::ModuleManagerRequestBus::BroadcastResult(modules, &AZ::ModuleManagerRequests::LoadStaticModules,
                    [authoring](AZStd::vector<AZ::Module*>& result)
                    {
                        result.push_back(authoring ? Surface::CreateAuthoringModule() : Surface::CreateRuntimeModule());
                    }, AZ::ModuleInitializationSteps::ActivateEntity);
                ASSERT_EQ(modules.size(), 1);
                ASSERT_TRUE(modules.front().IsSuccess());
                const auto* data = modules.front().GetValue().get();
                ASSERT_NE(data->GetEntity(), nullptr);
                ASSERT_EQ(data->GetEntity()->GetState(), AZ::Entity::State::Active);
                const auto* module = data->GetModule();
                ASSERT_NE(module, nullptr);
                EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid(authoring
                    ? "{B80F2321-B79A-4161-B586-3E508655DFAF}" : "{B58B7CA8-98C9-4DC8-8607-E094989BBBE2}"));
                auto expectedIds = runtimeIds;
                AZ::ComponentTypeList required{systemId};
                if (authoring)
                {
                    expectedIds.insert(expectedIds.end(), {editorSystemId,
                        AZ::Uuid("{0F44367D-B9A3-4C25-A769-24993C8BF7A7}"),
                        AZ::Uuid("{A2E691AC-A027-4B90-A604-689E543BEA91}")});
                    required.push_back(editorSystemId);
                }
                EXPECT_EQ(module->GetRequiredSystemComponents(), required);
                const auto descriptors = module->GetComponentDescriptors();
                ASSERT_EQ(descriptors.size(), expectedIds.size());
                size_t index = 0;
                for (const auto* descriptor : descriptors)
                {
                    ASSERT_NE(descriptor, nullptr);
                    EXPECT_EQ(descriptor->GetUuid(), expectedIds[index++]);
                    ASSERT_NE(application.GetSerializeContext()->FindClassData(descriptor->GetUuid()), nullptr);
                }
                const auto* tagClass = application.GetSerializeContext()->FindClassData(
                    AZ::Uuid("{67C8C6ED-F32A-443E-A777-1CAE48B22CD7}"));
                ASSERT_NE(tagClass, nullptr);
                EXPECT_EQ(tagClass->m_version, 2);
                ASSERT_EQ(tagClass->m_elements.size(), 1);
                EXPECT_EQ(tagClass->m_elements.front().m_nameCrc, AZ_CRC_CE("SurfaceTagCrc"));
                auto* system = AZ::Interface<Surface::SurfaceDataSystem>::Get();
                ASSERT_NE(system, nullptr);
                EXPECT_TRUE(Surface::SurfaceDataSystemRequestBus::HasHandlers());
                EXPECT_EQ(Surface::SurfaceDataTagProviderRequestBus::HasHandlers(), authoring);
                if (authoring)
                {
                    EXPECT_NE(assetManager->GetHandler(tagAssetId), nullptr);
                }
                NativeSurfaceNotifications notifications;
                const Surface::SurfaceTag tag("native-surface");
                const Surface::SurfaceTagVector desiredTags{tag};
                const AZ::Vector3 firstPosition(2.0f, 3.0f, 4.0f);
                const AZ::Vector3 movedPosition(8.0f, 3.0f, 4.0f);
                {
                    NativeSurfaceProvider provider(*system);
                    EXPECT_NE(provider.m_handle, Surface::InvalidSurfaceDataRegistryHandle);
                    EXPECT_EQ(system->GetSurfaceDataProviderHandle(AZ::EntityId(8123)), provider.m_handle);
                    Surface::SurfacePointList points;
                    Surface::SurfaceDataSystemRequestBus::Broadcast(
                        &Surface::SurfaceDataSystemRequestBus::Events::GetSurfacePoints, firstPosition, desiredTags, points);
                    ASSERT_EQ(points.GetSize(), 1);
                    points.EnumeratePoints([&](size_t, const AZ::Vector3& position, const AZ::Vector3& normal,
                        const Surface::SurfaceTagWeights& weights)
                    {
                        EXPECT_EQ(position, firstPosition);
                        EXPECT_EQ(normal, AZ::Vector3::CreateAxisZ());
                        EXPECT_EQ(weights.GetSize(), 1);
                        EXPECT_TRUE(weights.HasMatchingTag(AZ::Crc32(tag), 0.75f, 0.75f));
                        return true;
                    });
                    const auto changesBeforeMove = notifications.m_changes;
                    provider.Move(movedPosition);
                    EXPECT_EQ(notifications.m_changes, changesBeforeMove + 1);
                    EXPECT_EQ(notifications.m_entityId, AZ::EntityId(8123));
                    EXPECT_EQ(notifications.m_oldBounds.GetCenter(), firstPosition);
                    EXPECT_EQ(notifications.m_newBounds.GetCenter(), movedPosition);
                    EXPECT_EQ(notifications.m_tags.count(tag), 1);
                    system->GetSurfacePoints(firstPosition, desiredTags, points);
                    EXPECT_EQ(points.GetSize(), 0);
                    system->GetSurfacePoints(movedPosition, desiredTags, points);
                    EXPECT_EQ(points.GetSize(), 1);
                    system->GetSurfacePoints(movedPosition, {Surface::SurfaceTag("other-surface")}, points);
                    EXPECT_EQ(points.GetSize(), 0);
                }
                EXPECT_EQ(system->GetSurfaceDataProviderHandle(AZ::EntityId(8123)), Surface::InvalidSurfaceDataRegistryHandle);
                Surface::SurfacePointList afterUnregister;
                system->GetSurfacePoints(movedPosition, desiredTags, afterUnregister);
                EXPECT_EQ(afterUnregister.GetSize(), 0);
            }
            // Observe the real shutdown phase before SystemEntity destruction can hide a stale asset handler.
            bool observedModuleDeactivation = false;
            AZ::SettingsRegistryInterface::NotifyEventHandler cleanupObserver;
            auto* settingsRegistry = AZ::SettingsRegistry::Get();
            ASSERT_NE(settingsRegistry, nullptr);
            ASSERT_TRUE(AZ::ComponentApplicationLifecycle::RegisterHandler(*settingsRegistry, cleanupObserver,
                [&](const AZ::SettingsRegistryInterface::NotifyEventArgs&)
                {
                    if (observedModuleDeactivation)
                    {
                        return;
                    }
                    observedModuleDeactivation = true;
                    EXPECT_EQ(AZ::Interface<Surface::SurfaceDataSystem>::Get(), nullptr);
                    EXPECT_FALSE(Surface::SurfaceDataSystemRequestBus::HasHandlers());
                    EXPECT_FALSE(Surface::SurfaceDataTagProviderRequestBus::HasHandlers());
                    EXPECT_TRUE(AZ::Data::AssetManager::IsReady());
                    if (AZ::Data::AssetManager::IsReady())
                    {
                        EXPECT_EQ(&AZ::Data::AssetManager::Instance(), assetManager);
                        EXPECT_EQ(assetManager->GetHandler(tagAssetId), nullptr);
                    }
                }, "SystemComponentsDeactivated"));
            application.Stop();
            EXPECT_TRUE(observedModuleDeactivation);
            EXPECT_EQ(AZ::Interface<Surface::SurfaceDataSystem>::Get(), nullptr);
            EXPECT_FALSE(Surface::SurfaceDataSystemRequestBus::HasHandlers());
            EXPECT_FALSE(Surface::SurfaceDataTagProviderRequestBus::HasHandlers());
            if (AZ::Data::AssetManager::IsReady())
            {
                EXPECT_EQ(AZ::Data::AssetManager::Instance().GetHandler(tagAssetId), nullptr);
            }
        }
    }

    class SurfaceDataNativeModuleTests : public LeakDetectionFixture {};

    TEST_F(SurfaceDataNativeModuleTests, RuntimeFactoryPreservesQueriesNotificationsAndTwoCycleLifetime)
    {
        VerifySurfaceDataModuleCycles(false);
    }

    TEST_F(SurfaceDataNativeModuleTests, AuthoringFactoryPreservesTagAssetHandlerAndTwoCycleLifetime)
    {
        VerifySurfaceDataModuleCycles(true);
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
