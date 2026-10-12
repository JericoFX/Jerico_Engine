/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/Camera/CameraModule.h>
#include <JE/Editor/Camera/EditorCameraComponent.h>
#include <JE/World/Camera/CameraComponentConverters.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

TEST(NativeEditorCamera, FactoryPreservesFiveDescriptorsAndBothRequiredSystems)
{
    AZStd::unique_ptr<AZ::Module> module(JE::Editor::Camera::CreateAuthoringModule());
    ASSERT_NE(module, nullptr);
    AZStd::unordered_set<AZ::Uuid> expected{
        AZ::Uuid("{E2DC7EB8-02D1-4E6D-BFE4-CE652FCB7C7F}"),
        AZ::Uuid("{5DF8DB49-6430-4718-9417-85321596EDA5}"),
        AZ::Uuid("{286BF97A-1B4A-4EE1-944F-C13B2396227B}"),
        AZ::Uuid("{CA11DA46-29FF-4083-B5F6-E02C3A8C3A3D}"),
        AZ::Uuid("{769802EB-722A-4F89-A475-DA396DA1FDCC}")};
    const auto descriptors = module->GetComponentDescriptors();
    ASSERT_EQ(descriptors.size(), 5);
    for (const auto* descriptor : descriptors)
    {
        ASSERT_NE(descriptor, nullptr);
        EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
    }
    EXPECT_TRUE(expected.empty());
    const auto required = module->GetRequiredSystemComponents();
    ASSERT_EQ(required.size(), 2);
    EXPECT_EQ(required[0], AZ::Uuid("{5DF8DB49-6430-4718-9417-85321596EDA5}"));
    EXPECT_EQ(required[1], AZ::Uuid("{769802EB-722A-4F89-A475-DA396DA1FDCC}"));
}

namespace UnitTest
{
    class NativeCameraSerializationTests : public LeakDetectionFixture
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
            m_context = AZStd::make_unique<AZ::SerializeContext>();
            AZ::Entity::Reflect(m_context.get());
            AZ::Name::Reflect(m_context.get());
            AZ::Data::AssetId::Reflect(m_context.get());
            JE::World::Camera::CameraComponent::Reflect(m_context.get());
        }
        void ReflectEditor()
        {
            AzToolsFramework::Components::EditorComponentBase::Reflect(m_context.get());
            JE::Editor::Camera::EditorCameraComponent::Reflect(m_context.get());
        }
        void TearDown() override
        {
            if (m_registration)
            {
                m_registration->EnableRemoveReflection();
                AZ::JsonSystemComponent::Reflect(m_registration.get());
                m_registration->DisableRemoveReflection();
                m_registration.reset();
            }
            m_context.reset();
            if (m_ownsNames)
            {
                AZ::NameDictionary::Destroy();
            }
            LeakDetectionFixture::TearDown();
        }
        void CheckLegacyConversion(const char* name, const AZ::Uuid& expected)
        {
            AZ::SerializeContext::DataElementNode node;
            node.SetName(name);
            ASSERT_GE(node.AddElementWithData(*m_context, "Field of View", 52.0f), 0);
            ASSERT_GE(node.AddElementWithData(*m_context, "Near Clip Plane Distance", 0.3f), 0);
            ASSERT_GE(node.AddElementWithData(*m_context, "Far Clip Plane Distance", 700.0f), 0);
            ASSERT_GE(node.AddElementWithData(*m_context, "SpecifyDimensions", true), 0);
            ASSERT_GE(node.AddElementWithData(*m_context, "FrustumWidth", 333.0f), 0);
            ASSERT_GE(node.AddElementWithData(*m_context, "FrustumHeight", 222.0f), 0);
            const auto* deprecated = m_context->FindClassData(AZ::Uuid("{A0C21E18-F759-4E72-AF26-7A36FC59E477}"));
            ASSERT_NE(deprecated, nullptr);
            ASSERT_NE(deprecated->m_converter, nullptr);
            ASSERT_TRUE(deprecated->m_converter(*m_context, node));
            EXPECT_EQ(node.GetId(), expected);
            float fov{}, nearPlane{}, farPlane{}, width{}, height{};
            bool specify{};
            ASSERT_TRUE(node.GetChildData(AZ_CRC_CE("Field of View"), fov));
            ASSERT_TRUE(node.GetChildData(AZ_CRC_CE("Near Clip Plane Distance"), nearPlane));
            ASSERT_TRUE(node.GetChildData(AZ_CRC_CE("Far Clip Plane Distance"), farPlane));
            ASSERT_TRUE(node.GetChildData(AZ_CRC_CE("SpecifyDimensions"), specify));
            ASSERT_TRUE(node.GetChildData(AZ_CRC_CE("FrustumWidth"), width));
            ASSERT_TRUE(node.GetChildData(AZ_CRC_CE("FrustumHeight"), height));
            EXPECT_EQ(fov, 52.0f);
            EXPECT_EQ(nearPlane, 0.3f);
            EXPECT_EQ(farPlane, 700.0f);
            EXPECT_TRUE(specify);
            EXPECT_EQ(width, 333.0f);
            EXPECT_EQ(height, 222.0f);
        }
        AZStd::unique_ptr<AZ::SerializeContext> m_context;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registration;
        bool m_ownsNames{};
    };

    TEST_F(NativeCameraSerializationTests, LegacyTemplateWithoutEditorReflectionRemainsRuntimeCamera)
    {
        CheckLegacyConversion("m_template", CameraComponentTypeId);
    }
    TEST_F(NativeCameraSerializationTests, LegacyTemplateWithEditorReflectionUsesOriginalEditorCameraType)
    {
        ReflectEditor();
        CheckLegacyConversion("m_template", EditorCameraComponentTypeId);
    }
    TEST_F(NativeCameraSerializationTests, LegacyNormalNodeWithEditorReflectionRemainsRuntimeCamera)
    {
        ReflectEditor();
        CheckLegacyConversion("component", CameraComponentTypeId);
    }
    TEST_F(NativeCameraSerializationTests, LegacyPrefabTypeNameAndConfiguredCameraSurviveRoundTrip)
    {
        ReflectEditor();
        rapidjson::Document legacy;
        legacy.Parse(R"({"$type":"EditorCameraComponent"})");
        m_registration = AZStd::make_unique<AZ::JsonRegistrationContext>();
        AZ::JsonSystemComponent::Reflect(m_registration.get());
        AZ::JsonDeserializerSettings loadSettings;
        loadSettings.m_serializeContext = m_context.get();
        loadSettings.m_registrationContext = m_registration.get();
        const auto baseType = azrtti_typeid<AZ::Component>();
        AZ::Uuid type;
        const auto resolved = AZ::JsonSerialization::LoadTypeId(type, legacy["$type"], &baseType, "/$type", loadSettings);
        ASSERT_NE(resolved.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        ASSERT_EQ(type, EditorCameraComponentTypeId);
        const auto* data = m_context->FindClassData(type);
        ASSERT_NE(data, nullptr);
        EXPECT_STREQ(data->m_name, "EditorCameraComponent");
        ASSERT_NE(data->m_factory, nullptr);
        AZStd::unique_ptr<JE::Editor::Camera::EditorCameraComponent> original(
            static_cast<JE::Editor::Camera::EditorCameraComponent*>(data->m_factory->Create(data->m_name)));
        ASSERT_NE(original, nullptr);
        const auto loaded = AZ::JsonSerialization::Load(original.get(), type, legacy, loadSettings);
        ASSERT_NE(loaded.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        JE::World::Camera::CameraComponentConfig config;
        ASSERT_TRUE(original->GetConfiguration(config));
        config.m_fov = 61.0f;
        config.m_nearClipDistance = 0.25f;
        config.m_farClipDistance = 900.0f;
        config.m_orthographic = true;
        config.m_orthographicHalfWidth = 7.5f;
        config.m_makeActiveViewOnActivation = false;
        ASSERT_TRUE(original->SetConfiguration(config));
        rapidjson::Document saved;
        AZ::JsonSerializerSettings storeSettings;
        storeSettings.m_serializeContext = m_context.get();
        storeSettings.m_registrationContext = m_registration.get();
        storeSettings.m_keepDefaults = true;
        const auto stored = AZ::JsonSerialization::Store(saved, saved.GetAllocator(), original.get(), nullptr, type, storeSettings);
        ASSERT_NE(stored.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        JE::Editor::Camera::EditorCameraComponent reopened;
        const auto reloaded = AZ::JsonSerialization::Load(&reopened, type, saved, loadSettings);
        ASSERT_NE(reloaded.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        JE::World::Camera::CameraComponentConfig actual;
        ASSERT_TRUE(reopened.GetConfiguration(actual));
        EXPECT_EQ(actual.m_fov, config.m_fov);
        EXPECT_EQ(actual.m_nearClipDistance, config.m_nearClipDistance);
        EXPECT_EQ(actual.m_farClipDistance, config.m_farClipDistance);
        EXPECT_EQ(actual.m_orthographicHalfWidth, config.m_orthographicHalfWidth);
        EXPECT_TRUE(actual.m_orthographic);
        EXPECT_FALSE(actual.m_makeActiveViewOnActivation);
        EXPECT_EQ(reopened.RTTI_GetType(), type);
    }
}
