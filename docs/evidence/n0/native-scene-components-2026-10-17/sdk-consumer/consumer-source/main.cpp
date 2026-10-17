#include <JE/Editor/Materials/MaterialsRegistration.h>
#include <JE/Editor/Materials/Graph/PassPipelinePreview.h>
#include <JE/Editor/Camera/CameraModule.h>
#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>
#include <JE/Render/Integration/RenderIntegrationModule.h>
#include <JE/Render/Scene/SceneModule.h>
#include <JE/Editor/Render/Scene/SceneModule.h>
#include <JE/Render/Bootstrap/BootstrapRequestBus.h>
#include <JE/Render/Bootstrap/BootstrapNotificationBus.h>
#include <JE/Render/Bootstrap/DefaultWindowBus.h>
#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>
#include <JE/Assets/RenderBuilders/Model/ModelExporterContexts.h>
#include <JE/Assets/RenderBuilders/Shader/HashedVariantListSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <JE/Render/RPI/Public/Image/ImageTagSystemComponent.h>
#include <JE/Render/RPI/Public/Model/ModelTagSystemComponent.h>
#include <JE/Render/RPI/Reflect/RPISystemDescriptor.h>
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
    struct SceneDescriptorIdentity
    {
        const char* m_name;
        const char* m_uuid;
    };

    // Literal independent pre-migration snapshot: runtime first36, editor all74.
    constexpr SceneDescriptorIdentity SceneOriginalDescriptors[] = {
        { "AtomLyIntegrationCommonFeaturesSystemComponent", "{E29DAE7C-BF0F-42A9-9570-037FBB689A0A}" },
        { "AZ::Render::AreaLightComponent", "{744B3961-6242-4461-983F-2817D9D29C30}" },
        { "AZ::Render::DecalComponent", "{45D0D830-85F0-47D9-B4D7-509D7D5E7A0C}" },
        { "AZ::Render::DirectionalLightComponent", "{13054592-2753-46C2-B19E-59670D4CE03D}" },
        { "AZ::Render::BloomComponent", "{0D38705D-525D-4BA7-A805-26E3E9093F54}" },
        { "AZ::Render::HDRColorGradingComponent", "{51968E0F-4DF0-4851-8405-388CBB15B573}" },
        { "AZ::Render::DisplayMapperComponent", "{FC666EDA-89B6-43C7-A6EC-0D33BE7CDFF1}" },
        { "AZ::Render::DepthOfFieldComponent", "{405F505B-D900-461F-A17D-380C294BEE39}" },
        { "AZ::Render::ExposureControlComponent", "{0238ED5E-5ABF-463D-B87B-8E23CAA56C1A}" },
        { "AZ::Render::AoComponent", "{F1203F4B-89B6-409E-AB99-B9CC87AABC2E}" },
        { "AZ::Render::LookModificationComponent", "{1D8CEFB6-3A08-4230-A6E6-32781BB3006D}" },
        { "AZ::Render::GridComponent", "{27ACB2B3-C889-4DA5-BD27-E8C45CFBCFD6}" },
        { "AZ::Render::HDRiSkyboxComponent", "{3FF41FC3-A18A-4C0A-94C4-FA353A6B661D}" },
        { "AZ::Render::ImageBasedLightComponent", "{33A1302F-A769-4D06-820A-096A4836A7E9}" },
        { "AZ::Render::MeshComponent", "{C7801FA8-3E82-4D40-B039-4854F1892FDE}" },
        { "AZ::Render::PhysicalSkyComponent", "{B5D15F12-359D-463D-8FCA-60DA97055812}" },
        { "AZ::Render::PostFxLayerComponent", "{CB98EF9F-E99E-4262-9492-4964DCD01B8B}" },
        { "AZ::Render::ReflectionProbeComponent", "{E5D29F09-F974-45FE-A1D0-2126079D1021}" },
        { "AZ::Render::SpecularReflectionsComponent", "{8B6C6CA6-2DBB-4764-B366-9F8417A63D8B}" },
        { "RayTracingDebugComponent", "{143A2F13-8BE7-4F7F-BD23-4B5B8DCD83CA}" },
        { "AZ::Render::RenderDebugComponent", "{98A72F68-3DA3-451A-BC79-707370EE4AC0}" },
        { "AZ::Render::RadiusWeightModifierComponent", "{8F0FC718-50E1-425E-A1E7-9C0425879CEB}" },
        { "AZ::Render::ShapeWeightModifierComponent", "{0BB6438B-6DD1-4A09-927F-7757D39C940D}" },
        { "AZ::Render::EntityReferenceComponent", "{0B6422F7-DD6E-447A-9A99-DEB5A325A274}" },
        { "AZ::Render::GradientWeightModifierComponent", "{4DE2AD79-85BE-49FF-9DC5-D709720B013E}" },
        { "AZ::Render::DeferredFogComponent", "{9492DC07-B3F7-4DF2-88FA-E4EEF1DD98E3}" },
        { "SurfaceDataMeshComponent", "{F8915F34-BE8B-40B4-B7E8-01EBF3DA1C95}" },
        { "AttachmentComponent", "{2D17A64A-7AC5-4C02-AC36-C5E8141FFDDF}" },
        { "AZ::Render::OcclusionCullingPlaneComponent", "{F7537387-15A8-48F0-A1F3-D19C5886B886}" },
        { "AZ::Render::ChromaticAberrationComponent", "{123FF51D-7234-429D-817B-FA89F436826B}" },
        { "AZ::Render::MotionBlurComponent", "{87B77D17-1C0D-494B-88A2-1538136BD9E0}" },
        { "AZ::Render::PaniniProjectionComponent", "{87B77D17-1C0D-494B-88A2-15CB136BD9E0}" },
        { "AZ::Render::FilmGrainComponent", "{E2F5CF7E-3D25-41E4-B3BF-C8669494F7B4}" },
        { "AZ::Render::WhiteBalanceComponent", "{DC96CC56-1850-4B8A-8E05-C0690EBEB396}" },
        { "AZ::Render::VignetteComponent", "{93C2AD53-4722-4B33-BB23-BDBC1D423289}" },
        { "AZ::Render::CubeMapCaptureComponent", "{72AEBA97-E626-4172-AB02-6588ED4B838A}" },
        { "AZ::Render::EditorAreaLightComponent", "{8B605C0C-9027-4E0B-BA8C-19E396F8F262}" },
        { "EditorCommonFeaturesSystemComponent", "{D73D77CF-D5AF-428B-909B-324E96D3DEF5}" },
        { "EditorPostFxSystemComponent", "{D86D2F88-ACDC-49B3-89D3-AE2EC5B8FEBC}" },
        { "AZ::Render::EditorDecalComponent", "{340F4C9C-89D1-45F9-9F7A-D4EE4F7FBE3E}" },
        { "AZ::Render::EditorDirectionalLightComponent", "{45B97527-6E72-411B-BC23-00068CF01580}" },
        { "AZ::Render::EditorBloomComponent", "{33789179-AB9C-4891-9DA3-1972EAED6719}" },
        { "AZ::Render::EditorHDRColorGradingComponent", "{C1FAB0B1-5847-4533-B08E-7314AC807B8E}" },
        { "AZ::Render::EditorDepthOfFieldComponent", "{E9B85017-18F3-4CD6-9EEC-221B0E6B0619}" },
        { "AZ::Render::EditorDisplayMapperComponent", "{35963748-D7EC-4F8B-B779-B5BDB7984DA3}" },
        { "AZ::Render::EditorExposureControlComponent", "{DB986527-F52D-4FAC-8BEC-B754A00CC595}" },
        { "AZ::Render::EditorAoComponent", "{5A807489-4FB2-4421-A4D2-9D9E523ABF83}" },
        { "AZ::Render::EditorLookModificationComponent", "{A38B4D55-7B7C-40F7-AB0A-31CAE9DC25E8}" },
        { "AZ::Render::EditorGridComponent", "{DF2D071A-EC31-428A-9FD0-2B8A59945417}" },
        { "AZ::Render::EditorHDRiSkyboxComponent", "{B736789D-0101-4D17-A932-B5224EEFA8B4}" },
        { "AZ::Render::EditorImageBasedLightComponent", "{6202F16C-DDF9-4026-9479-F5BDC621D372}" },
        { "EditorMeshSystemComponent", "{4D332E3D-C4FC-410B-A915-8E234CBDD4EC}" },
        { "AZ::Render::EditorMeshComponent", "{DCE68F6E-2E16-4CB4-A834-B6C2F900A7E9}" },
        { "AZ::Render::EditorPhysicalSkyComponent", "{A249BA4E-BBD5-45A1-86AA-FCA4B2C4CF9B}" },
        { "AZ::Render::EditorPostFxLayerComponent", "{4DE50024-068D-4656-862B-6B51D38C3273}" },
        { "AZ::Render::EditorReflectionProbeComponent", "{6EBF2E41-2918-48B8-ACC3-FB115ED09E64}" },
        { "AZ::Render::EditorSpecularReflectionsComponent", "{288B89FC-4179-49FE-A2F1-95B47183F636}" },
        { "RayTracingDebugEditorComponent", "{352A6033-4127-4A8D-BE8A-3FA1267B02EB}" },
        { "AZ::Render::RenderDebugEditorComponent", "{235031F8-2AAD-442D-AB4D-F9B5B8337DCD}" },
        { "AZ::Render::EditorRadiusWeightModifierComponent", "{AFB4C267-2D2D-463A-94F7-A839353FF20E}" },
        { "AZ::Render::EditorShapeWeightModifierComponent", "{7C528F7A-5EE7-4540-BA52-0507C17CE67D}" },
        { "AZ::Render::EditorEntityReferenceComponent", "{71C5AD29-2B0C-4264-BCBF-61E0A4E98D32}" },
        { "AZ::Render::EditorGradientWeightModifierComponent", "{ABD08A3E-1A35-4BE2-AA57-092FCCFC9CA7}" },
        { "AZ::Render::EditorDeferredFogComponent", "{6459274F-54C8-4C22-9448-B2B13B69182C}" },
        { "EditorSurfaceDataMeshComponent", "{4D73E979-5463-4B75-AE46-70B1E52CBF43}" },
        { "EditorAttachmentComponent", "{DA6072FD-E696-47D8-81D9-1F77D3464200}" },
        { "AZ::Render::EditorOcclusionCullingPlaneComponent", "{BE7CC17B-32EB-49B0-BAD9-D26E3A059012}" },
        { "AZ::Render::EditorChromaticAberrationComponent", "{2F6115F3-C6C4-47ED-8F06-11006F190A68}" },
        { "AZ::Render::EditorMotionBlurComponent", "{ACA14BD9-BB53-4FEB-9E07-FDC0C9FE64FE}" },
        { "AZ::Render::EditorPaniniProjectionComponent", "{213CFFC8-E9E6-46EA-9DBE-B779F0B2A823}" },
        { "AZ::Render::EditorFilmGrainComponent", "{61D39B81-DE19-482B-97FF-3761F2C25E4D}" },
        { "AZ::Render::EditorWhiteBalanceComponent", "{32C7C44E-AABD-4575-A485-C820BF1F0873}" },
        { "AZ::Render::EditorVignetteComponent", "{8E362EA9-76D8-4EBC-B73D-400DF3DF8B4A}" },
        { "AZ::Render::EditorCubeMapCaptureComponent", "{F32903D7-6463-4A9B-843A-F83919E3288C}" },
    };

    bool CheckSceneModule(AZ::Module* (*factory)(), bool editor)
    {
        const AZ::ComponentTypeList required = editor
            ? AZ::ComponentTypeList{ AZ::Uuid("{E29DAE7C-BF0F-42A9-9570-037FBB689A0A}"), AZ::Uuid("{4D332E3D-C4FC-410B-A915-8E234CBDD4EC}"), AZ::Uuid("{D73D77CF-D5AF-428B-909B-324E96D3DEF5}"), AZ::Uuid("{D86D2F88-ACDC-49B3-89D3-AE2EC5B8FEBC}") }
            : AZ::ComponentTypeList{ AZ::Uuid("{E29DAE7C-BF0F-42A9-9570-037FBB689A0A}") };
        const size_t count = editor ? 74 : 36;
        for (int cycle = 0; cycle != 2; ++cycle)
        {
            std::unique_ptr<AZ::Module> module(factory());
            if (!module || module->RTTI_GetType() != AZ::Uuid("{E6FF4862-9355-4B23-AE00-B936F0C6E6C9}") ||
                std::strcmp(module->RTTI_GetTypeName(), "AtomLyIntegrationCommonFeaturesModule") != 0 ||
                module->GetComponentDescriptors().size() != count || module->GetRequiredSystemComponents() != required)
            {
                return false;
            }
            size_t index = 0;
            std::array<AZ::Uuid, 74> seen{};
            for (AZ::ComponentDescriptor* const descriptor : module->GetComponentDescriptors())
            {
                if (!descriptor || descriptor->GetUuid() != AZ::Uuid(SceneOriginalDescriptors[index].m_uuid) ||
                    std::strcmp(descriptor->GetName(), SceneOriginalDescriptors[index].m_name) != 0)
                {
                    return false;
                }
                for (size_t previous = 0; previous != index; ++previous)
                {
                    if (seen[previous] == descriptor->GetUuid())
                    {
                        return false;
                    }
                }
                seen[index] = descriptor->GetUuid();
                if (index == 0)
                {
                    AZ::ComponentDescriptor::DependencyArrayType provided;
                    AZ::ComponentDescriptor::DependencyArrayType dependencies;
                    descriptor->GetProvidedServices(provided, nullptr);
                    descriptor->GetRequiredServices(dependencies, nullptr);
                    if (provided.size() != 1 || provided.front() != AZ_CRC_CE("LyIntegrationCommonFeaturesService") ||
                        dependencies.size() != 1 || dependencies.front() != AZ_CRC_CE("CommonService"))
                    {
                        return false;
                    }
                }
                if (descriptor->GetUuid() == AZ::Uuid("{13054592-2753-46C2-B19E-59670D4CE03D}"))
                {
                    std::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
                    if (!component || !component->RTTI_IsTypeOf(azrtti_typeid<AZ::Component>()) ||
                        component->RTTI_AddressOf(azrtti_typeid<AZ::Component>()) != component.get())
                    {
                        return false;
                    }
                }
                ++index;
            }
        } // Descriptors are destroyed before the next cycle, while the allocator lives.
        return true;
    }

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
        JE::Render::RPI::ShaderOptionValuesSourceData options;
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
        AZ::Data::Asset<JE::Render::RPI::ModelAsset> model;
        AZ::Data::Asset<JE::Render::RPI::SkinMetaAsset> skin;
        AZ::Data::Asset<JE::Render::RPI::MorphTargetMetaAsset> morph;
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
        using Registration = AZ::Interface<JE::Render::RPI::MaterialFunctorSourceDataRegistration>;
        if (Registration::Get() != nullptr)
        {
            return false;
        }
        const AZ::Uuid componentId("{83E301F3-7A0C-4099-B530-9342B91B1BC0}");
        const AZ::ComponentTypeList required{
            componentId, azrtti_typeid<JE::Render::RPI::ImageTagSystemComponent>(),
            azrtti_typeid<JE::Render::RPI::ModelTagSystemComponent>(), AZ::Uuid("{75FEC6CC-ACA7-419C-8A63-4286998CBC0B}")};
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
                if (field.m_nameCrc == AZ_CRC_CE("RpiDescriptor") && field.m_typeId == azrtti_typeid<JE::Render::RPI::RPISystemDescriptor>())
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
        if (result == 0 &&
            (!CheckSceneModule(&JE::Render::Scene::CreateModule, false) ||
                !CheckSceneModule(&JE::Editor::Render::Scene::CreateModule, true)))
        {
            std::fputs("JE SDK: native runtime/editor Scene factory identity/descriptors/services/lifetime contract failed.\n", stderr);
            result = 14;
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
    std::puts("JE SDK: native runtime/editor Scene factories,36/74 descriptor identities,1/4 required components,services and two construction cycles exercised without activation.");
    return 0;
}
