/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Render/Debug/RenderDebugModule.h>
#include <JE/Render/Debug/AtomImGuiToolsBus.h>
#include <AtomImGuiToolsSystemComponent.h>
#include <ImguiAtomSystemComponent.h>
#include <MaterialShaderDetailsController.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>

namespace JE::Render::Debug
{
    class RenderDebugTests : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_ownsNames = !AZ::NameDictionary::IsReady();
            if (m_ownsNames)
            {
                AZ::NameDictionary::Create();
            }
        }

        void TearDown() override
        {
            if (m_ownsNames)
            {
                AZ::NameDictionary::Destroy();
            }
            LeakDetectionFixture::TearDown();
        }

        bool m_ownsNames{};
    };

    TEST_F(RenderDebugTests, ViewportFactoryPreservesIdentityAndCommonServiceRequirement)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateViewportModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{E3CE5991-30B5-4B04-BF79-516DDBD4D233}"));
        const auto descriptors = module->GetComponentDescriptors();
        ASSERT_EQ(descriptors.size(), 1);
        const auto* descriptor = descriptors.front();
        ASSERT_NE(descriptor, nullptr);
        const AZ::Uuid componentId("{D423E075-D971-435A-A9C1-57C3B0623A9B}");
        EXPECT_EQ(descriptor->GetUuid(), componentId);
        const auto requiredComponents = module->GetRequiredSystemComponents();
        ASSERT_EQ(requiredComponents.size(), 1);
        EXPECT_EQ(requiredComponents.front(), componentId);
        AZ::ComponentDescriptor::DependencyArrayType required;
        descriptor->GetRequiredServices(required, nullptr);
        ASSERT_EQ(required.size(), 1);
        EXPECT_EQ(required.front(), AZ_CRC_CE("CommonService"));
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        descriptor->Reflect(&context);
        const auto* reflected = context.FindClassData(componentId);
        ASSERT_NE(reflected, nullptr);
        EXPECT_STREQ(reflected->m_name, "ImguiAtomSystemComponent");
        EXPECT_EQ(reflected->m_version, 0);
    }

    TEST_F(RenderDebugTests, ToolsFactoryPreservesIdentityWithoutViewportServiceRequirement)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateToolsModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{1B65F246-7977-4DC4-B5D9-BDAD374388FF}"));
        const auto descriptors = module->GetComponentDescriptors();
        ASSERT_EQ(descriptors.size(), 1);
        const auto* descriptor = descriptors.front();
        ASSERT_NE(descriptor, nullptr);
        const AZ::Uuid componentId("{AFA2493D-DF1C-4DBB-BC13-0AF990B3D5FC}");
        EXPECT_EQ(descriptor->GetUuid(), componentId);
        const auto requiredComponents = module->GetRequiredSystemComponents();
        ASSERT_EQ(requiredComponents.size(), 1);
        EXPECT_EQ(requiredComponents.front(), componentId);
        AZ::ComponentDescriptor::DependencyArrayType required;
        descriptor->GetRequiredServices(required, nullptr);
        EXPECT_TRUE(required.empty());
        AZ::ComponentDescriptor::DependencyArrayType provided;
        descriptor->GetProvidedServices(provided, nullptr);
        ASSERT_EQ(provided.size(), 1);
        EXPECT_EQ(provided.front(), AZ_CRC_CE("AtomImGuiToolsService"));
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        descriptor->Reflect(&context);
        const auto* reflected = context.FindClassData(componentId);
        ASSERT_NE(reflected, nullptr);
        EXPECT_STREQ(reflected->m_name, "AtomImGuiToolsSystemComponent");
        EXPECT_EQ(reflected->m_version, 0);
    }

#if defined(IMGUI_ENABLED)
    TEST_F(RenderDebugTests, ToolsLifecycleDisconnectsAndReconnectsTheSharedRequestBus)
    {
        EXPECT_FALSE(AtomImGuiToolsRequestBus::HasHandlers());
        AtomImGuiToolsSystemComponent system;
        system.Activate();
        EXPECT_TRUE(AtomImGuiToolsRequestBus::HasHandlers());
        // Selection with autoOpen=false exercises the real handler without drawing a viewport.
        AtomImGuiToolsRequestBus::Broadcast(
            &AtomImGuiToolsRequests::ShowMaterialShaderDetailsForEntity, AZ::EntityId(91), false);
        system.Deactivate();
        EXPECT_FALSE(AtomImGuiToolsRequestBus::HasHandlers());
        system.Activate();
        EXPECT_TRUE(AtomImGuiToolsRequestBus::HasHandlers());
        system.Deactivate();
        EXPECT_FALSE(AtomImGuiToolsRequestBus::HasHandlers());
    }
#endif

    TEST_F(RenderDebugTests, MaterialDetailsRoutesDrawPacketsBySelectedEntityAndHandlesDisconnection)
    {
        class MeshCallbacks final : public AtomImGuiMeshCallbackBus::Handler
        {
        public:
            explicit MeshCallbacks(AZ::EntityId id) { BusConnect(id); }
            ~MeshCallbacks() override { BusDisconnect(); }
            const JE::Render::RPI::MeshDrawPacketLods* GetDrawPackets() const override { return &m_packets; }
            JE::Render::RPI::MeshDrawPacketLods m_packets;
        };

        MaterialShaderDetailsController controller;
        controller.SetSelectedEntityId(AZ::EntityId(999));
        EXPECT_EQ(controller.GetMeshDrawPackets(), nullptr);
        MeshCallbacks first(AZ::EntityId(101));
        {
            MeshCallbacks second(AZ::EntityId(102));
            controller.SetSelectedEntityId(AZ::EntityId(101));
            EXPECT_EQ(controller.GetMeshDrawPackets(), &first.m_packets);
            controller.SetSelectedEntityId(AZ::EntityId(102));
            EXPECT_EQ(controller.GetMeshDrawPackets(), &second.m_packets);
        }
        EXPECT_EQ(controller.GetMeshDrawPackets(), nullptr);
        controller.SetSelectedEntityId(AZ::EntityId(101));
        EXPECT_EQ(controller.GetMeshDrawPackets(), &first.m_packets);
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
