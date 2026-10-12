/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <JE/Render/Materials/MaterialComponent.h>
#include <JE/Editor/Materials/EditorMaterialComponent.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponent.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class MaterialsCompatibilityTests : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_ownsNameDictionary = !AZ::NameDictionary::IsReady();
            if (m_ownsNameDictionary)
            {
                AZ::NameDictionary::Create();
            }
            m_context = AZStd::make_unique<AZ::SerializeContext>();
            AZ::Entity::Reflect(m_context.get());
            AZ::Name::Reflect(m_context.get());
            AZ::Data::AssetId::Reflect(m_context.get());
            AZ::RHI::ShaderSemantic::Reflect(m_context.get());
            // Exercise the component's real reflection entry, including assignment/config ordering.
            JE::Render::MaterialComponent::Reflect(m_context.get());
            AzToolsFramework::Components::EditorComponentBase::Reflect(m_context.get());
            JE::Editor::EditorMaterialComponent::Reflect(m_context.get());
        }

        void TearDown() override
        {
            m_context.reset();
            if (m_ownsNameDictionary)
            {
                AZ::NameDictionary::Destroy();
            }
        }

        JE::Render::MaterialAssignment MakeAssignment() const
        {
            JE::Render::MaterialAssignment assignment;
            assignment.m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(
                AZ::Data::AssetId(AZ::Uuid::CreateString("{12121212-3434-5656-7878-909090909090}"), 19),
                azrtti_typeid<AZ::RPI::MaterialAsset>());
            assignment.m_materialAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
            assignment.m_propertyOverrides[AZ::Name("baseColor.color")] = AZ::Color(0.25f, 0.5f, 0.75f, 1.0f);
            assignment.m_propertyOverrides[AZ::Name("roughness.factor")] = 0.375f;
            assignment.m_matModUvOverrides[AZ::RHI::ShaderSemantic("UV", 1)] = AZ::Name("authored_uv");
            return assignment;
        }

        JE::Render::MaterialComponentConfig MakeConfig() const
        {
            JE::Render::MaterialComponentConfig config;
            config.m_materials.emplace(JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(2, 41), MakeAssignment());
            config.m_materialsByLabel.emplace("room.wall", MakeAssignment());
            return config;
        }

        void ExpectAssignment(const JE::Render::MaterialAssignment& actual) const
        {
            const auto expected = MakeAssignment();
            EXPECT_EQ(actual.m_materialAsset.GetId(), expected.m_materialAsset.GetId());
            EXPECT_EQ(actual.m_materialAsset.GetType(), expected.m_materialAsset.GetType());
            ASSERT_EQ(actual.m_propertyOverrides.size(), 2);
            const auto tint = actual.m_propertyOverrides.find(AZ::Name("baseColor.color"));
            ASSERT_NE(tint, actual.m_propertyOverrides.end());
            const auto* color = AZStd::any_cast<AZ::Color>(&tint->second);
            ASSERT_NE(color, nullptr);
            EXPECT_EQ(*color, AZ::Color(0.25f, 0.5f, 0.75f, 1.0f));
            const auto roughness = actual.m_propertyOverrides.find(AZ::Name("roughness.factor"));
            ASSERT_NE(roughness, actual.m_propertyOverrides.end());
            const auto* factor = AZStd::any_cast<float>(&roughness->second);
            ASSERT_NE(factor, nullptr);
            EXPECT_EQ(*factor, 0.375f);
            ASSERT_EQ(actual.m_matModUvOverrides.size(), 1);
            const auto uv = actual.m_matModUvOverrides.find(AZ::RHI::ShaderSemantic("UV", 1));
            ASSERT_NE(uv, actual.m_matModUvOverrides.end());
            EXPECT_EQ(uv->second, AZ::Name("authored_uv"));
        }

        void ExpectConfig(const JE::Render::MaterialComponentConfig& actual) const
        {
            ASSERT_EQ(actual.m_materials.size(), 1);
            const auto slot = actual.m_materials.find(JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(2, 41));
            ASSERT_NE(slot, actual.m_materials.end());
            ExpectAssignment(slot->second);
            ASSERT_EQ(actual.m_materialsByLabel.size(), 1);
            const auto label = actual.m_materialsByLabel.find("room.wall");
            ASSERT_NE(label, actual.m_materialsByLabel.end());
            ExpectAssignment(label->second);
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_context;
        bool m_ownsNameDictionary = false;
    };

    TEST_F(MaterialsCompatibilityTests, LegacyAssignmentAndIdXmlNamesAndUuidsDeserialize)
    {
        const auto assignment = MakeAssignment();
        const auto id = JE::Render::MaterialAssignmentId::CreateFromLodAndStableId(2, 41);
        AZStd::vector<char> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_XML, &assignment, m_context.get()));
        const AZStd::string xml(bytes.begin(), bytes.end());
        EXPECT_NE(xml.find("AZ::Render::MaterialAssignment"), AZStd::string::npos);
        EXPECT_NE(xml.find("{C66E5214-A24B-4722-B7F0-5991E6F8F163}"), AZStd::string::npos);
        // These are dummy asset references. Preserve IDs without requesting an asset file or a handler.
        const AZ::ObjectStream::FilterDescriptor referenceOnly{AZ::Data::AssetFilterNoAssetLoading};
        JE::Render::MaterialAssignment restored;
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, restored, m_context.get(), referenceOnly));
        ExpectAssignment(restored);

        AZStd::vector<char> idBytes;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> idStream(&idBytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(idStream, AZ::DataStream::ST_XML, &id, m_context.get()));
        const AZStd::string idXml(idBytes.begin(), idBytes.end());
        EXPECT_NE(idXml.find("AZ::Render::MaterialAssignmentId"), AZStd::string::npos);
        EXPECT_NE(idXml.find("{EB603581-4654-4C17-B6DE-AE61E79EDA97}"), AZStd::string::npos);
        JE::Render::MaterialAssignmentId restoredId;
        idStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(idStream, restoredId, m_context.get()));
        EXPECT_EQ(restoredId, id);
        EXPECT_EQ(restoredId.m_lodIndex, 2);
        EXPECT_EQ(restoredId.m_materialSlotStableId, 41);
    }

    TEST_F(MaterialsCompatibilityTests, LegacyConfigXmlRetainsReferencesPropertiesUvsAndLabels)
    {
        const auto config = MakeConfig();
        AZStd::vector<char> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_XML, &config, m_context.get()));
        const AZStd::string xml(bytes.begin(), bytes.end());
        EXPECT_NE(xml.find("MaterialComponentConfig"), AZStd::string::npos);
        EXPECT_NE(xml.find("{3366C279-32AE-48F6-839B-7700AE117A54}"), AZStd::string::npos);
        EXPECT_NE(xml.find("AZ::Render::MaterialAssignmentId"), AZStd::string::npos);
        EXPECT_NE(xml.find("AZ::Render::MaterialAssignment"), AZStd::string::npos);
        JE::Render::MaterialComponentConfig restored;
        const AZ::ObjectStream::FilterDescriptor referenceOnly{AZ::Data::AssetFilterNoAssetLoading};
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, restored, m_context.get(), referenceOnly));
        ExpectConfig(restored);
    }

    TEST_F(MaterialsCompatibilityTests, BuildGameEntityPreservesConfigWithoutEditorMaterialSystem)
    {
        EXPECT_EQ(m_context->FindClassData(azrtti_typeid<JE::Editor::EditorMaterialSystemComponent>()), nullptr);
        const auto config = MakeConfig();
        JE::Editor::EditorMaterialComponent editor(config);
        AZ::Entity runtimeEntity("material-runtime-export");
        // This is the original component export path used by prefab builders; no activation/preview is needed.
        editor.BuildGameEntity(&runtimeEntity);
        const auto* runtime = runtimeEntity.FindComponent<JE::Render::MaterialComponent>();
        ASSERT_NE(runtime, nullptr);
        JE::Render::MaterialComponentConfig exported;
        ASSERT_TRUE(runtime->GetConfiguration(exported));
        ExpectConfig(exported);
        EXPECT_EQ(runtimeEntity.GetComponents().size(), 1);
    }
}
