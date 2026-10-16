/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BuilderTestFixture.h>
#include <BuilderComponent.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace UnitTest
{
    TEST_F(BuilderTestFixture, NativeRenderBuildersFactoryPreservesDescriptorsTagAndModuleLifetime)
    {
        AZStd::unique_ptr<AZ::Module> module(JE::Assets::RenderBuilders::CreateToolsModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{CA15BD7F-01B4-4959-BEF2-81FA3AD2C834}"));
        EXPECT_STREQ(module->RTTI_GetTypeName(), "BuilderModule");
        EXPECT_TRUE(module->GetRequiredSystemComponents().empty());

        AZStd::unordered_set<AZ::Uuid> expected{
            AZ::Uuid("{AE42AB62-A4D6-4147-88A0-692549EE5427}"),
            AZ::Uuid("{FE21DEEB-F9E6-487E-B9F0-88478FC2F52F}"),
            AZ::Uuid("{CE1E8C2B-A05F-4AED-8771-88E58377A82B}"),
            AZ::Uuid("{777BB521-FCFE-4382-B8FD-E1EAF846F5C9}"),
            AZ::Uuid("{02163945-D7B4-45D4-9729-4376E1195505}"),
            AZ::Uuid("{7B959232-A812-4F3F-817D-25FEEA844E7C}")};
        const auto& descriptors = module->GetComponentDescriptors();
        ASSERT_EQ(descriptors.size(), 6);
        for (const AZ::ComponentDescriptor* descriptor : descriptors)
        {
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
        }
        EXPECT_TRUE(expected.empty());

        const auto* classData = m_context->FindClassData(AZ::Uuid("{7B959232-A812-4F3F-817D-25FEEA844E7C}"));
        ASSERT_NE(classData, nullptr);
        EXPECT_STREQ(classData->m_name, "BuilderComponent");
        auto* tagsAttribute = classData->FindAttribute(AZ::Edit::Attributes::SystemComponentTags);
        ASSERT_NE(tagsAttribute, nullptr);
        AZStd::vector<AZ::Crc32> tags;
        ASSERT_TRUE(AZ::AttributeReader(nullptr, tagsAttribute).Read<AZStd::vector<AZ::Crc32>>(tags));
        EXPECT_NE(AZStd::find(tags.begin(), tags.end(), AssetBuilderSDK::ComponentTags::AssetBuilder), tags.end());

        // AZ::Module owns the descriptors. Destroying and recreating through the public factory
        // exercises their real release/re-registration lifecycle under the fixture's leak check.
        module.reset();
        module.reset(JE::Assets::RenderBuilders::CreateToolsModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->GetComponentDescriptors().size(), 6);
        EXPECT_TRUE(module->GetRequiredSystemComponents().empty());
        for (AZ::ComponentDescriptor* descriptor : module->GetComponentDescriptors())
        {
            ASSERT_NE(descriptor, nullptr);
            AZStd::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
            ASSERT_NE(component, nullptr);
            EXPECT_EQ(component->RTTI_GetType(), descriptor->GetUuid());
        }
    }
    TEST_F(BuilderTestFixture, NativeShaderCompilerToolsFactoryPreservesDescriptorServicesAndModuleLifetime)
    {
        const AZ::Uuid shaderComponentId("{56B5B944-8AF4-4478-A047-8DFDE38DA681}");
        const AZ::ComponentTypeList requiredComponents{ shaderComponentId };
        AZStd::unique_ptr<AZ::Module> module(JE::Assets::RenderBuilders::CreateShaderCompilerToolsModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{43370465-DBF1-44BB-968D-97C0B42F5EA0}"));
        EXPECT_STREQ(module->RTTI_GetTypeName(), "AzslShaderBuilderModule");
        EXPECT_EQ(module->GetRequiredSystemComponents(), requiredComponents);
        ASSERT_EQ(module->GetComponentDescriptors().size(), 1);
        AZ::ComponentDescriptor* descriptor = module->GetComponentDescriptors().front();
        ASSERT_NE(descriptor, nullptr);
        EXPECT_EQ(descriptor->GetUuid(), shaderComponentId);
        AZ::ComponentDescriptor::DependencyArrayType provided, incompatible, dependent, required;
        descriptor->GetProvidedServices(provided, nullptr);
        descriptor->GetIncompatibleServices(incompatible, nullptr);
        descriptor->GetDependentServices(dependent, nullptr);
        descriptor->GetRequiredServices(required, nullptr);
        EXPECT_EQ(provided, AZ::ComponentDescriptor::DependencyArrayType{ AZ_CRC_CE("AzslShaderBuilderService") });
        EXPECT_EQ(incompatible, AZ::ComponentDescriptor::DependencyArrayType{ AZ_CRC_CE("AzslShaderBuilderService") });
        EXPECT_EQ(dependent, AZ::ComponentDescriptor::DependencyArrayType{ AZ_CRC_CE("AssetCatalogService") });
        EXPECT_TRUE(required.empty());

        {
            AZStd::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
            ASSERT_NE(component, nullptr);
            EXPECT_EQ(component->RTTI_GetType(), shaderComponentId);
        }
        module.reset();
        module.reset(JE::Assets::RenderBuilders::CreateShaderCompilerToolsModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{43370465-DBF1-44BB-968D-97C0B42F5EA0}"));
        EXPECT_STREQ(module->RTTI_GetTypeName(), "AzslShaderBuilderModule");
        EXPECT_EQ(module->GetRequiredSystemComponents(), requiredComponents);
        ASSERT_EQ(module->GetComponentDescriptors().size(), 1);
        descriptor = module->GetComponentDescriptors().front();
        ASSERT_NE(descriptor, nullptr);
        EXPECT_EQ(descriptor->GetUuid(), shaderComponentId);
        AZStd::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
        ASSERT_NE(component, nullptr);
        EXPECT_EQ(component->RTTI_GetType(), shaderComponentId);
    }

} // namespace UnitTest
