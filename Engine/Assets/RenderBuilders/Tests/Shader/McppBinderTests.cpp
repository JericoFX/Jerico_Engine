/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>

#include "Common/ShaderBuilderTestFixture.h"

#include <CommonFiles/Preprocessor.h>
#include <ShaderAssetBuilder.h>
#include <ShaderBuilderUtility.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <fstream>
#include <iterator>

namespace UnitTest
{
    using namespace AZ;

    // The main purpose of this class is to test ShaderBuilder::McppBinder::Fprintf_StaticHinge()
    // Which has three common scenarios to validate.
    // 1- The formatted string is expected to yield less bytes than McppBinder::DefaultFprintfBufferSize.
    // 2- The formatted string is expected to yield exactly McppBinder::DefaultFprintfBufferSize number of bytes.
    // 3- The formatted string is expectedc to yield more bytes than McppBinder::DefaultFprintfBufferSize.
    class McppBinderTests : public ShaderBuilderTestFixture
    {
    public:

        // Fills @buffer with 'a' to 'z' for up to @bufferSize number of bytes.
        // This function will null('\0') char terminate @buffer.
        void FillBufferWithAlphabet(char* buffer, int bufferSize)
        {
            for (int bufferPos = 0, rollback = 0; bufferPos < (bufferSize - 1); ++bufferPos)
            {
                const char value = 'a' + static_cast<char>(rollback++);
                buffer[bufferPos] = value;
                if (value == 'z')
                {
                    rollback = 0;
                }
            }
            buffer[bufferSize - 1] = '\0';
        }

        // Pushes the null terminated string, @inputString, into McppBinder capture stream
        // using McppBinder::Fprintf_StaticHinge().
        // Returns the content of the McppBinder capture stream as a string.
        AZStd::string PrintStringThroughStaticHinge(const char* inputString)
        {
            JE::Assets::RenderBuilders::Shader::PreprocessorData preprocessorData;
            JE::Assets::RenderBuilders::Shader::McppBinder mcppBinder(preprocessorData, false);
            JE::Assets::RenderBuilders::Shader::McppBinder::Fprintf_StaticHinge(MCPP_OUTDEST::MCPP_OUT, "%s", inputString);
            // convert from std::ostringstring to AZStd::string
            return AZStd::string(mcppBinder.m_outStream.str().c_str());
        }
    }; // class McppBinderTests


    TEST_F(McppBinderTests, ShouldPrintLessBytesThanDefaultSize)
    {
        constexpr int bufferSize = (JE::Assets::RenderBuilders::Shader::McppBinder::DefaultFprintfBufferSize / 2) + 1;
        EXPECT_TRUE(bufferSize > 0);
        char buffer[bufferSize] = "";
        FillBufferWithAlphabet(buffer, bufferSize);
        auto printedString = PrintStringThroughStaticHinge(buffer);
        EXPECT_EQ(AZStd::string(buffer), printedString);
    }

    TEST_F(McppBinderTests, ShouldPrintSameBytesAsDefaultSize)
    {
        constexpr int bufferSize = JE::Assets::RenderBuilders::Shader::McppBinder::DefaultFprintfBufferSize + 1;
        EXPECT_TRUE(bufferSize > 0);
        char buffer[bufferSize] = "";
        FillBufferWithAlphabet(buffer, bufferSize);
        auto printedString = PrintStringThroughStaticHinge(buffer);
        EXPECT_EQ(AZStd::string(buffer), printedString);
    }

    TEST_F(McppBinderTests, ShouldPrintMoreBytesThanDefaultSize)
    {
        constexpr int bufferSize = (JE::Assets::RenderBuilders::Shader::McppBinder::DefaultFprintfBufferSize * 2) + 1;
        EXPECT_TRUE(bufferSize > 0);
        char buffer[bufferSize] = "";
        FillBufferWithAlphabet(buffer, bufferSize);
        auto printedString = PrintStringThroughStaticHinge(buffer);
        EXPECT_EQ(AZStd::string(buffer), printedString);
    }


    TEST_F(McppBinderTests, GeneratedShaderResolvesNativeEngineAssetWithoutGemFallback)
    {
        AZ::Test::ScopedAutoTempDirectory engineFolder;
        AZ::Test::ScopedAutoTempDirectory projectFolder;
        AZ::Test::ScopedAutoTempDirectory generatedFolder;
        const auto nativeHeader = AZ::Test::CreateTestFile(
            engineFolder, "Engine/Render/Features/Assets/NativeRootFixture.azsli",
            "#define JE_NATIVE_INCLUDE_MARKER 73\n");
        const auto generatedShader = AZ::Test::CreateTestFile(
            generatedFolder, "MaterialGraph.azsl",
            "#include <Engine/Render/Features/Assets/NativeRootFixture.azsli>\n"
            "int nativeIncludeValue = JE_NATIVE_INCLUDE_MARKER;\n");
        ASSERT_TRUE(nativeHeader.has_value());
        ASSERT_TRUE(generatedShader.has_value());
        ASSERT_NE(engineFolder.GetDirectoryAsPath(), projectFolder.GetDirectoryAsPath());
        ASSERT_NE(engineFolder.GetDirectoryAsPath(), generatedFolder.GetDirectoryAsPath());
        ASSERT_FALSE(AZ::IO::SystemFile::Exists(engineFolder.Resolve("Gems").c_str()));
        ASSERT_FALSE(AZ::IO::SystemFile::Exists(projectFolder.Resolve("Engine").c_str()));
        ASSERT_FALSE(AZ::IO::SystemFile::Exists(generatedFolder.Resolve("Engine").c_str()));

        AZ::SettingsRegistryImpl localRegistry;
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        const bool registeredLocally = registry == nullptr;
        if (registeredLocally)
        {
            registry = &localRegistry;
            AZ::SettingsRegistry::Register(registry);
        }
        struct RestoreRegistryPaths
        {
            AZ::SettingsRegistryInterface* m_registry;
            bool m_registeredLocally;
            AZStd::string m_enginePath;
            AZStd::string m_projectPath;
            bool m_hadEnginePath;
            bool m_hadProjectPath;

            ~RestoreRegistryPaths()
            {
                if (m_hadEnginePath)
                {
                    m_registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, m_enginePath);
                }
                else
                {
                    m_registry->Remove(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
                }
                if (m_hadProjectPath)
                {
                    m_registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, m_projectPath);
                }
                else
                {
                    m_registry->Remove(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
                }
                if (m_registeredLocally)
                {
                    AZ::SettingsRegistry::Unregister(m_registry);
                }
            }
        } restorePaths{ registry, registeredLocally, {}, {}, false, false };
        restorePaths.m_hadEnginePath = registry->Get(
            restorePaths.m_enginePath, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        restorePaths.m_hadProjectPath = registry->Get(
            restorePaths.m_projectPath, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
        ASSERT_TRUE(registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, engineFolder.GetDirectory()));
        ASSERT_TRUE(registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, projectFolder.GetDirectory()));

        const auto includeDirectories = JE::Assets::RenderBuilders::Shader::BuildListOfIncludeDirectories(
            "McppBinderTests", generatedFolder.GetDirectory());
        const auto arguments = JE::Assets::RenderBuilders::Shader::AppendIncludePathsToArgumentList({}, includeDirectories);
        JE::Assets::RenderBuilders::Shader::PreprocessorData output;
        ASSERT_TRUE(JE::Assets::RenderBuilders::Shader::PreprocessFile(generatedShader->c_str(), output, arguments, true))
            << output.diagnostics.c_str();
        EXPECT_NE(output.code.find("int nativeIncludeValue =  73 ;"), AZStd::string::npos)
            << "\noutput.code (" << output.code.size() << " bytes):\n" << output.code.c_str();
        bool includedNativeHeader = false;
        for (const auto& includedPath : output.includedPaths)
        {
            includedNativeHeader = includedNativeHeader || AZ::IO::Path(includedPath) == AZ::IO::Path(*nativeHeader);
        }
        EXPECT_TRUE(includedNativeHeader);
    }

    TEST_F(McppBinderTests, LegacyNestedIncludesResolveNativeProvidersAndDependenciesWithoutGems)
    {
        AZ::Test::ScopedAutoTempDirectory engineFolder;
        AZ::Test::ScopedAutoTempDirectory projectFolder;
        AZ::Test::ScopedAutoTempDirectory generatedFolder;
        const auto coreHeader = AZ::Test::CreateTestFile(engineFolder,
            "Engine/Render/Features/Assets/CompatibilityFixture.azsli", "#define JE_COMPAT_MARKER 87\n");
        const auto rpiHeader = AZ::Test::CreateTestFile(engineFolder,
            "Engine/Render/Integration/RPI/Assets/ShaderLib/Atom/RPI/CompatibilityFixture.azsli", "#define JE_COMPAT_MARKER 87\n");
        const auto relativeHeader = AZ::Test::CreateTestFile(projectFolder,
            "RelativeFixture.azsli", "#define JE_COMPAT_MARKER 87\n");
        // Decoys make a missing boundary check observable as an incorrect success.
        const auto falseCoreProvider = AZ::Test::CreateTestFile(engineFolder,
            "Engine/Render/Features/AssetsExtra/CompatibilityFixture.azsli", "#define JE_COMPAT_MARKER 87\n");
        const auto falseRpiProvider = AZ::Test::CreateTestFile(engineFolder,
            "Engine/Render/Integration/RPI/Assets/ShaderLibExtra/Atom/RPI/CompatibilityFixture.azsli", "#define JE_COMPAT_MARKER 87\n");
        ASSERT_TRUE(coreHeader.has_value());
        ASSERT_TRUE(rpiHeader.has_value());
        ASSERT_TRUE(relativeHeader.has_value());
        ASSERT_TRUE(falseCoreProvider.has_value());
        ASSERT_TRUE(falseRpiProvider.has_value());
        ASSERT_FALSE(AZ::IO::SystemFile::Exists(engineFolder.Resolve("Gems").c_str()));
        AZ::SettingsRegistryImpl localRegistry;
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        const bool registeredLocally = registry == nullptr;
        if (registeredLocally)
        {
            registry = &localRegistry;
            AZ::SettingsRegistry::Register(registry);
        }
        struct RestoreRegistryPaths
        {
            AZ::SettingsRegistryInterface* m_registry;
            bool m_registeredLocally;
            AZStd::string m_enginePath;
            AZStd::string m_projectPath;
            bool m_hadEnginePath;
            bool m_hadProjectPath;

            ~RestoreRegistryPaths()
            {
                if (m_hadEnginePath)
                {
                    m_registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, m_enginePath);
                }
                else
                {
                    m_registry->Remove(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
                }
                if (m_hadProjectPath)
                {
                    m_registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, m_projectPath);
                }
                else
                {
                    m_registry->Remove(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
                }
                if (m_registeredLocally)
                {
                    AZ::SettingsRegistry::Unregister(m_registry);
                }
            }
        } restorePaths{ registry, registeredLocally, {}, {}, false, false };
        restorePaths.m_hadEnginePath = registry->Get(
            restorePaths.m_enginePath, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        restorePaths.m_hadProjectPath = registry->Get(
            restorePaths.m_projectPath, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
        ASSERT_TRUE(registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, engineFolder.GetDirectory()));
        ASSERT_TRUE(registry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, projectFolder.GetDirectory()));
        AZ::IO::LocalFileIO localFileIO;
        const bool installLocalFileIO = AZ::IO::FileIOBase::GetInstance() == nullptr;
        if (installLocalFileIO)
        {
            AZ::IO::FileIOBase::SetInstance(&localFileIO);
        }
        struct RestoreFileIO
        {
            bool m_installed;
            ~RestoreFileIO()
            {
                if (m_installed)
                {
                    AZ::IO::FileIOBase::SetInstance(nullptr);
                }
            }
        } restoreFileIO{ installLocalFileIO };

        const auto includeDirectories = JE::Assets::RenderBuilders::Shader::BuildListOfIncludeDirectories(
            "McppBinderTests", generatedFolder.GetDirectory());
        const auto arguments = JE::Assets::RenderBuilders::Shader::AppendIncludePathsToArgumentList({}, includeDirectories);
        struct IncludeCase
        {
            const char* m_name;
            const char* m_text;
            const char* m_provider;
            bool m_success;
            bool m_dfsLiteral;
            bool m_legacyNonFile = false;
        };
        const IncludeCase cases[] = {
            { "legacy-core", "#include <Atom/Feature/Common/Assets/CompatibilityFixture.azsli>\n", coreHeader->c_str(), true, true },
            { "legacy-rpi", "#include <Atom/RPI/Assets/ShaderLib/Atom/RPI/CompatibilityFixture.azsli>\n", rpiHeader->c_str(), true, true },
            { "legacy-backslash", "#include <Atom\\Feature\\Common\\Assets\\CompatibilityFixture.azsli>\n", coreHeader->c_str(), true, true },
            { "canonical-core", "#include <Engine/Render/Features/Assets/CompatibilityFixture.azsli>\n", coreHeader->c_str(), true, true },
            { "canonical-rpi", "#include <Engine/Render/Integration/RPI/Assets/ShaderLib/Atom/RPI/CompatibilityFixture.azsli>\n", rpiHeader->c_str(), true, true },
            { "relative", "#include \"RelativeFixture.azsli\"\n", relativeHeader->c_str(), true, true },
            { "macro", "#define JE_HEADER <Atom/Feature/Common/Assets/CompatibilityFixture.azsli>\n#include JE_HEADER\n", coreHeader->c_str(), true, false },
            { "false-core", "#include <Atom/Feature/Common/AssetsExtra/CompatibilityFixture.azsli>\n", nullptr, false, true },
            { "false-rpi", "#include <Atom/RPI/Assets/ShaderLibExtra/Atom/RPI/CompatibilityFixture.azsli>\n", nullptr, false, true },
            { "false-leading", "#include <Other/Atom/Feature/Common/Assets/CompatibilityFixture.azsli>\n", nullptr, false, true },
            { "false-case-core", "#include <atom/Feature/Common/Assets/CompatibilityFixture.azsli>\n", nullptr, false, true },
            { "false-case-rpi", "#include <atom/RPI/Assets/ShaderLib/Atom/RPI/CompatibilityFixture.azsli>\n", nullptr, false, true },
            { "bare-core", "#include <Atom/Feature/Common/Assets>\n", nullptr, false, true, true },
            { "bare-rpi", "#include <Atom/RPI/Assets/ShaderLib>\n", nullptr, false, true, true },
            { "separator-core", "#include <Atom/Feature/Common/Assets/>\n", nullptr, false, true, true },
            { "separator-rpi", "#include <Atom/RPI/Assets/ShaderLib\\>\n", nullptr, false, true, true },
            { "separators-core", "#include <Atom/Feature/Common/Assets///>\n", nullptr, false, true, true },
            { "missing", "#include <Atom/Feature/Common/Assets/MissingFixture.azsli>\n", nullptr, false, true }
        };
        for (const auto& testCase : cases)
        {
            SCOPED_TRACE(testCase.m_name);
            const auto authored = AZ::Test::CreateTestFile(projectFolder, "AuthoredFixture.azsli", testCase.m_text);
            ASSERT_TRUE(authored.has_value());
            const auto generatedText = AZStd::string::format(
                "#define MATERIAL_TYPE_AZSLI_FILE_PATH \"%s\"\n#include MATERIAL_TYPE_AZSLI_FILE_PATH\nint compatValue = JE_COMPAT_MARKER;\n",
                "AuthoredFixture.azsli");
            const auto generated = AZ::Test::CreateTestFile(generatedFolder, "GeneratedFixture.azsl", generatedText.c_str());
            ASSERT_TRUE(generated.has_value());
            JE::Assets::RenderBuilders::Shader::PreprocessorData output;
            EXPECT_EQ(JE::Assets::RenderBuilders::Shader::PreprocessFile(generated->c_str(), output, arguments, true), testCase.m_success)
                << output.diagnostics.c_str();
            if (testCase.m_success)
            {
                EXPECT_NE(output.code.find("int compatValue =  87 ;"), AZStd::string::npos) << output.code.c_str();
                bool reportedProvider = false;
                for (const auto& path : output.includedPaths)
                {
                    reportedProvider |= AZ::IO::Path(path) == AZ::IO::Path(testCase.m_provider);
                    EXPECT_EQ(path.find("Atom/Feature/Common/Assets/"), AZStd::string::npos);
                    EXPECT_EQ(path.find("Atom/RPI/Assets/ShaderLib/"), AZStd::string::npos);
                }
                EXPECT_TRUE(reportedProvider);
            }
            else
            {
                EXPECT_FALSE(output.diagnostics.empty());
            }
            std::ifstream authoredStream(authored->c_str(), std::ios::binary);
            const std::string authoredAfter((std::istreambuf_iterator<char>(authoredStream)), std::istreambuf_iterator<char>());
            EXPECT_EQ(authoredAfter, testCase.m_text);

            AZStd::unordered_set<AZStd::string> dependencies;
            const JE::Assets::RenderBuilders::Shader::ShaderBuilderUtility::IncludedFilesParser parser;
            JE::Assets::RenderBuilders::Shader::GetListOfIncludedFiles(generated->c_str(), includeDirectories, parser, dependencies);
            if (testCase.m_success && testCase.m_dfsLiteral)
            {
                bool declaredProvider = false;
                for (const auto& path : dependencies)
                {
                    declaredProvider |= AZ::IO::Path(path) == AZ::IO::Path(testCase.m_provider);
                }
                EXPECT_TRUE(declaredProvider);
            }
            if (testCase.m_legacyNonFile)
            {
                const auto parsedIncludes = parser.ParseStringAndGetIncludedFiles(testCase.m_text);
                ASSERT_EQ(parsedIncludes.size(), 1);
                const auto legacyFutureLocation = engineFolder.Resolve(parsedIncludes.front().c_str());
                bool preservedLegacyOperand = false;
                for (const auto& path : dependencies)
                {
                    preservedLegacyOperand |= AZ::IO::Path(path) == legacyFutureLocation;
                }
                EXPECT_TRUE(preservedLegacyOperand);
            }
            if (AZStd::string_view(testCase.m_name) == "missing")
            {
                bool nativeFutureDependency = false;
                for (const auto& path : dependencies)
                {
                    nativeFutureDependency |= AZ::IO::Path(path) == engineFolder.Resolve("Engine/Render/Features/Assets/MissingFixture.azsli");
                }
                EXPECT_TRUE(nativeFutureDependency);
            }
        }
        // Arbitrary macro dependency extraction remains the existing literal-parser limitation.
        // The MATERIAL_TYPE_AZSLI_FILE_PATH outer macro is covered by the real DFS for every case above.
        ASSERT_FALSE(AZ::IO::SystemFile::Exists(engineFolder.Resolve("Gems").c_str()));
    }

    TEST_F(McppBinderTests, LegacyIncludeResolverIsResetWhenBinderReleasesMutex)
    {
        AZ::Test::ScopedAutoTempDirectory root;
        const auto provider = AZ::Test::CreateTestFile(root,
            "Engine/Render/Features/Assets/ResetFixture.azsli", "int resetMarker = 91;\n");
        const auto shader = AZ::Test::CreateTestFile(root, "ResetFixture.azsl",
            "#include <Atom/Feature/Common/Assets/ResetFixture.azsli>\n");
        ASSERT_TRUE(provider.has_value());
        ASSERT_TRUE(shader.has_value());
        const AZStd::string includeArgument = AZStd::string::format("-I%s", root.GetDirectory());
        JE::Assets::RenderBuilders::Shader::PreprocessorData output;
        ASSERT_TRUE(JE::Assets::RenderBuilders::Shader::PreprocessFile(shader->c_str(), output, { includeArgument }, true))
            << output.diagnostics.c_str();
        // Remove the existing output/report hooks before invoking MCPP outside a binder.
        // Do not clear the new resolver here: the destructor is what this check exercises.
        mcpp_reset_def_out_func();
        mcpp_set_report_include_callback(nullptr);
        mcpp_use_mem_buffers(1);
        const char* argv[] = { "builder", shader->c_str(), includeArgument.c_str(), "-P", nullptr };
        const int rawResult = mcpp_lib_main(4, argv);
        mcpp_use_mem_buffers(0);
        EXPECT_NE(rawResult, 0);
    }

} //namespace UnitTest

//AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

