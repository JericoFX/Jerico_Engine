/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>
#include <JE/Editor/RenderIntegration/AssetProcessingModule.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzTest/AzTest.h>

namespace JE::Editor::RenderIntegration
{
    // Clean reflected generic types while the test allocator owner is still alive.
    class EditorRenderIntegration : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(EditorRenderIntegration, AuthoringFactoryPreservesRuntimeDescriptorsExactlyOnceAndEditorSchema)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateAuthoringModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{7B330394-BE9C-4BDA-9345-1A0859815982}"));
        AZStd::unordered_set<AZ::Uuid> expected{
            AZ::Uuid("{7AE0D6AD-691C-41B6-9DD5-F23F78B1A02E}"),
            AZ::Uuid("{FFB99CE4-2C9E-476D-8140-50A8A696E242}"),
            AZ::Uuid("{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}") };
        const auto descriptors = module->GetComponentDescriptors();
        ASSERT_EQ(descriptors.size(), 3);
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        AzToolsFramework::Components::EditorComponentBase::Reflect(&context);
        for (auto* descriptor : descriptors)
        {
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
            descriptor->Reflect(&context);
        }
        EXPECT_TRUE(expected.empty());
        const auto required = module->GetRequiredSystemComponents();
        ASSERT_EQ(required.size(), 1);
        EXPECT_EQ(required.front(), AZ::Uuid("{FFB99CE4-2C9E-476D-8140-50A8A696E242}"));
        const auto* editor = context.FindClassData(AZ::Uuid("{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}"));
        ASSERT_NE(editor, nullptr);
        EXPECT_STREQ(editor->m_name, "AssetCollectionAsyncLoaderTestComponent");
        EXPECT_EQ(editor->m_version, 1);
        bool hasAssetList = false;
        for (const auto& element : editor->m_elements)
        {
            if (element.m_nameCrc == AZ_CRC_CE("AssetListPathJson"))
            {
                hasAssetList = true;
                EXPECT_EQ(element.m_typeId, azrtti_typeid<AZStd::string>());
            }
        }
        EXPECT_TRUE(hasAssetList);
    }

    TEST_F(EditorRenderIntegration, AssetProcessingFactoryPreservesDescriptorsAndSchemaWithoutRendererActivation)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateAssetProcessingModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{7B330394-BE9C-4BDA-9345-1A0859815982}"));
        EXPECT_TRUE(module->GetRequiredSystemComponents().empty());
        AZStd::unordered_set<AZ::Uuid> expected{
            AZ::Uuid("{7AE0D6AD-691C-41B6-9DD5-F23F78B1A02E}"),
            AZ::Uuid("{FFB99CE4-2C9E-476D-8140-50A8A696E242}"),
            AZ::Uuid("{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}") };
        const auto descriptors = module->GetComponentDescriptors();
        ASSERT_EQ(descriptors.size(), 3);
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        AzToolsFramework::Components::EditorComponentBase::Reflect(&context);
        for (auto* descriptor : descriptors)
        {
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
            descriptor->Reflect(&context);
            EXPECT_NE(context.FindClassData(descriptor->GetUuid()), nullptr);
        }
        EXPECT_TRUE(expected.empty());
        const auto* editor = context.FindClassData(AZ::Uuid("{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}"));
        ASSERT_NE(editor, nullptr);
        EXPECT_STREQ(editor->m_name, "AssetCollectionAsyncLoaderTestComponent");
        EXPECT_EQ(editor->m_version, 1);
        bool hasAssetList = false;
        for (const auto& element : editor->m_elements)
        {
            if (element.m_nameCrc == AZ_CRC_CE("AssetListPathJson"))
            {
                hasAssetList = true;
                EXPECT_EQ(element.m_typeId, azrtti_typeid<AZStd::string>());
            }
        }
        EXPECT_TRUE(hasAssetList);
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
