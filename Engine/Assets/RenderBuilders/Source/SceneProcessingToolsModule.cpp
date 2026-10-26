/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <SceneProcessing/SceneProcessingModuleState.h>
#include <SceneProcessing/Config/Components/SceneProcessingConfigSystemComponent.h>
#include <SceneProcessing/Config/Components/SoftNameBehavior.h>
#include <SceneProcessing/SceneBuilder/SceneBuilderComponent.h>
#include <SceneProcessing/SceneBuilder/SceneSerializationHandler.h>
#include <SceneProcessing/Generation/Components/TangentGenerator/TangentGenerateComponent.h>
#include <SceneProcessing/Generation/Components/TangentGenerator/TangentPreExportComponent.h>
#include <SceneProcessing/Generation/Components/UVsGenerator/UVsGenerateComponent.h>
#include <SceneProcessing/Generation/Components/UVsGenerator/UVsPreExportComponent.h>
#include <SceneProcessing/Generation/Components/MeshOptimizer/MeshOptimizerComponent.h>
#include <SceneAPI/SceneBuilder/ImportContextRegistryComponent.h>

namespace JE::Assets::RenderBuilders::SceneProcessing
{
    namespace
    {
        AZStd::unique_ptr<AZ::DynamicModuleHandle> s_sceneCoreModule;
        AZStd::unique_ptr<AZ::DynamicModuleHandle> s_sceneDataModule;
        AZStd::unique_ptr<AZ::DynamicModuleHandle> s_sceneBuilderModule;
    }

    AZStd::unique_ptr<AZ::DynamicModuleHandle>& GetSceneCoreModule() { return s_sceneCoreModule; }
    AZStd::unique_ptr<AZ::DynamicModuleHandle>& GetSceneDataModule() { return s_sceneDataModule; }
    AZStd::unique_ptr<AZ::DynamicModuleHandle>& GetSceneBuilderModule() { return s_sceneBuilderModule; }

    class SceneProcessingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI_WITH_NAME(SceneProcessingModule, "SceneProcessingModule", "{13DCFEF2-BB25-4DBB-A69B-22958CAD6885}", AZ::Module);

        SceneProcessingModule()
        {
            LoadSceneModule(GetSceneCoreModule(), "SceneCore");
            LoadSceneModule(GetSceneDataModule(), "SceneData");
            LoadSceneModule(GetSceneBuilderModule(), "SceneBuilder");
            m_descriptors.insert(m_descriptors.end(),
            {
                Config::SceneProcessingConfigSystemComponent::CreateDescriptor(),
                Config::SoftNameBehavior::CreateDescriptor(),
                AZ::SceneAPI::SceneBuilder::ImportContextRegistryComponent::CreateDescriptor(),
                Builder::BuilderPluginComponent::CreateDescriptor(),
                Builder::SceneSerializationHandler::CreateDescriptor(),
                Generation::TangentPreExportComponent::CreateDescriptor(),
                Generation::TangentGenerateComponent::CreateDescriptor(),
                Generation::CreateUVsGenerateComponentDescriptor(),
                Generation::CreateUVsPreExportComponentDescriptor(),
                Generation::MeshOptimizerComponent::CreateDescriptor(),
            });

            AZStd::vector<AZ::Uuid> typeIds;
            typeIds.reserve(m_descriptors.size());
            for (AZ::ComponentDescriptor* descriptor : m_descriptors)
            {
                typeIds.emplace_back(descriptor->GetUuid());
            }
            AzFramework::MetricsPlainTextNameRegistrationBus::Broadcast(
                &AzFramework::MetricsPlainTextNameRegistrationBus::Events::RegisterForNameSending, typeIds);
        }

        ~SceneProcessingModule() override
        {
            UnloadModule(GetSceneBuilderModule());
            UnloadModule(GetSceneDataModule());
            UnloadModule(GetSceneCoreModule());
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return { azrtti_typeid<Config::SceneProcessingConfigSystemComponent>() };
        }

        void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SceneConfiguration"));
        }

        void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SceneConfiguration"));
        }

    protected:
        void LoadSceneModule(AZStd::unique_ptr<AZ::DynamicModuleHandle>& module, const char* name)
        {
            if (!module)
            {
                module = AZ::DynamicModuleHandle::Create(name);
                if (module)
                {
                    module->Load();
                    auto init = module->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
                    if (init)
                    {
                        (*init)();
                    }
                }
            }
        }

        void UnloadModule(AZStd::unique_ptr<AZ::DynamicModuleHandle>& module)
        {
            if (module)
            {
                auto uninit = module->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
                if (uninit)
                {
                    (*uninit)();
                }
                module.reset();
            }
        }
    };
} // namespace JE::Assets::RenderBuilders::SceneProcessing

namespace JE::Assets::RenderBuilders
{
    AZ::Module* CreateSceneProcessingToolsModule()
    {
        return aznew SceneProcessing::SceneProcessingModule();
    }
} // namespace JE::Assets::RenderBuilders
