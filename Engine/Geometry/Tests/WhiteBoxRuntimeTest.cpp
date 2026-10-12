/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <JE/Geometry/GeometryModule.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>

TEST(GeometryNativeModule, RuntimeFactoryRetainsOriginalDescriptorsAndRequiredService)
{
    AZStd::unique_ptr<AZ::Module> module(JE::Geometry::CreateRuntimeModule());
    ASSERT_NE(module, nullptr);
    AZStd::unordered_set<AZ::Uuid> expected{
        AZ::Uuid("{BD393FD9-CF47-433D-B171-C44FE2F7069F}"),
        AZ::Uuid("{B60C4D82-3299-414A-B91B-0299AA51BEF6}"),
        AZ::Uuid("{6CFD4D82-FA68-4C18-BE67-43FC2B755B64}")};
    const auto descriptors = module->GetComponentDescriptors();
    ASSERT_EQ(descriptors.size(), 3);
    for (auto* descriptor : descriptors)
    {
        ASSERT_NE(descriptor, nullptr);
        EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
    }
    EXPECT_TRUE(expected.empty());
    const auto required = module->GetRequiredSystemComponents();
    ASSERT_EQ(required.size(), 1);
    EXPECT_EQ(required.front(), AZ::Uuid("{BD393FD9-CF47-433D-B171-C44FE2F7069F}"));
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);