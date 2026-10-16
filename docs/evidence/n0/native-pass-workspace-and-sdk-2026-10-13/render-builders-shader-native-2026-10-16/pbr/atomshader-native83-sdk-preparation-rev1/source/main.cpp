#include <JE/Editor/Materials/MaterialsRegistration.h>
#include <JE/Editor/Materials/Graph/PassPipelinePreview.h>
#include <JE/Editor/Camera/CameraModule.h>
#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>
#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <JE/Assets/RenderBuilders/Model/ModelExporterContexts.h>
#include <JE/Assets/RenderBuilders/Shader/HashedVariantListSourceData.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Component/ComponentApplication.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>

namespace
{
    bool CheckModule(const AZ::Module& module)
    {
        if (module.RTTI_GetType() != AZ::Uuid("{CA15BD7F-01B4-4959-BEF2-81FA3AD2C834}") ||
            std::strcmp(module.RTTI_GetTypeName(), "BuilderModule") != 0 ||
            !module.GetRequiredSystemComponents().empty())
        {
            return false;
        }

        std::array<AZ::Uuid, 6> expected{
            AZ::Uuid("{AE42AB62-A4D6-4147-88A0-692549EE5427}"),
            AZ::Uuid("{FE21DEEB-F9E6-487E-B9F0-88478FC2F52F}"),
            AZ::Uuid("{CE1E8C2B-A05F-4AED-8771-88E58377A82B}"),
            AZ::Uuid("{777BB521-FCFE-4382-B8FD-E1EAF846F5C9}"),
            AZ::Uuid("{02163945-D7B4-45D4-9729-4376E1195505}"),
            AZ::Uuid("{7B959232-A812-4F3F-817D-25FEEA844E7C}")};
        const auto& descriptors = module.GetComponentDescriptors();
        if (descriptors.size() != expected.size())
        {
            return false;
        }
        for (const AZ::ComponentDescriptor* descriptor : descriptors)
        {
            if (descriptor == nullptr)
            {
                return false;
            }
            bool found = false;
            for (auto& id : expected)
            {
                if (descriptor->GetUuid() == id)
                {
                    id = AZ::Uuid::CreateNull();
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return false;
            }
        }
        return true;
    }

    bool CheckShaderCompilerModule(const AZ::Module& module)
    {
        const AZ::Uuid componentId("{56B5B944-8AF4-4478-A047-8DFDE38DA681}");
        if (module.RTTI_GetType() != AZ::Uuid("{43370465-DBF1-44BB-968D-97C0B42F5EA0}") ||
            std::strcmp(module.RTTI_GetTypeName(), "AzslShaderBuilderModule") != 0 ||
            module.GetRequiredSystemComponents() != AZ::ComponentTypeList{componentId} ||
            module.GetComponentDescriptors().size() != 1)
        {
            return false;
        }
        AZ::ComponentDescriptor* descriptor = module.GetComponentDescriptors().front();
        if (descriptor == nullptr || descriptor->GetUuid() != componentId)
        {
            return false;
        }
        AZ::ComponentDescriptor::DependencyArrayType provided, incompatible, dependent, required;
        descriptor->GetProvidedServices(provided, nullptr);
        descriptor->GetIncompatibleServices(incompatible, nullptr);
        descriptor->GetDependentServices(dependent, nullptr);
        descriptor->GetRequiredServices(required, nullptr);
        if (provided != AZ::ComponentDescriptor::DependencyArrayType{AZ_CRC_CE("AzslShaderBuilderService")} ||
            incompatible != AZ::ComponentDescriptor::DependencyArrayType{AZ_CRC_CE("AzslShaderBuilderService")} ||
            dependent != AZ::ComponentDescriptor::DependencyArrayType{AZ_CRC_CE("AssetCatalogService")} ||
            !required.empty())
        {
            return false;
        }
        // Construction/destruction only: do not Init or Activate the builder component.
        std::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
        return component != nullptr && component->RTTI_GetType() == componentId;
    }

    bool CheckShaderHashImport()
    {
        AZ::RPI::ShaderOptionValuesSourceData options;
        options.emplace(AZ::Name("o_sdkProbe"), AZ::Name("enabled"));
        using ShaderHash = JE::Assets::RenderBuilders::Shader::HashedVariantInfoSourceData;
        // Both calls execute the exported out-of-line method; no local copy of the hash algorithm.
        const size_t first = ShaderHash::HashCombineShaderOptionValues(81, options);
        const size_t repeated = ShaderHash::HashCombineShaderOptionValues(81, options);
        return first == repeated;
    }
    bool CheckNativeContexts()
    {
        namespace Builders = JE::Assets::RenderBuilders;
        AZ::SceneAPI::Containers::Scene scene(AZStd::string("SdkContextProbe"));
        AZ::SceneAPI::SceneData::MeshGroup group;
        AZ::SceneAPI::CoordinateSystemConverter converter;
        Builders::MaterialAssetsByUid materials;
        AZ::Data::Asset<AZ::RPI::ModelAsset> model;
        AZ::Data::Asset<AZ::RPI::SkinMetaAsset> skin;
        AZ::Data::Asset<AZ::RPI::MorphTargetMetaAsset> morph;
        AZ::SceneAPI::Events::ExportProductList products;

        // Real imported constructors with live Scene/MeshGroup owners; no builder execution or asset loading.
        auto modelContext = std::make_unique<Builders::ModelAssetBuilderContext>(
            scene, group, converter, materials, model, skin, morph);
        auto postContext = std::make_unique<Builders::ModelAssetPostBuildContext>(
            scene, AZStd::string("SdkContextOutput"), products, group, model);
        auto materialContext = std::make_unique<Builders::MaterialAssetBuilderContext>(scene, materials);
        if (&modelContext->m_scene != &scene || &modelContext->m_group != &group ||
            &modelContext->m_materialsByUid != &materials || &modelContext->m_outputModelAsset != &model ||
            &modelContext->m_outputSkinMetaAsset != &skin || &modelContext->m_outputMorphTargetMetaAsset != &morph ||
            modelContext->m_coordSysConverter.GetConversionTransform() != converter.GetConversionTransform() ||
            &postContext->m_scene != &scene || &postContext->m_group != &group ||
            &postContext->m_productList != &products || &postContext->m_modelAsset != &model ||
            postContext->m_outputDirectory != "SdkContextOutput" ||
            &materialContext->m_scene != &scene || &materialContext->m_outputMaterialsByUid != &materials)
        {
            return false;
        }

        using Context = AZ::SceneAPI::Events::ICallContext;
        // Load each pointer through volatile storage so the RTTI calls dispatch through the real base vtable.
        Context* volatile contexts[] = {modelContext.get(), postContext.get(), materialContext.get()};
        const std::array<AZ::Uuid, 3> expectedTypes{
            AZ::Uuid("{63FEFB4B-25DC-48DD-AC72-D27DA9A6D94A}"),
            AZ::Uuid("{E0AA70B6-FA06-41E9-A137-60D7DCB85115}"),
            AZ::Uuid("{6451418A-453B-4646-A5B2-A5687FA2E97F}")};
        const std::array<const char*, 3> expectedNames{
            "ModelAssetBuilderContext", "ModelAssetPostBuildContext", "MaterialAssetBuilderContext"};
        for (size_t index = 0; index < expectedTypes.size(); ++index)
        {
            Context* context = contexts[index];
            if (context->RTTI_GetType() != expectedTypes[index] ||
                std::strcmp(context->RTTI_GetTypeName(), expectedNames[index]) != 0 ||
                !context->RTTI_IsTypeOf(azrtti_typeid<Context>()))
            {
                return false;
            }
        }

        // Transfer ownership to the actual polymorphic base, then exercise virtual destruction before owners expire.
        std::unique_ptr<Context> modelBase(std::move(modelContext));
        std::unique_ptr<Context> postBase(std::move(postContext));
        std::unique_ptr<Context> materialBase(std::move(materialContext));
        materialBase.reset();
        postBase.reset();
        modelBase.reset();
        return true;
    }

    int CheckNativeBuilders()
    {
        AZ::ComponentApplication application;
        AZ::ComponentApplication::Descriptor descriptor;
        AZ::ComponentApplication::StartupParameters startup;
        startup.m_loadStaticModules = false;
        startup.m_loadDynamicModules = false;
        startup.m_loadAssetCatalog = false;
        startup.m_loadSettingsRegistry = false;
        if (application.Create(descriptor, startup) == nullptr)
        {
            std::fputs("JE SDK: minimal component application creation failed.\n", stderr);
            return 3;
        }

        int result = 0;
        {
            std::unique_ptr<AZ::Module> module(JE::Assets::RenderBuilders::CreateToolsModule());
            if (module == nullptr || !CheckModule(*module))
            {
                std::fputs("JE SDK: native builder module identity/descriptors contract failed.\n", stderr);
                result = 4;
            }
            else if (!CheckNativeContexts())
            {
                std::fputs("JE SDK: native model exporter context ABI/reference contract failed.\n", stderr);
                result = 5;
            }
            module.reset();
            if (result == 0)
            {
                module.reset(JE::Assets::RenderBuilders::CreateToolsModule());
                if (module == nullptr || !CheckModule(*module))
                {
                    std::fputs("JE SDK: native builder module recreation failed.\n", stderr);
                    result = 6;
                }
            }
        }
        if (result == 0)
        {
            std::unique_ptr<AZ::Module> shaderModule(JE::Assets::RenderBuilders::CreateShaderCompilerToolsModule());
            if (shaderModule == nullptr || !CheckShaderCompilerModule(*shaderModule))
            {
                std::fputs("JE SDK: shader compiler module identity/descriptors/services/lifetime contract failed.\n", stderr);
                result = 7;
            }
            shaderModule.reset();
            if (result == 0)
            {
                shaderModule.reset(JE::Assets::RenderBuilders::CreateShaderCompilerToolsModule());
                if (shaderModule == nullptr || !CheckShaderCompilerModule(*shaderModule))
                {
                    std::fputs("JE SDK: shader compiler module recreation failed.\n", stderr);
                    result = 8;
                }
                else if (!CheckShaderHashImport())
                {
                    std::fputs("JE SDK: shader option hash import call was not deterministic.\n", stderr);
                    result = 9;
                }
            }
        }
        // Every native module/descriptor/context has been released while the allocator is still alive.
        application.Destroy();
        return result;
    }
}

int main()
{
    // Empty-lease import/loader probe only: no factory activation, asset load or RPI access.
    {
        JE::Editor::Materials::PassPipelinePreview preview;
        const auto restored = preview.Restore();
        const auto polled = preview.Poll();
        if (preview.IsApplied() || preview.IsCleanupPending() ||
            !restored.IsSuccess() || !polled.IsSuccess())
        {
            std::fputs("JE SDK: empty PassPipelinePreview contract failed.\n", stderr);
            return 2;
        }
    } // Calls the imported destructor before the loader probe reports success.
    using NativeModuleFactory = AZ::Module* (*)();
    NativeModuleFactory volatile factories[] = {
        &JE::Editor::CreateMaterialsModule,
        &JE::Editor::Camera::CreateAuthoringModule,
        &JE::Editor::RenderIntegration::CreateAuthoringModule
    };
    for (auto& factory : factories)
    {
        if (factory == nullptr)
        {
            return 1;
        }
    }
    const int builderResult = CheckNativeBuilders();
    if (builderResult != 0)
    {
        return builderResult;
    }
    std::puts("JE SDK: Materials, Camera and RenderIntegration exports resolved; empty PassPipelinePreview exercised.");
    std::puts("JE SDK: native RenderAssetBuilders factory/descriptors/recreation and three public contexts exercised without activation or cook.");
    std::puts("JE SDK: shader compiler factory/descriptors/services/recreation and exported shader option hash exercised without activation or cook.");
    return 0;
}
