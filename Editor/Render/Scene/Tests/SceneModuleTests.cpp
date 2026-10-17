// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT

#include <AzTest/AzTest.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <JE/Render/Scene/SceneModule.h>
#include <JE/Editor/Render/Scene/SceneModule.h>

namespace UnitTest
{
    namespace
    {
        struct DescriptorIdentity
        {
            const char* m_name;
            const char* m_uuid;
        };

        // Pinned from the original module and component declarations, independently
        // of the native factories. Runtime is the first 36 entries, Editor all 74.
        constexpr DescriptorIdentity OriginalDescriptors[] = {
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

        void VerifySceneModule(AZ::Module* (*factory)(), size_t count, const AZ::ComponentTypeList& required)
        {
            for (int cycle = 0; cycle != 2; ++cycle)
            {
                for (size_t index = 0; index < count; ++index)
                {
                    AZ::ComponentDescriptor* existing = nullptr;
                    AZ::ComponentDescriptorBus::EventResult(existing, AZ::Uuid(OriginalDescriptors[index].m_uuid),
                        &AZ::ComponentDescriptor::GetDescriptor);
                    ASSERT_EQ(existing, nullptr) << OriginalDescriptors[index].m_name;
                }
                AZStd::unique_ptr<AZ::Module> module(factory());
                ASSERT_NE(module, nullptr);
                EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{E6FF4862-9355-4B23-AE00-B936F0C6E6C9}"));
                EXPECT_EQ(AZStd::string_view(module->RTTI_GetTypeName()), "AtomLyIntegrationCommonFeaturesModule");
                const auto descriptors = module->GetComponentDescriptors();
                ASSERT_EQ(descriptors.size(), count);
                AZStd::unordered_set<AZ::Uuid> uniqueTypes;
                size_t index = 0;
                for (auto* descriptor : descriptors)
                {
                    ASSERT_NE(descriptor, nullptr);
                    EXPECT_EQ(descriptor->GetUuid(), AZ::Uuid(OriginalDescriptors[index].m_uuid));
                    EXPECT_EQ(AZStd::string_view(descriptor->GetName()), OriginalDescriptors[index].m_name);
                    EXPECT_TRUE(uniqueTypes.insert(descriptor->GetUuid()).second);
                    if (descriptor->GetUuid() == AZ::Uuid("{13054592-2753-46C2-B19E-59670D4CE03D}"))
                    {
                        AZStd::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
                        ASSERT_NE(component, nullptr);
                        EXPECT_TRUE(component->RTTI_IsTypeOf(azrtti_typeid<AZ::Component>()));
                        EXPECT_EQ(component->RTTI_AddressOf(azrtti_typeid<AZ::Component>()), static_cast<void*>(component.get()));
                    }
                    AZ::ComponentDescriptor::DependencyArrayType provided;
                    descriptor->GetProvidedServices(provided, nullptr);
                    if (index == 0)
                    {
                        ASSERT_EQ(provided.size(), 1);
                        EXPECT_EQ(provided.front(), AZ_CRC_CE("LyIntegrationCommonFeaturesService"));
                        AZ::ComponentDescriptor::DependencyArrayType dependencies;
                        descriptor->GetRequiredServices(dependencies, nullptr);
                        ASSERT_EQ(dependencies.size(), 1);
                        EXPECT_EQ(dependencies.front(), AZ_CRC_CE("CommonService"));
                    }
                    ++index;
                }
                EXPECT_EQ(module->GetRequiredSystemComponents(), required);
                // Release this cycle before checking ownership and recreating the module.
                // No component activation, renderer or window is involved.
                module.reset();
                for (size_t releasedIndex = 0; releasedIndex < count; ++releasedIndex)
                {
                    AZ::ComponentDescriptor* existing = nullptr;
                    AZ::ComponentDescriptorBus::EventResult(existing, AZ::Uuid(OriginalDescriptors[releasedIndex].m_uuid),
                        &AZ::ComponentDescriptor::GetDescriptor);
                    ASSERT_EQ(existing, nullptr) << OriginalDescriptors[releasedIndex].m_name;
                }
            }
        }
    }

    TEST(NativeSceneModuleContract, RuntimePreservesOriginalDescriptorsRequiredServicesAndRecreation)
    {
        VerifySceneModule(&JE::Render::Scene::CreateModule, 36,
            { AZ::Uuid("{E29DAE7C-BF0F-42A9-9570-037FBB689A0A}") });
    }

    TEST(NativeSceneModuleContract, EditorPreservesOriginalDescriptorsRequiredServicesAndRecreation)
    {
        VerifySceneModule(&JE::Editor::Render::Scene::CreateModule, 74,
            { AZ::Uuid("{E29DAE7C-BF0F-42A9-9570-037FBB689A0A}"), AZ::Uuid("{4D332E3D-C4FC-410B-A915-8E234CBDD4EC}"), AZ::Uuid("{D73D77CF-D5AF-428B-909B-324E96D3DEF5}"), AZ::Uuid("{D86D2F88-ACDC-49B3-89D3-AE2EC5B8FEBC}") });
    }
}
