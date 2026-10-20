// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#include <AzTest/AzTest.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <JE/Editor/Tools/Module.h>

namespace UnitTest
{
    // Catches missing/wrong registration, service identity and failure to release descriptors.
    TEST(NativeEditorToolsModuleContract, PreservesDescriptorsServicesRequiredAndRecreation)
    {
        struct Identity { const char* name; const char* uuid; const char* service; };
        constexpr Identity expected[] = {
            { "AtomToolsFrameworkSystemComponent", "{4E0307A7-5EF4-4E00-BFF1-9B77F2401D2E}", "AtomToolsFrameworkSystemService" },
            { "AtomToolsMainWindowSystemComponent", "{6E42380B-4ECD-47CF-B904-E16AB4E87D0D}", "AtomToolsMainWindowSystemService" },
            { "PerformanceMonitorSystemComponent", "{C2F54D1B-A106-4922-82BE-ACB7A168D4AF}", "PerformanceMonitorService" },
            { "PreviewRendererSystemComponent", "{E9F79FD8-82F2-4C80-966D-95F28484F229}", "PreviewRendererSystem" },
        };
        for (int cycle = 0; cycle != 2; ++cycle)
        {
            for (const auto& identity : expected)
            {
                AZ::ComponentDescriptor* found = nullptr;
                AZ::ComponentDescriptorBus::EventResult(found, AZ::Uuid(identity.uuid), &AZ::ComponentDescriptor::GetDescriptor);
                ASSERT_EQ(found, nullptr) << identity.name;
            }
            AZStd::unique_ptr<AZ::Module> module(JE::Editor::Tools::CreateModule());
            ASSERT_NE(module, nullptr);
            EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{B58B7CA8-98C9-4DC8-8607-E094989BBBE2}"));
            EXPECT_EQ(AZStd::string_view(module->RTTI_GetTypeName()), "AtomToolsFrameworkModule");
            const auto descriptors = module->GetComponentDescriptors();
            ASSERT_EQ(descriptors.size(), 4);
            AZ::ComponentTypeList required;
            size_t index = 0;
            for (auto* descriptor : descriptors)
            {
                ASSERT_NE(descriptor, nullptr);
                EXPECT_EQ(descriptor->GetUuid(), AZ::Uuid(expected[index].uuid));
                EXPECT_EQ(AZStd::string_view(descriptor->GetName()), expected[index].name);
                AZ::ComponentDescriptor::DependencyArrayType provided;
                descriptor->GetProvidedServices(provided, nullptr);
                ASSERT_EQ(provided.size(), 1);
                EXPECT_EQ(provided.front(), AZ::Crc32(expected[index].service));
                required.push_back(AZ::Uuid(expected[index].uuid));
                ++index;
            }
            EXPECT_EQ(module->GetRequiredSystemComponents(), required);
            module.reset();
            for (const auto& identity : expected)
            {
                AZ::ComponentDescriptor* found = nullptr;
                AZ::ComponentDescriptorBus::EventResult(found, AZ::Uuid(identity.uuid), &AZ::ComponentDescriptor::GetDescriptor);
                EXPECT_EQ(found, nullptr) << identity.name;
            }
        }
    }
}
