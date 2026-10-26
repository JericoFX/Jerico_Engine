/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <JE/Assets/RenderBuilders/SceneProcessing/Config/SceneProcessingConfigBus.h>
#include <SceneProcessing/SceneProcessingModuleState.h>
#include <SceneProcessing/SceneBuilder/SceneBuilderComponent.h>
#include <SceneProcessing/SceneBuilder/SceneBuilderWorker.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/ScriptConfigEventBus.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

namespace UnitTest
{
    namespace Scene = JE::Assets::RenderBuilders::SceneProcessing;

    // Control the external extension boundary; the module, builder and callbacks remain real.
    class SceneProcessingTestExtensions final
        : public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
    {
    public:
        SceneProcessingTestExtensions() { BusConnect(); }
        ~SceneProcessingTestExtensions() override { BusDisconnect(); }
        void GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions) override
        {
            extensions.insert(".fbx");
            extensions.insert("gltf");
            extensions.insert("");
        }
    };

    class SceneBuilderRegistrationCapture final
        : public AssetBuilderSDK::AssetBuilderBus::Handler
    {
    public:
        SceneBuilderRegistrationCapture() { BusConnect(); }
        ~SceneBuilderRegistrationCapture() override { BusDisconnect(); }
        void RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& descriptor) override
        {
            ++m_registrations;
            m_descriptor = descriptor;
        }
        size_t m_registrations = 0;
        AZStd::optional<AssetBuilderSDK::AssetBuilderDesc> m_descriptor;
    };

    class SceneBuilderActivationScope final
    {
    public:
        SceneBuilderActivationScope() { m_builder.Activate(); }
        ~SceneBuilderActivationScope() { m_builder.Deactivate(); }
    private:
        Scene::Builder::BuilderPluginComponent m_builder;
    };

    void VerifySceneProcessingModuleCycles(AZ::ModuleInitializationSteps lastStep)
    {
        const bool activateRequiredConfig = lastStep == AZ::ModuleInitializationSteps::ActivateEntity;
        const AZ::Uuid configId("{80FE1130-91B4-44D4-869F-859BB996161A}");
        const AZ::Uuid builderId("{BD8BF658-9485-4FE3-830E-8EC3A23C35F3}");
        const AZStd::vector<AZ::Uuid> expectedDescriptors{
            configId,
            AZ::Uuid("{C2A9D207-485F-4752-B37B-388B0A52A956}"),
            AZ::Uuid("{9453DDF4-882C-4675-86EB-834F1D1DC5EF}"),
            AZ::Uuid("{47BB00DE-2C6F-4A8E-9DCF-9A226DF0D649}"),
            AZ::Uuid("{5917845E-2A6A-4C6C-BD02-E9CECC8D4E13}"),
            AZ::Uuid("{BFFE114A-2FC6-42F1-92C4-61329CC54A2B}"),
            AZ::Uuid("{57743E6F-8718-491C-8A82-24A6763904F5}"),
            AZ::Uuid("{49121BDD-C7E5-4D39-89BC-28789C90057F}"),
            AZ::Uuid("{64F79C1E-CED6-42A9-8229-6607F788C731}"),
            AZ::Uuid("{05791580-A464-436C-B3EA-36AD68A42BC8}")
        };
        AZStd::string firstFingerprint;
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            EXPECT_FALSE(Scene::GetSceneCoreModule());
            EXPECT_FALSE(Scene::GetSceneDataModule());
            EXPECT_FALSE(Scene::GetSceneBuilderModule());
            AzToolsFramework::ToolsApplication application;
            AZ::ComponentApplication::StartupParameters startup;
            startup.m_loadSettingsRegistry = false;
            startup.m_loadDynamicModules = false;
            application.Start(AZ::ComponentApplication::Descriptor(), startup);
            auto* registry = AZ::SettingsRegistry::Get();
            ASSERT_NE(registry, nullptr);
            ASSERT_TRUE(registry->MergeSettings(
                R"({"O3DE":{"AssetProcessor":{"SceneBuilder":{"NodeSoftNameSettings":[],"FileSoftNameSettings":[]}}}})",
                AZ::SettingsRegistryInterface::Format::JsonMergePatch));
            {
                SceneProcessingTestExtensions extensions;
                AZ::ModuleManagerRequests::LoadModulesResult modules;
                AZ::ModuleManagerRequestBus::BroadcastResult(
                    modules, &AZ::ModuleManagerRequests::LoadStaticModules,
                    [](AZStd::vector<AZ::Module*>& result)
                    {
                        result.push_back(JE::Assets::RenderBuilders::CreateSceneProcessingToolsModule());
                    }, lastStep);
                ASSERT_EQ(modules.size(), 1);
                ASSERT_TRUE(modules.front().IsSuccess());
                const auto* moduleData = modules.front().GetValue().get();
                if (activateRequiredConfig)
                {
                    ASSERT_NE(moduleData->GetEntity(), nullptr);
                    EXPECT_EQ(moduleData->GetEntity()->GetState(), AZ::Entity::State::Active);
                }
                else
                {
                    EXPECT_EQ(moduleData->GetEntity(), nullptr);
                }
                const auto* module = moduleData->GetModule();
                ASSERT_NE(module, nullptr);
                EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{13DCFEF2-BB25-4DBB-A69B-22958CAD6885}"));
                EXPECT_STREQ(module->RTTI_GetTypeName(), "SceneProcessingModule");
                EXPECT_EQ(module->GetRequiredSystemComponents(), AZ::ComponentTypeList{configId});
                AZStd::vector<AZ::Uuid> actualDescriptors;
                for (const auto* descriptor : module->GetComponentDescriptors())
                {
                    ASSERT_NE(descriptor, nullptr);
                    actualDescriptors.push_back(descriptor->GetUuid());
                }
                EXPECT_EQ(actualDescriptors, expectedDescriptors);
                ASSERT_TRUE(Scene::GetSceneCoreModule());
                EXPECT_TRUE(Scene::GetSceneCoreModule()->IsLoaded());
                ASSERT_TRUE(Scene::GetSceneDataModule());
                EXPECT_TRUE(Scene::GetSceneDataModule()->IsLoaded());
                ASSERT_TRUE(Scene::GetSceneBuilderModule());
                EXPECT_TRUE(Scene::GetSceneBuilderModule()->IsLoaded());
                EXPECT_EQ(Scene::Config::SceneProcessingConfigRequestBus::HasHandlers(), activateRequiredConfig);
                EXPECT_EQ(AZ::SceneAPI::Events::ScriptConfigEventBus::HasHandlers(), activateRequiredConfig);
                // Reflection crosses from the factory's DLL into the real SceneData provider.
                ASSERT_NE(application.GetSerializeContext()->FindClassData(
                    azrtti_typeid<AZ::SceneData::GraphData::MeshData>()), nullptr);
                Scene::Builder::SceneBuilderWorker fingerprintWorker;
                const AZStd::string fingerprint = fingerprintWorker.GetFingerprint();
                EXPECT_NE(fingerprint.find("[TangentGenerateComponent:v3]"), AZStd::string::npos);
                EXPECT_NE(fingerprint.find("[MeshOptimizerComponent:v12]"), AZStd::string::npos);
                EXPECT_NE(fingerprint.find("Version 5"), AZStd::string::npos);
                if (cycle == 0)
                {
                    firstFingerprint = fingerprint;
                }
                else
                {
                    EXPECT_EQ(fingerprint, firstFingerprint);
                }
                if (!activateRequiredConfig)
                {
                    SceneBuilderRegistrationCapture capture;
                    Scene::Config::SceneProcessingConfigRequests* configHandler = nullptr;
                    {
                        SceneBuilderActivationScope activeBuilder;
                        ASSERT_TRUE(Scene::Config::SceneProcessingConfigRequestBus::HasHandlers());
                        ASSERT_TRUE(AZ::SceneAPI::Events::ScriptConfigEventBus::HasHandlers());
                        configHandler = Scene::Config::SceneProcessingConfigRequestBus::FindFirstHandler();
                        ASSERT_NE(configHandler, nullptr);
                        ASSERT_EQ(capture.m_registrations, 1);
                        ASSERT_TRUE(capture.m_descriptor.has_value());
                        auto& descriptor = capture.m_descriptor.value();
                        EXPECT_EQ(descriptor.m_busId, builderId);
                        EXPECT_EQ(descriptor.m_version, 13);
                        EXPECT_EQ(descriptor.m_name, "Scene Builder");
                        EXPECT_EQ(descriptor.m_analysisFingerprint, fingerprint);
                        EXPECT_TRUE(AZStd::any_of(descriptor.m_patterns.begin(), descriptor.m_patterns.end(),
                            [](const auto& pattern) { return pattern.m_pattern == "*.fbx"; }));
                        EXPECT_TRUE(AZStd::any_of(descriptor.m_patterns.begin(), descriptor.m_patterns.end(),
                            [](const auto& pattern) { return pattern.m_pattern == "*.gltf"; }));
                        ASSERT_TRUE(descriptor.m_createJobFunction);
                        ASSERT_TRUE(descriptor.m_processJobFunction);
                        ASSERT_TRUE(AssetBuilderSDK::AssetBuilderCommandBus::HasHandlers(builderId));
                        AssetBuilderSDK::AssetBuilderCommandBus::Event(
                            builderId, &AssetBuilderSDK::AssetBuilderCommandBus::Events::ShutDown);
                        AssetBuilderSDK::CreateJobsResponse createResponse;
                        descriptor.m_createJobFunction(AssetBuilderSDK::CreateJobsRequest{}, createResponse);
                        EXPECT_EQ(createResponse.m_result, AssetBuilderSDK::CreateJobsResultCode::ShuttingDown);
                        AssetBuilderSDK::ProcessJobResponse processResponse;
                        descriptor.m_processJobFunction(AssetBuilderSDK::ProcessJobRequest{}, processResponse);
                        EXPECT_EQ(processResponse.m_resultCode, AssetBuilderSDK::ProcessJobResult_Cancelled);
                    }
                    EXPECT_FALSE(AssetBuilderSDK::AssetBuilderCommandBus::HasHandlers(builderId));
                    // Scene System is owned by the application; BuilderDeactivate only disconnects its worker.
                    EXPECT_EQ(Scene::Config::SceneProcessingConfigRequestBus::FindFirstHandler(), configHandler);
                    EXPECT_TRUE(AZ::SceneAPI::Events::ScriptConfigEventBus::HasHandlers());
                    ASSERT_TRUE(Scene::GetSceneCoreModule());
                    EXPECT_TRUE(Scene::GetSceneCoreModule()->IsLoaded());
                    ASSERT_TRUE(Scene::GetSceneDataModule());
                    EXPECT_TRUE(Scene::GetSceneDataModule()->IsLoaded());
                    ASSERT_TRUE(Scene::GetSceneBuilderModule());
                    EXPECT_TRUE(Scene::GetSceneBuilderModule()->IsLoaded());
                }
            }
            application.Stop();
            EXPECT_FALSE(Scene::Config::SceneProcessingConfigRequestBus::HasHandlers());
            EXPECT_FALSE(AZ::SceneAPI::Events::ScriptConfigEventBus::HasHandlers());
            EXPECT_FALSE(Scene::GetSceneCoreModule());
            EXPECT_FALSE(Scene::GetSceneDataModule());
            EXPECT_FALSE(Scene::GetSceneBuilderModule());
        }
    }

    TEST_F(LeakDetectionFixture, NativeSceneProcessingEditorActivatePreservesRequiredConfigReflectionAndTwoCycleLifetime)
    {
        VerifySceneProcessingModuleCycles(AZ::ModuleInitializationSteps::ActivateEntity);
    }

    TEST_F(LeakDetectionFixture, NativeSceneProcessingAssetBuilderDescriptorsPreserveCallbacksAndTwoCycleLifetime)
    {
        VerifySceneProcessingModuleCycles(AZ::ModuleInitializationSteps::RegisterComponentDescriptors);
    }
} // namespace UnitTest
