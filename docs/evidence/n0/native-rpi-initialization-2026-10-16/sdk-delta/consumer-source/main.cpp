#include <JE/Editor/Materials/MaterialsRegistration.h>
#include <JE/Editor/Materials/Graph/PassPipelinePreview.h>
#include <JE/Editor/Camera/CameraModule.h>
#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>
#include <JE/Render/Integration/RenderIntegrationModule.h>
#include <JE/Render/Bootstrap/BootstrapRequestBus.h>
#include <JE/Render/Bootstrap/BootstrapNotificationBus.h>
#include <JE/Render/Bootstrap/DefaultWindowBus.h>
#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <JE/Assets/RenderBuilders/Model/ModelExporterContexts.h>
#include <JE/Assets/RenderBuilders/Shader/HashedVariantListSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <Atom/RPI.Public/Image/ImageTagSystemComponent.h>
#include <Atom/RPI.Public/Model/ModelTagSystemComponent.h>
#include <Atom/RPI.Reflect/RPISystemDescriptor.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
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

    bool CheckBootstrapModule(const AZ::Module& module, bool requireRendererSystem)
    {
        const AZ::Uuid componentId("{1EAFD87D-A64A-4612-93D8-B3AFFA70F09B}");
        const AZ::ComponentTypeList expectedRequired = requireRendererSystem
            ? AZ::ComponentTypeList{componentId} : AZ::ComponentTypeList{};
        if (module.RTTI_GetType() != AZ::Uuid("{ADDE20F4-03E6-4692-A736-E56B87952727}") ||
            std::strcmp(module.RTTI_GetTypeName(), "BootstrapModule") != 0 ||
            module.GetRequiredSystemComponents() != expectedRequired ||
            module.GetComponentDescriptors().size() != 1)
        {
            return false;
        }
        AZ::ComponentDescriptor* descriptor = module.GetComponentDescriptors().front();
        if (descriptor == nullptr || descriptor->GetUuid() != componentId)
        {
            return false;
        }
        AZ::ComponentDescriptor::DependencyArrayType provided, incompatible, required, dependent;
        descriptor->GetProvidedServices(provided, nullptr);
        descriptor->GetIncompatibleServices(incompatible, nullptr);
        descriptor->GetRequiredServices(required, nullptr);
        descriptor->GetDependentServices(dependent, nullptr);
        const AZ::ComponentDescriptor::DependencyArrayType bootstrapService{AZ_CRC_CE("BootstrapSystemComponent")};
        const AZ::ComponentDescriptor::DependencyArrayType requiredServices{
            AZ_CRC_CE("RPISystem"), AZ_CRC_CE("SceneSystemComponentService")};
        const AZ::ComponentDescriptor::DependencyArrayType dependentServices{
            AZ_CRC_CE("ImGuiSystemComponent"), AZ_CRC_CE("PrimitiveSystemComponent"), AZ_CRC_CE("MeshSystemComponent"),
            AZ_CRC_CE("CoreLightsService"), AZ_CRC_CE("DynamicDrawService"), AZ_CRC_CE("CommonService"), AZ_CRC_CE("HairService")};
        if (provided != bootstrapService || incompatible != bootstrapService ||
            required != requiredServices || dependent != dependentServices)
        {
            return false;
        }
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        descriptor->Reflect(&context);
        const auto* reflected = context.FindClassData(componentId);
        if (reflected == nullptr || reflected->m_name == nullptr || std::strcmp(reflected->m_name, "BootstrapSystemComponent") != 0 ||
            reflected->m_version != 1)
        {
            return false;
        }
        // Construction/destruction only: neither mode initializes or activates a renderer or window.
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

    bool CheckRpiModule(AZ::Module* (*factory)(), bool editor)
    {
        using Registration = AZ::Interface<AZ::RPI::MaterialFunctorSourceDataRegistration>;
        if (Registration::Get() != nullptr)
        {
            return false;
        }
        const AZ::Uuid componentId("{83E301F3-7A0C-4099-B530-9342B91B1BC0}");
        const AZ::ComponentTypeList required{
            componentId, azrtti_typeid<AZ::RPI::ImageTagSystemComponent>(),
            azrtti_typeid<AZ::RPI::ModelTagSystemComponent>(), AZ::Uuid("{75FEC6CC-ACA7-419C-8A63-4286998CBC0B}")};
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            std::unique_ptr<AZ::Module> module(factory());
            const AZ::Uuid moduleId(editor ? "{F2DF5DD9-1323-436C-B0E5-B200B8709CE5}" : "{CDB54E96-717D-4DFC-BEA6-F809BDE601AE}");
            if (module == nullptr || module->RTTI_GetType() != moduleId ||
                module->GetRequiredSystemComponents() != required || module->GetComponentDescriptors().size() != required.size())
            {
                return false;
            }
            AZ::ComponentDescriptor* rpiDescriptor = nullptr;
            for (const auto& id : required)
            {
                size_t matches = 0;
                for (auto* descriptor : module->GetComponentDescriptors())
                {
                    if (descriptor == nullptr)
                    {
                        return false;
                    }
                    if (descriptor->GetUuid() == id)
                    {
                        ++matches;
                        if (id == componentId)
                        {
                            rpiDescriptor = descriptor;
                        }
                    }
                }
                if (matches != 1)
                {
                    return false;
                }
            }
            if (rpiDescriptor == nullptr)
            {
                return false;
            }
            AZ::SerializeContext context;
            AZ::Entity::Reflect(&context);
            rpiDescriptor->Reflect(&context);
            const auto* reflected = context.FindClassData(componentId);
            if (reflected == nullptr || reflected->m_version != 0)
            {
                return false;
            }
            bool hasDescriptor = false;
            for (const auto& field : reflected->m_elements)
            {
                if (field.m_nameCrc == AZ_CRC_CE("RpiDescriptor") && field.m_typeId == azrtti_typeid<AZ::RPI::RPISystemDescriptor>())
                {
                    hasDescriptor = true;
                }
            }
            if (!hasDescriptor)
            {
                return false;
            }
            std::unique_ptr<AZ::Component> component(rpiDescriptor->CreateComponent());
            if (component == nullptr || component->RTTI_GetType() != componentId || (Registration::Get() != nullptr) != editor)
            {
                return false;
            }
            component.reset();
            if (Registration::Get() != nullptr)
            {
                return false;
            }
        }
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
        if (result == 0)
        {
            const bool hadRequests = JE::Render::Bootstrap::RequestBus::HasHandlers();
            const bool hadWindow = JE::Render::Bootstrap::DefaultWindowBus::HasHandlers();
            for (const bool requireRendererSystem : std::array<bool, 2>{true, false})
            {
                {
                    std::unique_ptr<AZ::Module> bootstrapModule(requireRendererSystem
                        ? JE::Render::Integration::CreateBootstrapModule()
                        : JE::Render::Integration::CreateBootstrapModule(false));
                    if (bootstrapModule == nullptr || !CheckBootstrapModule(*bootstrapModule, requireRendererSystem))
                    {
                        std::fputs("JE SDK: Bootstrap module identity/descriptors/services/reflection/lifetime contract failed.\n", stderr);
                        result = 10;
                    }
                    bootstrapModule.reset();
                    if (result == 0)
                    {
                        bootstrapModule.reset(JE::Render::Integration::CreateBootstrapModule(requireRendererSystem));
                        if (bootstrapModule == nullptr || !CheckBootstrapModule(*bootstrapModule, requireRendererSystem))
                        {
                            std::fputs("JE SDK: Bootstrap module recreation failed.\n", stderr);
                            result = 11;
                        }
                    }
                }
                if (JE::Render::Bootstrap::RequestBus::HasHandlers() != hadRequests ||
                    JE::Render::Bootstrap::DefaultWindowBus::HasHandlers() != hadWindow)
                {
                    std::fputs("JE SDK: unactivated Bootstrap factory changed request/window bus handlers.\n", stderr);
                    result = 12;
                }
                if (result != 0)
                {
                    break;
                }
            }
        }
        if (result == 0 &&
            (!CheckRpiModule(&JE::Render::Integration::CreateRpiRuntimeModule, false) ||
                !CheckRpiModule(&JE::Editor::RenderIntegration::CreateRpiEditorModule, true)))
        {
            std::fputs("JE SDK: native runtime/editor RPI factory registration/lifetime contract failed.\n", stderr);
            result = 13;
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
    std::puts("JE SDK: native Bootstrap true/false factory/descriptors/services/reflection/recreation and stable buses exercised without activation or window.");
    std::puts("JE SDK: native runtime/editor RPI factory identities/four required descriptors/reflection and two construction cycles exercised without activation.");
    return 0;
}
