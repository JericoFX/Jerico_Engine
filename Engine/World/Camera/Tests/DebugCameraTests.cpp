/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/World/Camera/Debug/DebugCameraModule.h>
#include <JE/World/Camera/Debug/CameraComponent.h>
#include <JE/World/Camera/Debug/ArcBallControllerComponent.h>
#include <JE/World/Camera/Debug/NoClipControllerComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>

namespace JE::World::Camera::Debug
{
    class TestTransformComponent : public AzFramework::TransformComponent
    {
    public:
        AZ_TYPE_INFO(TestTransformComponent, "{B6DB69DE-A892-48B5-BBAC-FBC6EECF7BC4}");
        using AzFramework::TransformComponent::Activate;
        using AzFramework::TransformComponent::Deactivate;
    };

    class NativeDebugCameraTests : public UnitTest::LeakDetectionFixture
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

    TEST_F(NativeDebugCameraTests, FactoryPreservesFourDescriptorsAndAddsNoCameraSystem)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateDebugModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{C4F5D301-5C7F-42C2-8326-08F685B2D7A3}"));
        AZStd::unordered_set<AZ::Uuid> expected{
            AZ::Uuid("{A3503719-6DE2-46D0-A54B-922155F4537F}"),
            AZ::Uuid("{3CCDE644-2798-4A58-992C-1C420599FCEE}"),
            AZ::Uuid("{2BAFDA24-B354-4C5C-95BE-D7254B4BD415}"),
            AZ::Uuid("{FDDF608A-7866-4886-87E5-6F02899C6C4D}") };
        ASSERT_EQ(module->GetComponentDescriptors().size(), 4);
        for (const auto* descriptor : module->GetComponentDescriptors())
        {
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
        }
        EXPECT_TRUE(expected.empty());
        EXPECT_TRUE(module->GetRequiredSystemComponents().empty());
    }

    TEST_F(NativeDebugCameraTests, CameraConfigurationRoundTripPreservesLegacyNameAndProjection)
    {
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        CameraComponent::Reflect(&context);
        const auto* data = context.FindClassData(AZ::Uuid("{2BAFDA24-B354-4C5C-95BE-D7254B4BD415}"));
        ASSERT_NE(data, nullptr);
        EXPECT_STREQ(data->m_name, "CameraComponent");
        CameraComponentConfig config;
        config.m_fovY = 0.95f;
        config.m_depthNear = 0.25f;
        config.m_depthFar = 750.0f;
        config.m_aspectRatioOverride = 1.75f;
        CameraComponent original;
        ASSERT_TRUE(original.SetConfiguration(config));
        AZStd::vector<AZ::u8> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, &original, &context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZStd::unique_ptr<CameraComponent> reopened(AZ::Utils::LoadObjectFromStream<CameraComponent>(stream, &context));
        ASSERT_NE(reopened, nullptr);
        CameraComponentConfig actual;
        ASSERT_TRUE(reopened->GetConfiguration(actual));
        EXPECT_EQ(actual.m_fovY, config.m_fovY);
        EXPECT_EQ(actual.m_depthNear, config.m_depthNear);
        EXPECT_EQ(actual.m_depthFar, config.m_depthFar);
        EXPECT_EQ(actual.m_aspectRatioOverride, config.m_aspectRatioOverride);
        EXPECT_EQ(actual.m_target, nullptr);
    }

    TEST_F(NativeDebugCameraTests, PreviewCameraProjectionUsesExistingCameraBusWithoutTakingActiveView)
    {
        CameraComponent camera;
        CameraComponentConfig config;
        config.m_fovY = AZ::Constants::HalfPi;
        config.m_depthFar = 10.0f;
        config.m_aspectRatioOverride = 2.0f;
        ASSERT_TRUE(camera.SetConfiguration(config));
        const AZ::EntityId id(911);
        // Exercise the real projection API without creating a GPU scene or activating an RPI pipeline.
        auto& handler = static_cast<::Camera::CameraRequestBus::Handler&>(camera);
        handler.BusConnect(id);
        float width = 0.0f;
        float height = 0.0f;
        ::Camera::CameraRequestBus::EventResult(width, id, &::Camera::CameraRequestBus::Events::GetFrustumWidth);
        ::Camera::CameraRequestBus::EventResult(height, id, &::Camera::CameraRequestBus::Events::GetFrustumHeight);
        EXPECT_NEAR(width, 40.0f, 0.0001f);
        EXPECT_NEAR(height, 20.0f, 0.0001f);
        ::Camera::CameraRequestBus::Event(id, &::Camera::CameraRequestBus::Events::SetFrustumHeight, 10.0f);
        ::Camera::CameraRequestBus::EventResult(height, id, &::Camera::CameraRequestBus::Events::GetFrustumHeight);
        EXPECT_NEAR(height, 10.0f, 0.0001f);
        bool active = true;
        ::Camera::CameraRequestBus::Event(id, &::Camera::CameraRequestBus::Events::MakeActiveView);
        ::Camera::CameraRequestBus::EventResult(active, id, &::Camera::CameraRequestBus::Events::IsActiveView);
        EXPECT_FALSE(active);
        handler.BusDisconnect();
        EXPECT_FALSE(::Camera::CameraRequestBus::HasHandlers(id));
    }

    TEST_F(NativeDebugCameraTests, NoClipPropertiesRoundTripPreservesAllFiveValues)
    {
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        CameraControllerComponent::Reflect(&context);
        NoClipControllerComponent::Reflect(&context);
        NoClipControllerProperties original;
        original.m_mouseSensitivityX = 1.25f;
        original.m_mouseSensitivityY = 2.5f;
        original.m_moveSpeed = 3.75f;
        original.m_panningSpeed = 4.25f;
        original.m_touchSensitivity = 5.5f;
        AZStd::vector<AZ::u8> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, &original, &context));
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        NoClipControllerProperties actual;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, actual, &context));
        EXPECT_EQ(azrtti_typeid<NoClipControllerProperties>(), AZ::Uuid("{65F7E522-7FDE-414E-AA0F-638B234699B8}"));
        EXPECT_EQ(actual.m_mouseSensitivityX, original.m_mouseSensitivityX);
        EXPECT_EQ(actual.m_mouseSensitivityY, original.m_mouseSensitivityY);
        EXPECT_EQ(actual.m_moveSpeed, original.m_moveSpeed);
        EXPECT_EQ(actual.m_panningSpeed, original.m_panningSpeed);
        EXPECT_EQ(actual.m_touchSensitivity, original.m_touchSensitivity);
    }

    TEST_F(NativeDebugCameraTests, ControllerSelectionAndResetUseOneEntityAndDisconnectOnDeactivate)
    {
        AZ::Entity entity("Native debug controllers");
        auto* arcball = entity.CreateComponent<ArcBallControllerComponent>();
        auto* noclip = entity.CreateComponent<NoClipControllerComponent>();
        ASSERT_NE(arcball, nullptr);
        ASSERT_NE(noclip, nullptr);
        entity.Init();
        // Activate only controllers: their bus/input lifecycle does not require a second camera or renderer.
        arcball->Activate();
        noclip->Activate();
        CameraControllerRequestBus::Event(entity.GetId(), &CameraControllerRequests::Enable, azrtti_typeid<NoClipControllerComponent>());
        EXPECT_TRUE(NoClipControllerRequestBus::HasHandlers(entity.GetId()));
        EXPECT_FALSE(ArcBallControllerRequestBus::HasHandlers(entity.GetId()));
        NoClipControllerRequestBus::Event(entity.GetId(), &NoClipControllerRequests::SetHeading, 0.8f);
        CameraControllerRequestBus::Event(entity.GetId(), &CameraControllerRequests::Reset);
        float heading = -1.0f;
        NoClipControllerRequestBus::EventResult(heading, entity.GetId(), &NoClipControllerRequests::GetHeading);
        EXPECT_EQ(heading, 0.0f);
        CameraControllerRequestBus::Event(entity.GetId(), &CameraControllerRequests::Enable, azrtti_typeid<ArcBallControllerComponent>());
        EXPECT_TRUE(ArcBallControllerRequestBus::HasHandlers(entity.GetId()));
        EXPECT_FALSE(NoClipControllerRequestBus::HasHandlers(entity.GetId()));
        CameraControllerRequestBus::Event(entity.GetId(), &CameraControllerRequests::Disable);
        EXPECT_FALSE(ArcBallControllerRequestBus::HasHandlers(entity.GetId()));
        arcball->Deactivate();
        noclip->Deactivate();
        EXPECT_FALSE(CameraControllerRequestBus::HasHandlers(entity.GetId()));
    }

    TEST_F(NativeDebugCameraTests, ArcBallTickUpdatesExistingTransformAndHonorsDistanceLimits)
    {
        AZ::Entity entity("Native orbit camera");
        auto* transform = entity.CreateComponent<TestTransformComponent>();
        auto* arcball = entity.CreateComponent<ArcBallControllerComponent>();
        ASSERT_NE(transform, nullptr);
        ASSERT_NE(arcball, nullptr);
        entity.Init();
        transform->Activate();
        arcball->Activate();
        CameraControllerRequestBus::Event(entity.GetId(), &CameraControllerRequests::Enable, azrtti_typeid<ArcBallControllerComponent>());
        ArcBallControllerRequestBus::Event(entity.GetId(), &ArcBallControllerRequests::SetCenter, AZ::Vector3(1.0f, 2.0f, 3.0f));
        ArcBallControllerRequestBus::Event(entity.GetId(), &ArcBallControllerRequests::SetMinDistance, 2.0f);
        ArcBallControllerRequestBus::Event(entity.GetId(), &ArcBallControllerRequests::SetMaxDistance, 6.0f);
        ArcBallControllerRequestBus::Event(entity.GetId(), &ArcBallControllerRequests::SetDistance, 100.0f);
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.016f, AZ::ScriptTimePoint());
        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, entity.GetId(), &AZ::TransformInterface::GetWorldTranslation);
        EXPECT_TRUE(position.IsClose(AZ::Vector3(1.0f, -4.0f, 3.0f)));
        arcball->Deactivate();
        transform->Deactivate();
        EXPECT_FALSE(ArcBallControllerRequestBus::HasHandlers(entity.GetId()));
    }
}
