/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Assets/Prefab/PrefabBuilderModule.h>
#include <JE/Assets/Prefab/PrefabBuilderComponent.h>
#include <JE/Assets/Prefab/PrefabGroup/PrefabGroupBehavior.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>

namespace JE::Assets::Prefab::Tests
{
    TEST(NativePrefabBuilder, FactoryPreservesDescriptorIdentitiesAndTagDrivenActivation)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateToolsModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{088B2BA8-9F19-469C-A0B5-1DD523879C70}"));
        EXPECT_TRUE(module->GetRequiredSystemComponents().empty());
        AZStd::unordered_set<AZ::Uuid> expected{
            AZ::Uuid("{A2E0791C-4607-4363-A7FD-73D01ED49660}"),
            AZ::Uuid("{13DC2819-CAC2-4977-91D7-C870087072AB}") };
        const auto descriptors = module->GetComponentDescriptors();
        ASSERT_EQ(descriptors.size(), 2);
        for (const auto* descriptor : descriptors)
        {
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
        }
        EXPECT_TRUE(expected.empty());
    }
}
