/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include "WhiteBoxTestFixtures.h"
#include "Asset/WhiteBoxMeshAsset.h"
#include <JE/Geometry/GeometryModule.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

TEST(GeometryNativeModule, AuthoringFactoryRegistersSixOriginalTypesWithoutDuplicatingRuntimeService)
{
    AZStd::unique_ptr<AZ::Module> module(JE::Geometry::CreateAuthoringModule());
    ASSERT_NE(module, nullptr);
    AZStd::unordered_set<AZ::Uuid> expected{
        AZ::Uuid("{BD393FD9-CF47-433D-B171-C44FE2F7069F}"),
        AZ::Uuid("{B60C4D82-3299-414A-B91B-0299AA51BEF6}"),
        AZ::Uuid("{6CFD4D82-FA68-4C18-BE67-43FC2B755B64}"),
        AZ::Uuid("{42D40E84-A8C4-474B-A4D6-B665CCEA8A83}"),
        AZ::Uuid("{C9F2D913-E275-49BB-AB4F-2D221C16170A}"),
        AZ::Uuid("{4EF53472-6ED4-4740-B956-F6AE5B4A4BB1}")};
    const auto descriptors = module->GetComponentDescriptors();
    ASSERT_EQ(descriptors.size(), 6);
    for (auto* descriptor : descriptors)
    {
        ASSERT_NE(descriptor, nullptr);
        EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
        if (descriptor->GetUuid() == AZ::Uuid("{42D40E84-A8C4-474B-A4D6-B665CCEA8A83}"))
        {
            AZ::ComponentDescriptor::DependencyArrayType provided, dependent;
            descriptor->GetProvidedServices(provided, nullptr);
            descriptor->GetDependentServices(dependent, nullptr);
            EXPECT_NE(AZStd::find(provided.begin(), provided.end(), AZ_CRC_CE("WhiteBoxService")), provided.end());
            EXPECT_NE(AZStd::find(dependent.begin(), dependent.end(), AZ_CRC_CE("AssetDatabaseService")), dependent.end());
        }
    }
    EXPECT_TRUE(expected.empty());
    const auto required = module->GetRequiredSystemComponents();
    ASSERT_EQ(required.size(), 1);
    EXPECT_EQ(required.front(), AZ::Uuid("{42D40E84-A8C4-474B-A4D6-B665CCEA8A83}"));
    EXPECT_EQ(azrtti_typeid<JE::Geometry::Pipeline::WhiteBoxMeshAsset>(),
        AZ::Uuid("{6784304A-4ED6-42FD-A5C9-316265F071F2}"));
}

namespace UnitTest
{
    TEST_F(EditorWhiteBoxComponentTestFixture, NativeGeometryLoadsAndRoundTripsLegacyPrefabComponentTypeName)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(context, nullptr);
        const auto* classData = context->FindClassData(AZ::Uuid("{C9F2D913-E275-49BB-AB4F-2D221C16170A}"));
        ASSERT_NE(classData, nullptr);
        EXPECT_STREQ(classData->m_name, "EditorWhiteBoxComponent");

        rapidjson::Document legacy;
        legacy.Parse(R"({"$type":"EditorWhiteBoxComponent","Material":{"Tint":[0.25,0.5,0.75],"UseTexture":false},"ManualUVs":true})");
        ASSERT_FALSE(legacy.HasParseError());
        AZ::JsonDeserializerSettings loadSettings;
        loadSettings.m_serializeContext = context;
        const AZ::Uuid componentBaseType = azrtti_typeid<AZ::Component>();
        AZ::Uuid decodedType;
        const auto resolved = AZ::JsonSerialization::LoadTypeId(
            decodedType, legacy["$type"], &componentBaseType, "/$type", loadSettings);
        ASSERT_NE(resolved.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        ASSERT_EQ(decodedType, AZ::Uuid("{C9F2D913-E275-49BB-AB4F-2D221C16170A}"));
        const auto* decodedClass = context->FindClassData(decodedType);
        ASSERT_NE(decodedClass, nullptr);
        ASSERT_NE(decodedClass->m_factory, nullptr);
        AZStd::unique_ptr<JE::Geometry::EditorWhiteBoxComponent> owned(
            static_cast<JE::Geometry::EditorWhiteBoxComponent*>(decodedClass->m_factory->Create(decodedClass->m_name)));
        ASSERT_NE(owned, nullptr);
        AZ::Component* decoded = owned.get();
        const auto loaded = AZ::JsonSerialization::Load(owned.get(), decodedType, legacy, loadSettings);
        ASSERT_NE(loaded.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        ASSERT_NE(azrtti_cast<JE::Geometry::EditorWhiteBoxComponent*>(decoded), nullptr);
        EXPECT_EQ(decoded->RTTI_GetType(), AZ::Uuid("{C9F2D913-E275-49BB-AB4F-2D221C16170A}"));

        rapidjson::Document saved;
        AZ::JsonSerializerSettings saveSettings;
        saveSettings.m_serializeContext = context;
        saveSettings.m_keepDefaults = true;
        const auto stored = AZ::JsonSerialization::Store(saved, saved.GetAllocator(), owned.get(), nullptr, decodedType, saveSettings);
        ASSERT_NE(stored.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        ASSERT_TRUE(saved.HasMember("Material"));
        ASSERT_TRUE(saved["Material"].HasMember("UseTexture"));
        EXPECT_FALSE(saved["Material"]["UseTexture"].GetBool());
        ASSERT_TRUE(saved.HasMember("ManualUVs"));
        EXPECT_TRUE(saved["ManualUVs"].GetBool());

        rapidjson::Value savedType;
        const auto typeStored = AZ::JsonSerialization::StoreTypeId(
            savedType, saved.GetAllocator(), decodedType, "/$type", saveSettings);
        ASSERT_NE(typeStored.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        ASSERT_TRUE(savedType.IsString());
        EXPECT_STREQ(savedType.GetString(), "EditorWhiteBoxComponent");
        saved.AddMember("$type", savedType, saved.GetAllocator());
        AZ::Uuid reopenedType;
        const auto typeReloaded = AZ::JsonSerialization::LoadTypeId(
            reopenedType, saved["$type"], &componentBaseType, "/$type", loadSettings);
        ASSERT_NE(typeReloaded.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        ASSERT_EQ(reopenedType, decodedType);
        const auto* reopenedClass = context->FindClassData(reopenedType);
        ASSERT_NE(reopenedClass, nullptr);
        ASSERT_NE(reopenedClass->m_factory, nullptr);
        AZStd::unique_ptr<JE::Geometry::EditorWhiteBoxComponent> reopenedOwned(
            static_cast<JE::Geometry::EditorWhiteBoxComponent*>(reopenedClass->m_factory->Create(reopenedClass->m_name)));
        ASSERT_NE(reopenedOwned, nullptr);
        AZ::Component* reopened = reopenedOwned.get();
        const auto reloaded = AZ::JsonSerialization::Load(reopenedOwned.get(), reopenedType, saved, loadSettings);
        ASSERT_NE(reloaded.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        ASSERT_NE(azrtti_cast<JE::Geometry::EditorWhiteBoxComponent*>(reopened), nullptr);
        EXPECT_EQ(reopened->RTTI_GetType(), decoded->RTTI_GetType());
    }
}