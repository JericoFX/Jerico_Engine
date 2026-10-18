/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <Common/RPITestFixture.h>
#include <Common/ErrorMessageFinder.h>
#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace JE::Render::RPI;

    class AssetUtilsTests
        : public RPITestFixture
    {
    protected:
        void SetUp() override
        {
            RPITestFixture::SetUp();
        }

        void TearDown() override
        {
            RPITestFixture::TearDown();
        }

    };

    TEST_F(AssetUtilsTests, TestSanitizeFilename)
    {
        static const char AllSupportedCharacters[] = "abcdefghijklmnopqrstufwxyz-ABCDEFGHIJKLMNOPQRSTUFWXYZ_0123456789.txt";

        EXPECT_STREQ("", AssetUtils::SanitizeFileName("").c_str());
        EXPECT_STREQ(AllSupportedCharacters, AssetUtils::SanitizeFileName(AllSupportedCharacters).c_str());
        EXPECT_STREQ("a_b_c_d_e_f_g_h_i_jklmnopqrstufwxyz", AssetUtils::SanitizeFileName(R"(a\b/c:d*e?f\"g<h>i|jklmnopqrstufwxyz)").c_str());
        EXPECT_STREQ("Hello_World", AssetUtils::SanitizeFileName(R"(Hello<::\*/::>World)").c_str());
        EXPECT_STREQ("CantEndWithDot", AssetUtils::SanitizeFileName("CantEndWithDot.").c_str());
        EXPECT_STREQ("Cant_Have_Multiple_Dots_", AssetUtils::SanitizeFileName("Cant..Have...Multiple....Dots...").c_str());
        EXPECT_STREQ("Prevent_MultipleUnderscores", AssetUtils::SanitizeFileName(R"(Prevent<::_\*/_::>MultipleUnderscores)").c_str());
        // These characters might be allowed by the OS, but they are prevented in FileDialog::GetSaveFileName
        EXPECT_STREQ("Other_Characters", AssetUtils::SanitizeFileName("Other~!@#$%^&*()+=[]{},;'`Characters").c_str());
    }

    class AssetReferenceCompatibilityTests
        : public AssetUtilsTests
    {
    protected:
        void SetUp() override
        {
            AssetUtilsTests::SetUp();
            m_fileIO = IO::FileIOBase::GetInstance();
            ASSERT_NE(m_fileIO, nullptr);
            const char* engineAlias = m_fileIO->GetAlias("@engroot@");
            m_hadEngineAlias = engineAlias != nullptr;
            if (m_hadEngineAlias)
            {
                m_previousEngineAlias = engineAlias;
            }
            ASSERT_EQ(m_fileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
            m_fileIO->SetAlias("@engroot@", "C:/AssetUtilsReferenceEngine");
        }

        void TearDown() override
        {
            if (m_fileIO)
            {
                EXPECT_EQ(m_fileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
                if (m_hadEngineAlias)
                {
                    m_fileIO->SetAlias("@engroot@", m_previousEngineAlias.c_str());
                }
                else
                {
                    m_fileIO->ClearAlias("@engroot@");
                }
            }
            AssetUtilsTests::TearDown();
        }

        void ExpectResolvedReference(const char* reference, const char* expected, const char* origin = "")
        {
            ASSERT_EQ(m_fileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
            const AZStd::string actual = AssetUtils::ResolvePathReference(origin, reference);
            EXPECT_STREQ(expected, actual.c_str()) << reference;
            EXPECT_EQ(m_fileIO->GetAlias("@gemroot:Atom_Feature_Common@"), nullptr);
        }

        void ExpectResolvedAbsoluteReference(const char* reference, const char* expected, const char* origin = "")
        {
            const IO::Path expectedPath = IO::Path(expected).LexicallyNormal();
            ASSERT_TRUE(expectedPath.IsAbsolute());
            ExpectResolvedReference(reference, expectedPath.c_str(), origin);
        }

        IO::FileIOBase* m_fileIO = nullptr;
        AZStd::string m_previousEngineAlias;
        bool m_hadEngineAlias = false;
    };

    TEST_F(AssetReferenceCompatibilityTests, LegacySlashReferenceResolvesToNativeFeatures)
    {
        ExpectResolvedAbsoluteReference("@gemroot:Atom_Feature_Common@/Assets/Materials/test.material",
            "C:/AssetUtilsReferenceEngine/Engine/Render/Features/Assets/Materials/test.material");
    }

    TEST_F(AssetReferenceCompatibilityTests, LegacyBackslashReferenceResolvesToNativeFeatures)
    {
        ExpectResolvedAbsoluteReference(R"(@gemroot:Atom_Feature_Common@\Assets\Materials\test.material)",
            "C:/AssetUtilsReferenceEngine/Engine/Render/Features/Assets/Materials/test.material");
    }

    TEST_F(AssetReferenceCompatibilityTests, LegacyBareAliasResolvesToNativeFeatures)
    {
        ExpectResolvedAbsoluteReference("@gemroot:Atom_Feature_Common@",
            "C:/AssetUtilsReferenceEngine/Engine/Render/Features");
    }

    TEST_F(AssetReferenceCompatibilityTests, NativeEngineAliasReferenceRemainsSupported)
    {
        ExpectResolvedAbsoluteReference("@engroot@/Engine/Render/Features/Assets/Materials/test.material",
            "C:/AssetUtilsReferenceEngine/Engine/Render/Features/Assets/Materials/test.material");
    }

    TEST_F(AssetReferenceCompatibilityTests, RelativeAndEmptyReferencesRemainSupported)
    {
        Data::AssetInfo parentInfo;
        parentInfo.m_assetId = Data::AssetId(Uuid::CreateString("{CD9192D1-FF43-4B3C-B47C-38120158C1B7}"), 0);
        parentInfo.m_relativePath = "Materials/parent.material";
        m_assetSystemStub.RegisterSourceInfo("C:/AssetUtilsReferenceOrigin/Materials/parent.material", parentInfo,
            "C:/AssetUtilsReferenceOrigin");
        Data::AssetInfo childInfo;
        childInfo.m_assetId = Data::AssetId(Uuid::CreateString("{E15853F2-C1A6-44F3-AD42-192C156592A8}"), 0);
        childInfo.m_relativePath = "Materials/child.materialtype";
        m_assetSystemStub.RegisterSourceInfo("C:/AssetUtilsReferenceOrigin/Materials/child.materialtype", childInfo,
            "C:/AssetUtilsReferenceOrigin");

        ExpectResolvedAbsoluteReference("child.materialtype", "C:/AssetUtilsReferenceOrigin/Materials/child.materialtype",
            "C:/AssetUtilsReferenceOrigin/Materials/parent.material");
        ExpectResolvedReference("", "");
    }

    TEST_F(AssetReferenceCompatibilityTests, SimilarLegacyPrefixesAreNotTranslated)
    {
        ExpectResolvedReference("@gemroot:Atom_Feature_CommonExtra@/Assets/Materials/test.material",
            IO::Path("@gemroot:Atom_Feature_CommonExtra@/Assets/Materials/test.material").LexicallyNormal().c_str());
        ExpectResolvedReference("@gemroot:Atom_Feature_Common@evil/Assets/Materials/test.material",
            IO::Path("@gemroot:Atom_Feature_Common@evil/Assets/Materials/test.material").LexicallyNormal().c_str());
    }

}
