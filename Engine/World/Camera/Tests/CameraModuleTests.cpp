/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/World/Camera/CameraModule.h>
#include <JE/World/Camera/CameraComponent.h>
#include <JE/World/Camera/CameraSystemComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

TEST(NativeWorldCamera, FactoryPreservesThreeComponentIdentitiesAndRequiredCameraService)
{
    AZStd::unique_ptr<AZ::Module> module(JE::World::Camera::CreateRuntimeModule());
    ASSERT_NE(module, nullptr);
    AZStd::unordered_set<AZ::Uuid> expected{
        AZ::Uuid("{E2DC7EB8-02D1-4E6D-BFE4-CE652FCB7C7F}"),
        AZ::Uuid("{5DF8DB49-6430-4718-9417-85321596EDA5}"),
        AZ::Uuid("{286BF97A-1B4A-4EE1-944F-C13B2396227B}")};
    const auto descriptors = module->GetComponentDescriptors();
    ASSERT_EQ(descriptors.size(), 3);
    for (const auto* descriptor : descriptors)
    {
        ASSERT_NE(descriptor, nullptr);
        EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
    }
    EXPECT_TRUE(expected.empty());
    const auto required = module->GetRequiredSystemComponents();
    ASSERT_EQ(required.size(), 1);
    EXPECT_EQ(required.front(), AZ::Uuid("{5DF8DB49-6430-4718-9417-85321596EDA5}"));
}

TEST(NativeWorldCamera, ExistingActiveCameraBusTracksViewChangesAndDisconnects)
{
    JE::World::Camera::CameraSystemComponent system;
    system.Activate();
    const AZ::EntityId camera(91);
    ::Camera::CameraNotificationBus::Broadcast(&::Camera::CameraNotifications::OnActiveViewChanged, camera);
    AZ::EntityId active;
    ::Camera::CameraSystemRequestBus::BroadcastResult(active, &::Camera::CameraSystemRequests::GetActiveCamera);
    EXPECT_EQ(active, camera);
    ::Camera::CameraNotificationBus::Broadcast(&::Camera::CameraNotifications::OnActiveViewChanged, AZ::EntityId());
    ::Camera::CameraSystemRequestBus::BroadcastResult(active, &::Camera::CameraSystemRequests::GetActiveCamera);
    EXPECT_FALSE(active.IsValid());
    system.Deactivate();
    EXPECT_FALSE(::Camera::CameraSystemRequestBus::HasHandlers());
}

class NativeWorldCameraSerializationTests : public UnitTest::LeakDetectionFixture
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

TEST_F(NativeWorldCameraSerializationTests, RuntimeConfigurationRoundTripPreservesProjectionAndActivation)
{
    {
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        AZ::Name::Reflect(&context);
        AZ::Data::AssetId::Reflect(&context);
        JE::World::Camera::CameraComponent::Reflect(&context);
        JE::World::Camera::CameraComponentConfig config;
        config.m_fov = 57.0f;
        config.m_nearClipDistance = 0.125f;
        config.m_farClipDistance = 600.0f;
        config.m_orthographic = true;
        config.m_orthographicHalfWidth = 9.0f;
        config.m_makeActiveViewOnActivation = false;
        JE::World::Camera::CameraComponent original(config);
        AZStd::vector<AZ::u8> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, &original, &context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZStd::unique_ptr<JE::World::Camera::CameraComponent> reopened(
            AZ::Utils::LoadObjectFromStream<JE::World::Camera::CameraComponent>(stream, &context));
        ASSERT_NE(reopened, nullptr);
        JE::World::Camera::CameraComponentConfig actual;
        ASSERT_TRUE(reopened->GetConfiguration(actual));
        EXPECT_EQ(actual.m_fov, config.m_fov);
        EXPECT_EQ(actual.m_nearClipDistance, config.m_nearClipDistance);
        EXPECT_EQ(actual.m_farClipDistance, config.m_farClipDistance);
        EXPECT_EQ(actual.m_orthographicHalfWidth, config.m_orthographicHalfWidth);
        EXPECT_TRUE(actual.m_orthographic);
        EXPECT_FALSE(actual.m_makeActiveViewOnActivation);
        EXPECT_EQ(reopened->RTTI_GetType(), AZ::Uuid("{E2DC7EB8-02D1-4E6D-BFE4-CE652FCB7C7F}"));
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
