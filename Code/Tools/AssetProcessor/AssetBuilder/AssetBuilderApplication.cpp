/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <JE/Editor/Tools/Module.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <JE/Geometry/GeometryModule.h>
#include <JE/Editor/Camera/CameraModule.h>
#include <JE/World/Camera/Debug/DebugCameraModule.h>
#include <JE/Editor/RenderIntegration/AssetProcessingModule.h>
#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>
#include <JE/Editor/Render/Scene/SceneModule.h>
#include <JE/Editor/Render/Features/FeatureModule.h>
#include <JE/Render/Integration/RenderIntegrationModule.h>
#include <JE/Assets/Prefab/PrefabBuilderModule.h>
#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <JE/Debug/ImGui/ImGuiModule.h>
#include <JE/Debug/Profiler/ProfilerModule.h>
#include <JE/Render/Materials/MaterialsRegistration.h>
#include <JE/Editor/Materials/MaterialsRegistration.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityModelComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySearchComponent.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextComponent.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>
#include <AzToolsFramework/Metadata/UuidUtils.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderApplication.h>
#include <AssetBuilderComponent.h>
#include <AssetBuilderInfo.h>
#include <AzCore/Interface/Interface.h>
#include <Entity/EntityUtilityComponent.h>
#include <AssetBuilderStatic.h>

namespace AssetBuilder
{
    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }
}

AZ::ComponentTypeList AssetBuilderApplication::GetRequiredSystemComponents() const
{
    AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

    for (auto iter = components.begin(); iter != components.end();)
    {
        if (*iter == azrtti_typeid<AZ::UserSettingsComponent>()
            || *iter == azrtti_typeid<AzFramework::InputSystemComponent>()
            || *iter == azrtti_typeid<AzFramework::AssetCatalogComponent>()
            )
        {
            iter = components.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    components.insert(components.end(), {
        azrtti_typeid<AZ::SliceSystemComponent>(),
        azrtti_typeid<AzToolsFramework::SliceMetadataEntityContextComponent>(),
        azrtti_typeid<AssetBuilderComponent>(),
        azrtti_typeid<AssetProcessor::ToolsAssetCatalogComponent>(),
        azrtti_typeid<AzToolsFramework::AssetSystem::AssetSystemComponent>(),
        azrtti_typeid<AzToolsFramework::Components::EditorComponentAPIComponent>(),
        azrtti_typeid<AzToolsFramework::Components::EditorEntityActionComponent>(),
        azrtti_typeid<AzToolsFramework::Components::EditorEntitySearchComponent>(),
        azrtti_typeid<AzToolsFramework::Components::EditorEntityModelComponent>(),
        azrtti_typeid<AzToolsFramework::EditorEntityContextComponent>(),
        azrtti_typeid<AzToolsFramework::Prefab::PrefabSystemComponent>(),
        azrtti_typeid<AzToolsFramework::EntityUtilityComponent>(),
        azrtti_typeid<AzToolsFramework::MetadataManager>(),
        azrtti_typeid<AzToolsFramework::UuidUtilComponent>(),
        });

    return components;
}

AssetBuilderApplication::AssetBuilderApplication(int* argc, char*** argv)
    : AssetBuilderApplication(argc, argv, {})
{
}

AssetBuilderApplication::AssetBuilderApplication(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings)
    : AzToolsFramework::ToolsApplication(argc, argv, AZStd::move(componentAppSettings))
    , m_qtApplication(*argc, *argv)
{
    // The settings registry has been created at this point
    auto settingsRegistry = AZ::SettingsRegistry::Get();
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
        *settingsRegistry, AssetBuilder::GetBuildTargetName());

    AZ::Interface<IBuilderApplication>::Register(this);
}

AssetBuilderApplication::~AssetBuilderApplication()
{
    AZ::Interface<IBuilderApplication>::Unregister(this);
}

void AssetBuilderApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
{
    AzToolsFramework::ToolsApplication::CreateStaticModules(outModules);
    outModules.emplace_back(JE::Geometry::CreateAuthoringModule());
    outModules.emplace_back(JE::Editor::Camera::CreateAuthoringModule());
    outModules.emplace_back(JE::World::Camera::Debug::CreateDebugModule());
    outModules.emplace_back(JE::Render::Integration::CreateBootstrapModule(false));
    outModules.emplace_back(JE::Editor::RenderIntegration::CreateAssetProcessingModule());
    outModules.emplace_back(JE::Assets::Prefab::CreateToolsModule());
    outModules.emplace_back(JE::Debug::CreateToolsModule());
    outModules.emplace_back(JE::Debug::Profiler::CreateCaptureModule());
}

void AssetBuilderApplication::RegisterCoreComponents()
{
    AzToolsFramework::ToolsApplication::RegisterCoreComponents();

    // Prefab conversion needs the authored and runtime types, but not the
    // material browser/preview system used by the interactive editor.
    RegisterComponentDescriptor(JE::Render::CreateMaterialComponentDescriptor());
    RegisterComponentDescriptor(JE::Editor::CreateEditorMaterialComponentDescriptor());

    RegisterComponentDescriptor(AssetBuilderComponent::CreateDescriptor());
    RegisterComponentDescriptor(AssetProcessor::ToolsAssetCatalogComponent::CreateDescriptor());
}

void AssetBuilderApplication::StartCommon(AZ::Entity* systemEntity)
{
    InstallCtrlHandler();

    // Merge in the SettingsRegistry for the game being processed. This does not
    // necessarily correspond to the project name in the bootstrap.cfg since it
    // the AssetBuilder supports overriding the project-path on the command line
    AZ::SettingsRegistryInterface& registry = *AZ::SettingsRegistry::Get();

    // Retrieve specializations from the Settings Registry and ComponentApplication derived classes
    AZ::SettingsRegistryInterface::Specializations specializations;
    SetSettingsRegistrySpecializations(specializations);

    // Merge the SettingsRegistry file again using gameName as an additional specialization
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(registry,
        AZ_TRAIT_OS_PLATFORM_CODENAME, specializations);

    AzToolsFramework::ToolsApplication::StartCommon(systemEntity);

#if defined(AZ_PLATFORM_MAC)
    // The asset builder needs to start astcenc as a child process to compress textures.
    // astcenc is started by the PVRTexLib dynamic library. In order for it to be able to find
    // the executable, we need to set the PATH environment variable.
    AZStd::string exeFolder;
    AZ::ComponentApplicationBus::BroadcastResult(exeFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);

    setenv("PATH", exeFolder.c_str(), 1);
#endif // AZ_PLATFORM_MAC

    // Make sure a project path was set to settings registry and error/warn if not.
    auto projectPath = AZ::Utils::GetProjectPath();
    if (projectPath.empty())
    {
        if (IsInDebugMode())
        {
            AZ_Error("AssetBuilder", false, "Unable to determine the project path automatically. "
                "Make sure a default project path has been set or provide a --project-path option on the command line. "
                "(See -help for more info.)");
            return;
        }
        else
        {
            AZ_Printf(AssetBuilderSDK::InfoWindow, "project-path not specified on the command line, assuming current directory.\n");
            AZ_Printf(AssetBuilderSDK::InfoWindow, "project-path is best specified as the full path to the project's folder.");
        }
    }

    // Builders need native render reflection without activating renderer modules or an RHI backend.
    // Reflect native presets before dynamic tools consume them; keep required-component contracts intact.
    if (!systemEntity || systemEntity->GetState() != AZ::Entity::State::Active)
    {
        AZ_Error("AssetBuilder", false, "Native render descriptor registration requires an active system entity.");
        ExitMainLoop();
        return;
    }

    AZ::ModuleManagerRequests::LoadModulesResult renderModules;
    AZ::ModuleManagerRequestBus::BroadcastResult(
        renderModules, &AZ::ModuleManagerRequests::LoadStaticModules,
        [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(JE::Editor::RenderIntegration::CreateRpiEditorModule());
            modules.emplace_back(JE::Editor::Render::Scene::CreateModule());
            modules.emplace_back(JE::Editor::Render::Features::CreateModule());
        },
        AZ::ModuleInitializationSteps::RegisterComponentDescriptors);

    if (renderModules.size() != 3)
    {
        AZ_Error("AssetBuilder", false, "Failed to register the three native render modules.");
        ExitMainLoop();
        return;
    }
    for (const auto& moduleResult : renderModules)
    {
        if (!moduleResult.IsSuccess())
        {
            AZ_Error("AssetBuilder", false, "%s", moduleResult.GetError().c_str());
            ExitMainLoop();
            return;
        }
        if (moduleResult.GetValue()->GetEntity() != nullptr)
        {
            AZ_Error("AssetBuilder", false, "Native render descriptor-only module unexpectedly created an entity.");
            ExitMainLoop();
            return;
        }
    }

    // The live script context needs native preset reflection before the tools bus is registered.
    AZ::ModuleManagerRequests::LoadModulesResult toolsModules;
    AZ::ModuleManagerRequestBus::BroadcastResult(
        toolsModules, &AZ::ModuleManagerRequests::LoadStaticModules,
        [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(JE::Editor::Tools::CreateModule());
        },
        AZ::ModuleInitializationSteps::ActivateEntity);
    if (toolsModules.size() != 1)
    {
        AZ_Error("AssetBuilder", false, "Failed to load the native editor tools module.");
        ExitMainLoop();
        return;
    }
    const auto& toolsModule = toolsModules.front();
    if (!toolsModule.IsSuccess())
    {
        AZ_Error("AssetBuilder", false, "%s", toolsModule.GetError().c_str());
        ExitMainLoop();
        return;
    }
    const AZ::Entity* toolsEntity = toolsModule.GetValue()->GetEntity();
    if (!toolsEntity || toolsEntity->GetState() != AZ::Entity::State::Active)
    {
        AZ_Error("AssetBuilder", false, "Native editor tools module did not activate its required components.");
        ExitMainLoop();
        return;
    }

    // Loads dynamic modules and registers any component descriptors populated into the AZ::Module m_descriptor list
    // for each instantiated module class
    LoadDynamicModules();

    // Scene processing keeps SceneAPI loaded, but its configuration is created by
    // the scene builder entity. Preserve descriptor-only registration in this host.
    AZ::ModuleManagerRequests::LoadModulesResult sceneProcessingModules;
    AZ::ModuleManagerRequestBus::BroadcastResult(
        sceneProcessingModules, &AZ::ModuleManagerRequests::LoadStaticModules,
        [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(JE::Assets::RenderBuilders::CreateSceneProcessingToolsModule());
        },
        AZ::ModuleInitializationSteps::RegisterComponentDescriptors);
    if (sceneProcessingModules.size() != 1)
    {
        AZ_Error("AssetBuilder", false, "Failed to load the native scene processing module.");
        ExitMainLoop();
        return;
    }
    const auto& sceneProcessingModule = sceneProcessingModules.front();
    if (!sceneProcessingModule.IsSuccess())
    {
        AZ_Error("AssetBuilder", false, "%s", sceneProcessingModule.GetError().c_str());
        ExitMainLoop();
        return;
    }
    if (sceneProcessingModule.GetValue()->GetEntity() != nullptr)
    {
        AZ_Error("AssetBuilder", false, "Native scene processing descriptor-only module unexpectedly created an entity.");
        ExitMainLoop();
        return;
    }

    // RHI types are now reflected. Register builders without a module entity;
    // InitializeBuilderComponents activates their AssetBuilder-tagged components exactly once.
    AZ::ModuleManagerRequests::LoadModulesResult builderModules;
    AZ::ModuleManagerRequestBus::BroadcastResult(
        builderModules, &AZ::ModuleManagerRequests::LoadStaticModules,
        [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(JE::Assets::RenderBuilders::CreateToolsModule());
            modules.emplace_back(JE::Assets::RenderBuilders::CreateShaderCompilerToolsModule());
        },
        AZ::ModuleInitializationSteps::RegisterComponentDescriptors);

    if (builderModules.size() != 2)
    {
        AZ_Error("AssetBuilder", false, "Failed to register the two native builder modules.");
        ExitMainLoop();
        return;
    }
    for (const auto& moduleResult : builderModules)
    {
        if (!moduleResult.IsSuccess())
        {
            AZ_Error("AssetBuilder", false, "%s", moduleResult.GetError().c_str());
            ExitMainLoop();
            return;
        }
        if (moduleResult.GetValue()->GetEntity() != nullptr)
        {
            AZ_Error("AssetBuilder", false, "Native builder descriptor-only module unexpectedly created an entity.");
            ExitMainLoop();
            return;
        }
    }

    AssetBuilderSDK::InitializeSerializationContext();
    AssetBuilderSDK::InitializeBehaviorContext();
    AssetBuilder::InitializeSerializationContext();
    // the asset builder app never writes source files, only assets, so there is no need to do any kind of asset upgrading
    AZ::Data::AssetManager::Instance().SetAssetInfoUpgradingEnabled(false);

    // Disable parallel dependency loads since the builders can't count on all other assets and their info being ready.
    // Specifically, asset builders can trigger asset loads during the building process.  The ToolsAssetCatalog doesn't
    // implement the dependency APIs, so the asset loads will fail to load any dependent assets.
    //
    // NOTE:  The ToolsAssetCatalog could *potentially* implement the dependency APIs by querying the live Asset Processor instance,
    // but this will return incomplete dependency information based on the subset of assets that have already processed.
    // In theory, if the Asset Builder dependencies are set up correctly, the needed subset should always be processed first,
    // but the one edge case that can't be handled is the case where the Asset Builder intends to filter out the dependent load,
    // but needs to query enough information about the asset (specifically asset type) to know that it can filter it out.  Since
    // the assets are being filtered out, they aren't dependencies, might not be built yet, and so might not have asset type available.
    AZ::Data::AssetManager::Instance().SetParallelDependentLoadingEnabled(false);
}

bool AssetBuilderApplication::IsInDebugMode() const
{
    return AssetBuilderComponent::IsInDebugMode(m_commandLine);
}

void AssetBuilderApplication::InitializeBuilderComponents()
{
    CreateAndAddEntityFromComponentTags(AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }), "AssetBuilders Entity");
}
