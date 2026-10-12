/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Data/ShaderVariantListGenerator.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace JE::ShaderVariants
{
    class ShaderVariantListGeneratorTests : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            AZ::NameDictionary::Create();
            m_layout = AZ::RPI::ShaderOptionGroupLayout::Create();
            using namespace AZ::RPI;
            ASSERT_TRUE(m_layout->AddShaderOption({AZ::Name("A"), ShaderOptionType::Boolean, 0, 0,
                CreateBoolShaderOptionValues(), AZ::Name("False")}));
            ASSERT_TRUE(m_layout->AddShaderOption({AZ::Name("B"), ShaderOptionType::Boolean, 1, 1,
                CreateBoolShaderOptionValues(), AZ::Name("False")}));
            // Non-first default catches comparing an option-value handle with a value name, or retaining a material value.
            ASSERT_TRUE(m_layout->AddShaderOption({AZ::Name("System"), ShaderOptionType::Enumeration, 2, 2,
                CreateEnumShaderOptionValues({"Low", "Medium", "High"}), AZ::Name("Medium")}));
            m_layout->Finalize();
        }

        void TearDown() override
        {
            m_layout.reset();
            AZ::NameDictionary::Destroy();
        }

        MaterialVariant Material(AZStd::initializer_list<AZStd::pair<const char*, const char*>> values)
        {
            MaterialVariant result{AZ::RPI::ShaderOptionGroup(m_layout), {}};
            for (const auto& [name, value] : values)
            {
                EXPECT_TRUE(result.m_options.SetValue(AZ::Name(name), AZ::Name(value)));
                result.m_materialOwnedOptions.insert(AZ::Name(name));
            }
            return result;
        }

        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> m_layout;
    };

    TEST_F(ShaderVariantListGeneratorTests, NoVariantsReturnsEmptyListAndHints)
    {
        auto result = Generate("fixture.shader", *m_layout, {}, {});
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue().m_shaderFilePath, "fixture.shader");
        EXPECT_TRUE(result.GetValue().m_shaderVariants.empty());
        EXPECT_TRUE(result.GetValue().m_materialOptionsHint.empty());
        auto root = Material({});
        auto rootResult = Generate("fixture.shader", *m_layout, {root}, {});
        ASSERT_TRUE(rootResult.IsSuccess());
        EXPECT_TRUE(rootResult.GetValue().m_shaderVariants.empty());
    }

    TEST_F(ShaderVariantListGeneratorTests, HintsAreUnionOfAllMaterialsAndStableIdsStartAtOne)
    {
        auto result = Generate("fixture.shader", *m_layout, {Material({{"A", "True"}}), Material({{"B", "True"}})}, {});
        ASSERT_TRUE(result.IsSuccess());
        const auto& data = result.GetValue();
        ASSERT_EQ(data.m_shaderVariants.size(), 2);
        EXPECT_EQ(data.m_shaderVariants[0].m_stableId, 1);
        EXPECT_EQ(data.m_shaderVariants[1].m_stableId, 2);
        EXPECT_EQ(data.m_shaderVariants[0].m_options.at(AZ::Name("A")), AZ::Name("True"));
        EXPECT_EQ(data.m_shaderVariants[1].m_options.at(AZ::Name("B")), AZ::Name("True"));
        EXPECT_EQ(data.m_materialOptionsHint.size(), 2);
        EXPECT_EQ(data.m_materialOptionsHint.count(AZ::Name("A")), 1);
        EXPECT_EQ(data.m_materialOptionsHint.count(AZ::Name("B")), 1);
    }

    TEST_F(ShaderVariantListGeneratorTests, DeduplicatesAfterNonMaterialOptionsAreCleared)
    {
        auto first = Material({{"A", "True"}, {"B", "False"}});
        auto second = Material({{"A", "True"}, {"B", "True"}});
        first.m_materialOwnedOptions.erase(AZ::Name("B"));
        second.m_materialOwnedOptions.erase(AZ::Name("B"));
        auto result = Generate("fixture.shader", *m_layout, {first, second, first}, {});
        ASSERT_TRUE(result.IsSuccess());
        ASSERT_EQ(result.GetValue().m_shaderVariants.size(), 1);
        EXPECT_EQ(result.GetValue().m_shaderVariants[0].m_options.size(), 1);
        EXPECT_EQ(result.GetValue().m_materialOptionsHint.size(), 1);
    }

    TEST_F(ShaderVariantListGeneratorTests, FixedSystemOverrideDeduplicatesAndIsExcludedFromHints)
    {
        auto result = Generate("fixture.shader", *m_layout,
            {Material({{"A", "True"}, {"System", "Low"}}), Material({{"A", "True"}, {"System", "High"}})},
            {{AZ::Name("System"), AZ::Name("Medium")}});
        ASSERT_TRUE(result.IsSuccess());
        ASSERT_EQ(result.GetValue().m_shaderVariants.size(), 1);
        EXPECT_EQ(result.GetValue().m_shaderVariants[0].m_options.at(AZ::Name("System")), AZ::Name("Medium"));
        EXPECT_EQ(result.GetValue().m_materialOptionsHint.count(AZ::Name("System")), 0);
    }

    TEST_F(ShaderVariantListGeneratorTests, ExpansionUsesRealNonFirstDefaultAndDenseStableIds)
    {
        auto result = Generate("fixture.shader", *m_layout, {Material({{"A", "True"}, {"System", "High"}})},
            {{AZ::Name("System"), AZ::Name()}});
        ASSERT_TRUE(result.IsSuccess());
        const auto& variants = result.GetValue().m_shaderVariants;
        ASSERT_EQ(variants.size(), 3);
        EXPECT_EQ(variants[0].m_options.at(AZ::Name("System")), AZ::Name("Medium"));
        EXPECT_EQ(variants[1].m_options.at(AZ::Name("System")), AZ::Name("Low"));
        EXPECT_EQ(variants[2].m_options.at(AZ::Name("System")), AZ::Name("High"));
        EXPECT_EQ(variants[0].m_stableId, 1);
        EXPECT_EQ(variants[1].m_stableId, 2);
        EXPECT_EQ(variants[2].m_stableId, 3);
    }

    TEST_F(ShaderVariantListGeneratorTests, NoMaterialsPreservesSystemOnlyInitializationAndCartesianExpansion)
    {
        auto result = Generate("fixture.shader", *m_layout, {}, {{AZ::Name("A"), AZ::Name()}, {AZ::Name("System"), AZ::Name()}});
        ASSERT_TRUE(result.IsSuccess());
        const auto& variants = result.GetValue().m_shaderVariants;
        ASSERT_EQ(variants.size(), 6);
        EXPECT_TRUE(result.GetValue().m_materialOptionsHint.empty());
        EXPECT_EQ(variants[0].m_options.at(AZ::Name("A")), AZ::Name("False"));
        EXPECT_EQ(variants[0].m_options.at(AZ::Name("System")), AZ::Name("Medium"));
        EXPECT_EQ(variants[5].m_options.at(AZ::Name("A")), AZ::Name("True"));
        EXPECT_EQ(variants[5].m_options.at(AZ::Name("System")), AZ::Name("High"));
        for (size_t index = 0; index < variants.size(); ++index)
        {
            EXPECT_EQ(variants[index].m_stableId, index + 1);
        }
    }

    TEST_F(ShaderVariantListGeneratorTests, ExpansionDoesNotCloneVariantsMissingTheSystemOption)
    {
        auto result = Generate("fixture.shader", *m_layout,
            {Material({{"A", "True"}}), Material({{"B", "True"}, {"System", "High"}})},
            {{AZ::Name("System"), AZ::Name()}});
        ASSERT_TRUE(result.IsSuccess());
        ASSERT_EQ(result.GetValue().m_shaderVariants.size(), 4);
        EXPECT_EQ(result.GetValue().m_shaderVariants[0].m_options.count(AZ::Name("System")), 0);
        EXPECT_EQ(result.GetValue().m_materialOptionsHint.size(), 2);
    }

    TEST_F(ShaderVariantListGeneratorTests, InvalidSystemKeyOrValueReturnsFailureWithoutADataSet)
    {
        auto unknown = Generate("fixture.shader", *m_layout, {}, {{AZ::Name("Absent"), AZ::Name("True")}});
        ASSERT_FALSE(unknown.IsSuccess());
        EXPECT_EQ(unknown.GetError().m_reason, FailureReason::InvalidInput);
        auto invalid = Generate("fixture.shader", *m_layout, {}, {{AZ::Name("System"), AZ::Name("Typo")}});
        ASSERT_FALSE(invalid.IsSuccess());
        EXPECT_EQ(invalid.GetError().m_reason, FailureReason::InvalidInput);
    }

    TEST_F(ShaderVariantListGeneratorTests, InvalidLayoutPathAndOwnershipAreRejected)
    {
        auto unfinished = AZ::RPI::ShaderOptionGroupLayout::Create();
        EXPECT_FALSE(Generate("fixture.shader", *unfinished, {}, {}).IsSuccess());
        EXPECT_FALSE(Generate("", *m_layout, {}, {}).IsSuccess());
        auto material = Material({{"A", "True"}});
        material.m_materialOwnedOptions.insert(AZ::Name("Absent"));
        EXPECT_FALSE(Generate("fixture.shader", *m_layout, {material}, {}).IsSuccess());
        material.m_options = AZ::RPI::ShaderOptionGroup();
        EXPECT_FALSE(Generate("fixture.shader", *m_layout, {material}, {}).IsSuccess());
    }

    TEST_F(ShaderVariantListGeneratorTests, CancelBeforeAndDuringGenerationReturnsNoPartialData)
    {
        auto before = Generate("fixture.shader", *m_layout, {}, {}, [] { return true; });
        ASSERT_FALSE(before.IsSuccess());
        EXPECT_EQ(before.GetError().m_reason, FailureReason::Cancelled);
        size_t polls = 0;
        auto during = Generate("fixture.shader", *m_layout, {}, {{AZ::Name("A"), AZ::Name()}, {AZ::Name("System"), AZ::Name()}},
            [&polls] { return ++polls >= 4; });
        ASSERT_FALSE(during.IsSuccess());
        EXPECT_EQ(during.GetError().m_reason, FailureReason::Cancelled);
    }

    TEST_F(ShaderVariantListGeneratorTests, MalformedMaterialValueAndPartialOrForeignMasksAreRejected)
    {
        auto material = Material({{"System", "High"}});
        auto id = material.m_options.GetShaderVariantId();
        id.m_key.set(2).set(3); // value 3 is not part of the three-value System enum.
        material.m_options = AZ::RPI::ShaderOptionGroup(m_layout, id);
        EXPECT_FALSE(Generate("fixture.shader", *m_layout, {material}, {}).IsSuccess());
        id.m_key.reset();
        id.m_mask.reset(3); // only one bit of the two-bit descriptor is present.
        material.m_options = AZ::RPI::ShaderOptionGroup(m_layout, id);
        EXPECT_FALSE(Generate("fixture.shader", *m_layout, {material}, {}).IsSuccess());
        id.m_mask.set(10);
        material.m_options = AZ::RPI::ShaderOptionGroup(m_layout, id);
        EXPECT_FALSE(Generate("fixture.shader", *m_layout, {material}, {}).IsSuccess());
    }

    TEST_F(ShaderVariantListGeneratorTests, ProductSubIdOverflowIsRejectedBeforeExpansion)
    {
        auto layout = AZ::RPI::ShaderOptionGroupLayout::Create();
        ASSERT_TRUE(layout->AddShaderOption({AZ::Name("Range"), AZ::RPI::ShaderOptionType::IntegerRange, 0, 0,
            AZ::RPI::CreateIntRangeShaderOptionValues(0, 131071), AZ::Name("0")}));
        layout->Finalize();
        auto result = Generate("fixture.shader", *layout, {}, {{AZ::Name("Range"), AZ::Name()}});
        ASSERT_FALSE(result.IsSuccess());
        EXPECT_EQ(result.GetError().m_reason, FailureReason::InvalidInput);
    }

    TEST_F(ShaderVariantListGeneratorTests, CartesianProductOverflowIsRejectedWithIndividuallyValidRanges)
    {
        auto layout = AZ::RPI::ShaderOptionGroupLayout::Create();
        ASSERT_TRUE(layout->AddShaderOption({AZ::Name("X"), AZ::RPI::ShaderOptionType::IntegerRange, 0, 0,
            AZ::RPI::CreateIntRangeShaderOptionValues(0, 511), AZ::Name("0")}));
        ASSERT_TRUE(layout->AddShaderOption({AZ::Name("Y"), AZ::RPI::ShaderOptionType::IntegerRange, 9, 1,
            AZ::RPI::CreateIntRangeShaderOptionValues(0, 511), AZ::Name("0")}));
        layout->Finalize();
        auto result = Generate("fixture.shader", *layout, {}, {{AZ::Name("X"), AZ::Name()}, {AZ::Name("Y"), AZ::Name()}});
        ASSERT_FALSE(result.IsSuccess());
        EXPECT_EQ(result.GetError().m_reason, FailureReason::InvalidInput);
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
