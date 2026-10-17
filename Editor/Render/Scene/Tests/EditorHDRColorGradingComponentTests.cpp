/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ColorGrading/EditorHDRColorGradingComponent.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    class EditorHDRColorGradingComponentTests : public ToolsApplicationFixture<false>
    {
    protected:
        using Component = JE::Editor::Render::Scene::EditorHDRColorGradingComponent;
        using Shaper = AZ::Render::ShaperPresetType;

        void SetUpEditorFixtureImpl() override
        {
            m_runtimeDescriptor.reset(JE::Render::Scene::HDRColorGradingComponent::CreateDescriptor());
            m_runtimeDescriptor->Reflect(Context());
            m_editorDescriptor.reset(Component::CreateDescriptor());
            m_editorDescriptor->Reflect(Context());
        }

        void TearDownEditorFixtureImpl() override
        {
            Context()->EnableRemoveReflection();
            m_editorDescriptor->Reflect(Context());
            m_runtimeDescriptor->Reflect(Context());
            Context()->DisableRemoveReflection();
            m_editorDescriptor.reset();
            m_runtimeDescriptor.reset();
        }

        AZ::SerializeContext* Context()
        {
            return GetApplication()->GetSerializeContext();
        }

        // Arrange/read the serialized snapshot through its real reflection; no private API or alternate writer.
        template<class T>
        T* Field(Component& component, const char* name)
        {
            const auto* data = Context()->FindClassData(azrtti_typeid<Component>());
            if (data)
            {
                for (const auto& field : data->m_elements)
                {
                    if (AZStd::string_view(field.m_name) == name &&
                        field.m_typeId == AZ::SerializeGenericTypeInfo<T>::GetClassTypeId() && field.m_dataSize == sizeof(T))
                    {
                        return reinterpret_cast<T*>(reinterpret_cast<AZ::u8*>(&component) + field.m_offset);
                    }
                }
            }
            ADD_FAILURE() << "Missing reflected LUT snapshot field: " << name;
            return nullptr;
        }

        bool SeedSnapshot(Component& component, Shaper shaper, float minimum, float maximum)
        {
            auto* path = Field<AZStd::string>(component, "generatedLut");
            auto* valid = Field<bool>(component, "generatedLutMetadataValid");
            auto* type = Field<Shaper>(component, "generatedLutShaper");
            auto* low = Field<float>(component, "generatedLutMinimumExposure");
            auto* high = Field<float>(component, "generatedLutMaximumExposure");
            if (!path || !valid || !type || !low || !high)
            {
                return false;
            }
            *path = "LutGeneration/SavedLut_snapshot.azasset";
            *valid = true;
            *type = shaper;
            *low = minimum;
            *high = maximum;
            return true;
        }

        void ExpectSnapshot(Component& component, Shaper shaper, float minimum, float maximum)
        {
            auto* path = Field<AZStd::string>(component, "generatedLut");
            auto* valid = Field<bool>(component, "generatedLutMetadataValid");
            auto* type = Field<Shaper>(component, "generatedLutShaper");
            auto* low = Field<float>(component, "generatedLutMinimumExposure");
            auto* high = Field<float>(component, "generatedLutMaximumExposure");
            ASSERT_TRUE(path && valid && type && low && high);
            EXPECT_EQ(*path, "LutGeneration/SavedLut_snapshot.azasset");
            EXPECT_TRUE(*valid);
            EXPECT_EQ(*type, shaper);
            EXPECT_FLOAT_EQ(*low, minimum);
            EXPECT_FLOAT_EQ(*high, maximum);
        }

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_runtimeDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorDescriptor;
    };

    TEST_F(EditorHDRColorGradingComponentTests, LegacyGeneratedPathRemainsButEncodingIsUnknown)
    {
        Component source;
        ASSERT_TRUE(SeedSnapshot(source, Shaper::None, -4.0f, 8.0f));
        AZ::SerializeContext::DataElementNode root;
        const int index = root.AddElementWithData(*Context(), "component", source);
        ASSERT_GE(index, 0);
        auto& node = root.GetSubElement(index);
        for (const char* name : {"generatedLutMetadataValid", "generatedLutShaper",
            "generatedLutMinimumExposure", "generatedLutMaximumExposure"})
        {
            const int field = node.FindElement(AZ::Crc32(name));
            ASSERT_GE(field, 0);
            node.RemoveElement(field);
        }
        node.SetVersion(2);
        const auto* data = Context()->FindClassData(azrtti_typeid<Component>());
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(data->m_version, 3u);
        ASSERT_TRUE(data->m_converter);
        ASSERT_TRUE(data->m_converter(*Context(), node));
        AZStd::string path;
        bool valid = true;
        ASSERT_TRUE(node.GetChildData(AZ_CRC_CE("generatedLut"), path));
        ASSERT_TRUE(node.GetChildData(AZ_CRC_CE("generatedLutMetadataValid"), valid));
        EXPECT_EQ(path, "LutGeneration/SavedLut_snapshot.azasset");
        EXPECT_FALSE(valid); // Do not infer Log2 or None for a historical LUT.
    }

    TEST_F(EditorHDRColorGradingComponentTests, GeneratedEncodingAndPathSurviveObjectStreamRoundtrip)
    {
        for (const Shaper shaper : {Shaper::None, Shaper::Log2CustomRange})
        {
            Component source;
            ASSERT_TRUE(SeedSnapshot(source, shaper, -4.0f, 8.0f));
            AZStd::vector<char> bytes;
            AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&bytes);
            ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, &source, Context()));
            AZStd::unique_ptr<Component> reopened(
                AZ::Utils::LoadObjectFromBuffer<Component>(bytes.data(), bytes.size(), Context()));
            ASSERT_NE(reopened, nullptr);
            ExpectSnapshot(*reopened, shaper, -4.0f, 8.0f);
        }
    }

    TEST_F(EditorHDRColorGradingComponentTests, LaterHdrConfigurationDoesNotChangeGeneratedEncoding)
    {
        Component source;
        ASSERT_TRUE(SeedSnapshot(source, Shaper::None, -4.0f, 8.0f));
        JE::Render::Scene::HDRColorGradingComponentConfig current;
        current.m_shaperPresetType = Shaper::Log2_48Nits;
        current.m_customMinExposure = -10.0f;
        current.m_customMaxExposure = 12.0f;
        ASSERT_TRUE(source.SetConfiguration(current));
        JE::Render::Scene::HDRColorGradingComponentConfig applied;
        ASSERT_TRUE(source.GetConfiguration(applied));
        EXPECT_EQ(applied.m_shaperPresetType, Shaper::Log2_48Nits);
        EXPECT_FLOAT_EQ(applied.m_customMinExposure, -10.0f);
        EXPECT_FLOAT_EQ(applied.m_customMaxExposure, 12.0f);
        ExpectSnapshot(source, Shaper::None, -4.0f, 8.0f);
    }
}
