/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Builders/LuaBuilder/LuaBuilderWorker.h>
#include <AzTest/Utils.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzFramework/Network/SocketConnection.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>

namespace UnitTest
{
    using namespace LuaBuilder;
    using namespace AssetBuilderSDK;

    class LuaBuilderTests
        : public UnitTest::LeakDetectionFixture
    {
        void SetUp() override
        {
            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(m_descriptor, startupParameters);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            const AZStd::string engineRoot = AZ::Test::GetEngineRootPath();
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@engroot@", engineRoot.c_str());

            AZ::IO::Path assetRoot(AZ::Utils::GetProjectPath());
            assetRoot /= "Cache";
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", assetRoot.c_str());

            // Set the @gemroot:<gem-name> alias for LmbrCentral gem
            AZ::Test::AddActiveGem("LmbrCentral", *registry, AZ::IO::FileIOBase::GetInstance());
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;
    };

    TEST_F(LuaBuilderTests, ParseLuaScript_DependencyRegexStress_Success)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/dependencyRegexStress.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("test0.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test1.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/separated/test2.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/separated/test3.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test4.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("test5.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test6.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("test7.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test8.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test9.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test10.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test11.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test12.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test13.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test14.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test15.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/test16.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("test17.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("local_test0.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test1.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/separated/local_test2.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/separated/local_test3.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test4.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("local_test5.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test6.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("local_test7.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test8.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test9.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test10.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test11.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test12.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test13.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test14.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test15.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("folder/local_test16.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("local_test17.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile)
            )
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_UsingRequire_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test1.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("scripts/test2.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("scripts/test3.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("scripts/test4.luac", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_UsingReloadScript_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test2.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("some/path/test3.lua", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_WithPathInAString_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test3_general_dependencies.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("some/file/in/some/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_WithRegistryKeysAndOutputPaths_ShouldOnlyFindRelativeFileDependency)
    {
        LuaBuilderWorker worker;
        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(
            resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test9_registry_and_output_paths.lua");
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(resolvedPath.c_str()));

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("some/file/in/some/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_WithConsoleCommand_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test4_console_command.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("somefile.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("somefile/in/a/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    void VerifyNoDependenciesGenerated(const AZStd::string& testFileUnresolvedPath)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, testFileUnresolvedPath.c_str());

        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(resolvedPath.c_str()));

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        EXPECT_TRUE(pathDependencies.empty());
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_EntireLine_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@gemroot:LmbrCentral@/Code/Tests/Lua/test5_whole_line_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_PartialLine_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@gemroot:LmbrCentral@/Code/Tests/Lua/test6_partial_line_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_BlockComment_ShouldFindNoDependencies)
    {
        VerifyNoDependenciesGenerated("@gemroot:LmbrCentral@/Code/Tests/Lua/test7_block_comment.lua");
    }

    TEST_F(LuaBuilderTests, ParseLuaScript_CommentedOutDependency_NegatedBlockComment_ShouldFindDependencies)
    {
        LuaBuilderWorker worker;

        AssetBuilderSDK::ProductPathDependencySet pathDependencies;

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, "@gemroot:LmbrCentral@/Code/Tests/Lua/test8_negated_block_comment.lua");

        worker.ParseDependencies(resolvedPath.c_str(), pathDependencies);

        ASSERT_THAT(pathDependencies,
            testing::UnorderedElementsAre(
                ProductPathDependency("somefile.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile),
                ProductPathDependency("somefile/in/a/folder.cfg", AssetBuilderSDK::ProductPathDependencyType::ProductFile))
        );
    }

    // Replace only the external AP source-info lookup; parsing, compilation, serialization and job output stay real.
    class ScopedLuaBuilderSourceInfo
        : public AzToolsFramework::AssetSystem::AssetSystemComponent
    {
    public:
        ScopedLuaBuilderSourceInfo(AzToolsFramework::AssetSystem::AssetSystemComponent& original, const AZ::Uuid& scriptGuid)
            : m_original(original)
            , m_scriptGuid(scriptGuid)
        {
            m_original.Deactivate();
            Activate();
        }

        ~ScopedLuaBuilderSourceInfo() override
        {
            Deactivate();
            m_original.Activate();
        }

        bool GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override
        {
            if (AZStd::string_view(sourcePath) != "scripts/dependency.lua")
            {
                return false;
            }
            assetInfo.m_assetId = AZ::Data::AssetId(m_scriptGuid, 0);
            assetInfo.m_relativePath = "scripts/dependency.lua";
            watchFolder.clear();
            return true;
        }

    private:
        AzToolsFramework::AssetSystem::AssetSystemComponent& m_original;
        AZ::Uuid m_scriptGuid;
    };

    TEST_F(LuaBuilderTests, ProcessJob_MixedLuaAndProductPaths_PreservesBothDependencyKinds)
    {
        AZ::Entity* systemEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            systemEntity, &AZ::ComponentApplicationRequests::FindEntity, AZ::SystemEntityId);
        ASSERT_NE(systemEntity, nullptr);
        auto* assetSystem = systemEntity->FindComponent<AzToolsFramework::AssetSystem::AssetSystemComponent>();
        ASSERT_NE(assetSystem, nullptr);
        ASSERT_NE(AzFramework::SocketConnection::GetInstance(), nullptr);
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(serializeContext, nullptr);
        ASSERT_NE(AzToolsFramework::AssetSystemRequestBus::FindFirstHandler(), nullptr);
        const AZ::Uuid scriptGuid("{353DA8E2-BDAD-446C-95C2-53D34FAED222}");
        ScopedLuaBuilderSourceInfo sourceInfo(*assetSystem, scriptGuid);
        AZ::Test::ScopedAutoTempDirectory tempDirectory;
        const auto script = AZ::Test::CreateTestFile(tempDirectory, "mixed.lua",
            "require(\"scripts.dependency\")\n"
            "local texture = \"materials/presets/macbeth/05_blue_flower_srgb.tif.streamingimage\"\n"
            "ConsoleRequestBus.Broadcast.ExecuteConsoleCommand(\"exec configs/render.cfg\")\n"
            "return {}\n");
        ASSERT_TRUE(script.has_value());
        LuaBuilderWorker worker;
        CreateJobsRequest createRequest;
        createRequest.m_watchFolder = tempDirectory.GetDirectory();
        createRequest.m_sourceFile = "mixed.lua";
        PlatformInfo platform;
        platform.m_identifier = "pc";
        createRequest.m_enabledPlatforms.push_back(platform);
        CreateJobsResponse createResponse;
        AZ_TEST_START_TRACE_SUPPRESSION;
        worker.CreateJobs(createRequest, createResponse);
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
        ASSERT_EQ(createResponse.m_result, CreateJobsResultCode::Success);
        ASSERT_EQ(createResponse.m_createJobOutputs.size(), 1);
        const auto& job = createResponse.m_createJobOutputs.front();
        ASSERT_EQ(job.m_jobDependencyList.size(), 1);
        EXPECT_EQ(job.m_jobDependencyList.front().m_sourceFile.m_sourceFileDependencyUUID, scriptGuid);
        EXPECT_EQ(job.m_jobDependencyList.front().m_jobKey, "Lua Compile");
        ProcessJobRequest request;
        request.m_fullPath = script->c_str();
        request.m_sourceFile = "mixed.lua";
        request.m_tempDirPath = tempDirectory.GetDirectory();
        request.m_jobDescription = job;
        ProcessJobResponse response;
        AZ_TEST_START_TRACE_SUPPRESSION;
        worker.ProcessJob(request, response);
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
        ASSERT_EQ(response.m_resultCode, ProcessJobResult_Success);
        ASSERT_EQ(response.m_outputProducts.size(), 1);
        const auto& product = response.m_outputProducts.front();
        EXPECT_EQ(product.m_productAssetType, azrtti_typeid<AZ::ScriptAsset>());
        EXPECT_EQ(product.m_productSubID, AZ::ScriptAsset::CompiledAssetSubId);
        EXPECT_TRUE(product.m_dependenciesHandled);
        ASSERT_EQ(product.m_dependencies.size(), 1);
        EXPECT_EQ(product.m_dependencies.front().m_dependencyId, AZ::Data::AssetId(scriptGuid, AZ::ScriptAsset::CompiledAssetSubId));
        EXPECT_EQ(product.m_dependencies.front().m_flags,
            AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad));
        EXPECT_THAT(product.m_pathDependencies, testing::UnorderedElementsAre(
            ProductPathDependency("materials/presets/macbeth/05_blue_flower_srgb.tif.streamingimage", ProductPathDependencyType::ProductFile),
            ProductPathDependency("configs/render.cfg", ProductPathDependencyType::ProductFile)));
        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(tempDirectory.Resolve(product.m_productFileName.c_str()).c_str()));
    }

    TEST_F(LuaBuilderTests, ProcessJob_MissingRequiredLua_PreservesDependencyError)
    {
        AZ::Entity* systemEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            systemEntity, &AZ::ComponentApplicationRequests::FindEntity, AZ::SystemEntityId);
        ASSERT_NE(systemEntity, nullptr);
        auto* assetSystem = systemEntity->FindComponent<AzToolsFramework::AssetSystem::AssetSystemComponent>();
        ASSERT_NE(assetSystem, nullptr);
        ASSERT_NE(AzFramework::SocketConnection::GetInstance(), nullptr);
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        ASSERT_NE(serializeContext, nullptr);
        ASSERT_NE(AzToolsFramework::AssetSystemRequestBus::FindFirstHandler(), nullptr);
        ScopedLuaBuilderSourceInfo sourceInfo(*assetSystem, AZ::Uuid("{353DA8E2-BDAD-446C-95C2-53D34FAED222}"));
        AZ::Test::ScopedAutoTempDirectory tempDirectory;
        const auto script = AZ::Test::CreateTestFile(tempDirectory, "missing.lua", "require(\"scripts.missing\")\nreturn {}\n");
        ASSERT_TRUE(script.has_value());
        LuaBuilderWorker worker;
        ProcessJobRequest request;
        request.m_fullPath = script->c_str();
        request.m_sourceFile = "missing.lua";
        request.m_tempDirPath = tempDirectory.GetDirectory();
        request.m_jobDescription.m_jobParameters[AZ_CRC_CE("BuildType")] = "Compiled";
        ProcessJobResponse response;
        AZ_TEST_START_TRACE_SUPPRESSION;
        worker.ProcessJob(request, response);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        ASSERT_EQ(response.m_resultCode, ProcessJobResult_Success);
        ASSERT_EQ(response.m_outputProducts.size(), 1);
        EXPECT_TRUE(response.m_outputProducts.front().m_dependencies.empty());
        EXPECT_TRUE(response.m_outputProducts.front().m_pathDependencies.empty());
    }
}
