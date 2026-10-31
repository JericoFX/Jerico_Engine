/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/RenderIntegration/RenderIntegrationModule.h>
#include <JE/Editor/RenderIntegration/AssetProcessingModule.h>
#include <Atom/RHI/Factory.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/Path/Path.h>
#include <Tests/AssetSystemMocks.h>
#include <AzCore/PlatformIncl.h>
#include <JE/Render/RPI/Reflect/RPISystemDescriptor.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/Entity.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <JE/Render/RPI/Public/Image/ImageTagSystemComponent.h>
#include <JE/Render/RPI/Public/Model/ModelTagSystemComponent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzTest/AzTest.h>

namespace JE::Editor::RenderIntegration
{
    // Clean reflected generic types while the test allocator owner is still alive.
    class EditorRenderIntegration : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_ownsNames = !AZ::NameDictionary::IsReady();
            if (m_ownsNames)
            {
                AZ::NameDictionary::Create();
            }
        }

        void TearDown() override
        {
            if (m_ownsNames)
            {
                AZ::NameDictionary::Destroy();
            }
            LeakDetectionFixture::TearDown();
        }

        bool m_ownsNames{};
    };

    namespace
    {
        // Serial CPU fixture: shadow the global registry without writing/deleting the prior owner's data.
        class ScopedMaterialTypePathServices final
        {
        public:
            ScopedMaterialTypePathServices()
                : m_previousRegistry(AZ::SettingsRegistry::Get())
            {
                if (m_previousRegistry)
                {
                    AZ::SettingsRegistry::Unregister(m_previousRegistry);
                }
                AZ::SettingsRegistry::Register(&m_registry);
                m_assetSystem.BusConnect();
                ON_CALL(m_assetSystem, GetSourceInfoBySourcePath(::testing::_, ::testing::_, ::testing::_))
                    .WillByDefault([this](const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder)
                    {
                        m_lastSourcePath = sourcePath ? sourcePath : "";
                        if (!m_sourceFound)
                        {
                            return false;
                        }
                        assetInfo.m_relativePath = m_relativePath;
                        watchFolder = "F:/Installed/Materials/Assets";
                        return true;
                    });
            }

            ~ScopedMaterialTypePathServices()
            {
                m_assetSystem.BusDisconnect();
                AZ::SettingsRegistry::Unregister(&m_registry);
                if (m_previousRegistry)
                {
                    AZ::SettingsRegistry::Register(m_previousRegistry);
                }
            }

            AZ::SettingsRegistryImpl m_registry;
            ::testing::NiceMock<UnitTests::MockAssetSystemRequest> m_assetSystem;
            AZStd::string m_relativePath = "Materials/Types/StandardPBR.materialtype";
            AZStd::string m_lastSourcePath;
            bool m_sourceFound = true;

        private:
            AZ::SettingsRegistryInterface* m_previousRegistry{};
        };
    }

    TEST_F(EditorRenderIntegration, IntermediateMaterialTypePathHonorsConfiguredProjectCache)
    {
        ASSERT_FALSE(AzToolsFramework::AssetSystemRequestBus::HasHandlers());
        auto* previousRegistry = AZ::SettingsRegistry::Get();
        {
            ScopedMaterialTypePathServices services;
            ASSERT_TRUE(services.m_registry.Set(
                AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, "F:/Validation/Project"));
            ASSERT_TRUE(services.m_registry.Set(
                AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder, "F:/Validation/Mixed Cache/r6"));
            const char* relativePaths[]{
                "Materials/Types/StandardPBR.materialtype",
                "MaterialCanvas/GraphData/Nodes/MaterialOutputs/BasePBR/MaterialGraphName.materialtype",
                "MaterialCanvas/GraphData/Nodes/MaterialOutputs/StandardPBR/MaterialGraphName.materialtype" };
            const char* expectedPaths[]{
                "F:/Validation/Mixed Cache/r6/Intermediate Assets/materials/types/standardpbr_generated.materialtype",
                "F:/Validation/Mixed Cache/r6/Intermediate Assets/materialcanvas/graphdata/nodes/materialoutputs/basepbr/materialgraphname_generated.materialtype",
                "F:/Validation/Mixed Cache/r6/Intermediate Assets/materialcanvas/graphdata/nodes/materialoutputs/standardpbr/materialgraphname_generated.materialtype" };
            for (size_t index = 0; index < AZ_ARRAY_SIZE(relativePaths); ++index)
            {
                SCOPED_TRACE(index);
                services.m_relativePath = relativePaths[index];
                const AZStd::string sourcePath = "F:/Installed/Materials/Assets/" + services.m_relativePath;
                const auto actual = JE::Render::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath(sourcePath);
                const auto expected = AZ::IO::Path(expectedPaths[index]).LexicallyNormal().String();
                EXPECT_STREQ(actual.c_str(), expected.c_str());
                EXPECT_STREQ(services.m_lastSourcePath.c_str(), sourcePath.c_str());
            }
        }
        EXPECT_EQ(AZ::SettingsRegistry::Get(), previousRegistry);
        EXPECT_FALSE(AzToolsFramework::AssetSystemRequestBus::HasHandlers());
    }

    TEST_F(EditorRenderIntegration, IntermediateMaterialTypePathKeepsDefaultCacheAndMissingSourceBehavior)
    {
        ASSERT_FALSE(AzToolsFramework::AssetSystemRequestBus::HasHandlers());
        ScopedMaterialTypePathServices services;
        ASSERT_TRUE(services.m_registry.Set(
            AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, "F:/Validation/Project"));
        ASSERT_TRUE(services.m_registry.Set(
            AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder, "F:/Validation/Project/Cache"));
        const auto actual = JE::Render::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath("StandardPBR.materialtype");
        const auto expected = AZ::IO::Path(
            "F:/Validation/Project/Cache/Intermediate Assets/materials/types/standardpbr_generated.materialtype").LexicallyNormal().String();
        EXPECT_STREQ(actual.c_str(), expected.c_str());
        services.m_sourceFound = false;
        EXPECT_TRUE(JE::Render::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath("missing.materialtype").empty());
    }

    TEST_F(EditorRenderIntegration, IntermediateMaterialTypePathDoesNotInventMissingOrEmptyCacheRoot)
    {
        ASSERT_FALSE(AzToolsFramework::AssetSystemRequestBus::HasHandlers());
        ScopedMaterialTypePathServices services;
        ASSERT_TRUE(services.m_registry.Set(
            AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, "F:/Validation/Project"));
        EXPECT_TRUE(JE::Render::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath("StandardPBR.materialtype").empty());
        ASSERT_TRUE(services.m_registry.Set(
            AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder, ""));
        EXPECT_TRUE(JE::Render::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath("StandardPBR.materialtype").empty());
    }
    TEST_F(EditorRenderIntegration, AuthoringFactoryPreservesRuntimeDescriptorsExactlyOnceAndEditorSchema)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateAuthoringModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{7B330394-BE9C-4BDA-9345-1A0859815982}"));
        AZStd::unordered_set<AZ::Uuid> expected{
            AZ::Uuid("{7AE0D6AD-691C-41B6-9DD5-F23F78B1A02E}"),
            AZ::Uuid("{FFB99CE4-2C9E-476D-8140-50A8A696E242}"),
            AZ::Uuid("{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}") };
        const auto descriptors = module->GetComponentDescriptors();
        ASSERT_EQ(descriptors.size(), 3);
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        AzToolsFramework::Components::EditorComponentBase::Reflect(&context);
        for (auto* descriptor : descriptors)
        {
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
            descriptor->Reflect(&context);
        }
        EXPECT_TRUE(expected.empty());
        const auto required = module->GetRequiredSystemComponents();
        ASSERT_EQ(required.size(), 1);
        EXPECT_EQ(required.front(), AZ::Uuid("{FFB99CE4-2C9E-476D-8140-50A8A696E242}"));
        const auto* editor = context.FindClassData(AZ::Uuid("{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}"));
        ASSERT_NE(editor, nullptr);
        EXPECT_STREQ(editor->m_name, "AssetCollectionAsyncLoaderTestComponent");
        EXPECT_EQ(editor->m_version, 1);
        bool hasAssetList = false;
        for (const auto& element : editor->m_elements)
        {
            if (element.m_nameCrc == AZ_CRC_CE("AssetListPathJson"))
            {
                hasAssetList = true;
                EXPECT_EQ(element.m_typeId, azrtti_typeid<AZStd::string>());
            }
        }
        EXPECT_TRUE(hasAssetList);
    }

    TEST_F(EditorRenderIntegration, AssetProcessingFactoryPreservesDescriptorsAndSchemaWithoutRendererActivation)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateAssetProcessingModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{7B330394-BE9C-4BDA-9345-1A0859815982}"));
        EXPECT_TRUE(module->GetRequiredSystemComponents().empty());
        AZStd::unordered_set<AZ::Uuid> expected{
            AZ::Uuid("{7AE0D6AD-691C-41B6-9DD5-F23F78B1A02E}"),
            AZ::Uuid("{FFB99CE4-2C9E-476D-8140-50A8A696E242}"),
            AZ::Uuid("{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}") };
        const auto descriptors = module->GetComponentDescriptors();
        ASSERT_EQ(descriptors.size(), 3);
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        AzToolsFramework::Components::EditorComponentBase::Reflect(&context);
        for (auto* descriptor : descriptors)
        {
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
            descriptor->Reflect(&context);
            EXPECT_NE(context.FindClassData(descriptor->GetUuid()), nullptr);
        }
        EXPECT_TRUE(expected.empty());
        const auto* editor = context.FindClassData(AZ::Uuid("{D0A558AD-F8CD-4DB8-80A4-40B4E1F947FA}"));
        ASSERT_NE(editor, nullptr);
        EXPECT_STREQ(editor->m_name, "AssetCollectionAsyncLoaderTestComponent");
        EXPECT_EQ(editor->m_version, 1);
        bool hasAssetList = false;
        for (const auto& element : editor->m_elements)
        {
            if (element.m_nameCrc == AZ_CRC_CE("AssetListPathJson"))
            {
                hasAssetList = true;
                EXPECT_EQ(element.m_typeId, azrtti_typeid<AZStd::string>());
            }
        }
        EXPECT_TRUE(hasAssetList);
    }
#if defined(AZ_PLATFORM_WINDOWS)
    TEST_F(EditorRenderIntegration, RpiAuthoringBodyResolvesFromNativeEditorRenderIntegration)
    {
        HMODULE provider = nullptr;
        ASSERT_TRUE(::GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&JE::Render::RPI::MaterialTypeSourceData::Reflect), &provider));
        const HMODULE expected = ::GetModuleHandleW(L"EditorRenderIntegration.dll");
        ASSERT_NE(expected, nullptr);
        EXPECT_EQ(provider, expected) << "The real RPI implementation still lives in its legacy DLL";
    }
    TEST_F(EditorRenderIntegration, NativeRpiEditorFactoryPreservesRegistrationAndLifecycle)
    {
        // Probe the native Windows C++ export without linking an as-yet absent factory.
        auto library = AZ::DynamicModuleHandle::Create("EditorRenderIntegration");
        ASSERT_NE(library, nullptr);
        ASSERT_TRUE(library->Load(AZ::DynamicModuleHandle::LoadFlags::NoLoad));
        using Factory = AZ::Module* (*)();
        const auto factory = library->GetFunction<Factory>("?CreateRpiEditorModule@RenderIntegration@Editor@JE@@YAPEAVModule@AZ@@XZ");
        ASSERT_NE(factory, nullptr) << "Native RPI factory is missing from EditorRenderIntegration";
        using FunctorRegistration = AZ::Interface<JE::Render::RPI::MaterialFunctorSourceDataRegistration>;
        ASSERT_EQ(FunctorRegistration::Get(), nullptr);
        const AZ::Uuid componentId("{83E301F3-7A0C-4099-B530-9342B91B1BC0}");
        const AZ::ComponentTypeList required{
            componentId, azrtti_typeid<JE::Render::RPI::ImageTagSystemComponent>(),
            azrtti_typeid<JE::Render::RPI::ModelTagSystemComponent>(), AZ::Uuid("{75FEC6CC-ACA7-419C-8A63-4286998CBC0B}") };
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            AZStd::unique_ptr<AZ::Module> module(factory());
            ASSERT_NE(module, nullptr);
            EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{F2DF5DD9-1323-436C-B0E5-B200B8709CE5}"));
            EXPECT_EQ(module->GetRequiredSystemComponents(), required);
            ASSERT_EQ(module->GetComponentDescriptors().size(), 4);
            AZStd::unordered_set<AZ::Uuid> expected(required.begin(), required.end());
            AZ::ComponentDescriptor* rpiDescriptor = nullptr;
            for (auto* descriptor : module->GetComponentDescriptors())
            {
                ASSERT_NE(descriptor, nullptr);
                EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
                if (descriptor->GetUuid() == componentId)
                {
                    rpiDescriptor = descriptor;
                }
            }
            EXPECT_TRUE(expected.empty());
            ASSERT_NE(rpiDescriptor, nullptr);
            AZStd::unique_ptr<AZ::Component> component(rpiDescriptor->CreateComponent());
            ASSERT_NE(component, nullptr);
            EXPECT_EQ(component->RTTI_GetType(), componentId);
            EXPECT_EQ(FunctorRegistration::Get() != nullptr, true);
            component.reset();
            EXPECT_EQ(FunctorRegistration::Get(), nullptr);
        }
    }

    TEST_F(EditorRenderIntegration, NativeRpiDescriptorRoundTripPreservesDefaultsAndPartialRegistryOverrides)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateRpiEditorModule());
        ASSERT_NE(module, nullptr);
        const AZ::Uuid componentId("{83E301F3-7A0C-4099-B530-9342B91B1BC0}");
        AZ::ComponentDescriptor* descriptor = nullptr;
        for (auto* entry : module->GetComponentDescriptors())
        {
            if (entry->GetUuid() == componentId)
            {
                descriptor = entry;
            }
        }
        ASSERT_NE(descriptor, nullptr);
        AZ::ComponentDescriptor::DependencyArrayType provided, required, dependent;
        descriptor->GetProvidedServices(provided, nullptr);
        descriptor->GetRequiredServices(required, nullptr);
        descriptor->GetDependentServices(dependent, nullptr);
        const AZ::ComponentDescriptor::DependencyArrayType expectedProvided{ AZ_CRC_CE("RPISystem") };
        const AZ::ComponentDescriptor::DependencyArrayType expectedRequired{ AZ::RHI::Factory::GetComponentService() };
        const AZ::ComponentDescriptor::DependencyArrayType expectedDependent{ AZ_CRC_CE("XRSystemService") };
        EXPECT_EQ(provided, expectedProvided);
        EXPECT_EQ(required, expectedRequired);
        EXPECT_EQ(dependent, expectedDependent);
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        descriptor->Reflect(&context);
        const auto* reflected = context.FindClassData(componentId);
        ASSERT_NE(reflected, nullptr);
        EXPECT_EQ(reflected->m_version, 0);
        bool hasDescriptorField = false;
        for (const auto& field : reflected->m_elements)
        {
            if (field.m_nameCrc == AZ_CRC_CE("RpiDescriptor"))
            {
                hasDescriptorField = true;
                EXPECT_EQ(field.m_typeId, azrtti_typeid<JE::Render::RPI::RPISystemDescriptor>());
            }
        }
        ASSERT_TRUE(hasDescriptorField);
        AZStd::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
        ASSERT_NE(component, nullptr);
        AZStd::vector<char> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(
            stream, AZ::ObjectStream::ST_XML, component->RTTI_AddressOf(componentId), componentId, &context));
        const AZStd::string xml(bytes.begin(), bytes.end());
        EXPECT_NE(xml.find("field=\"RpiDescriptor\""), AZStd::string::npos);
        component.reset(); // Editor functor registration must be released before reconstructing it.
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZStd::unique_ptr<AZ::Component> reopened(AZ::Utils::LoadObjectFromStream<AZ::Component>(stream, &context));
        ASSERT_NE(reopened, nullptr);
        EXPECT_EQ(reopened->RTTI_GetType(), componentId);

        // Read the real reflected member, rather than reproducing its constructor defaults in the test.
        JE::Render::RPI::RPISystemDescriptor nativeDescriptor;
        unsigned int descriptorCount = 0;
        AZ::SerializeContext::EnumerateInstanceCallContext enumeration(
            [&nativeDescriptor, &descriptorCount](void* data, const AZ::SerializeContext::ClassData* classData,
                const AZ::SerializeContext::ClassElement* field)
            {
                if (classData->m_typeId == azrtti_typeid<JE::Render::RPI::RPISystemDescriptor>())
                {
                    EXPECT_NE(field, nullptr);
                    if (field)
                    {
                        EXPECT_EQ(field->m_nameCrc, AZ_CRC_CE("RpiDescriptor"));
                    }
                    nativeDescriptor = *static_cast<const JE::Render::RPI::RPISystemDescriptor*>(data);
                    ++descriptorCount;
                    return false;
                }
                return true;
            }, {}, &context, AZ::SerializeContext::ENUM_ACCESS_FOR_READ, nullptr);
        ASSERT_TRUE(context.EnumerateInstanceConst(
            &enumeration, reopened->RTTI_AddressOf(componentId), componentId, nullptr, nullptr));
        ASSERT_EQ(descriptorCount, 1);
        EXPECT_EQ(nativeDescriptor.m_imageSystemDescriptor.m_systemAttachmentImagePoolSize, 0);
        EXPECT_EQ(nativeDescriptor.m_commonSrgsShaderAssetPath, "shaders/sceneandviewsrgs.azshader");
        EXPECT_EQ(nativeDescriptor.m_gpuQuerySystemDescriptor.m_occlusionQueryCount, 128);
        EXPECT_EQ(nativeDescriptor.m_gpuQuerySystemDescriptor.m_statisticsQueryCount, 256);
        EXPECT_EQ(nativeDescriptor.m_gpuQuerySystemDescriptor.m_timestampQueryCount, 256);
        EXPECT_EQ(nativeDescriptor.m_imageSystemDescriptor.m_systemStreamingImagePoolSize, 0);
        EXPECT_EQ(nativeDescriptor.m_imageSystemDescriptor.m_systemStreamingImagePoolMipBias, 0);
        EXPECT_EQ(nativeDescriptor.m_dynamicDrawSystemDescriptor.m_dynamicBufferPoolSize, 16 * 1024 * 1024);
        EXPECT_FALSE(nativeDescriptor.m_isNullRenderer);

        AZStd::unique_ptr<AZ::JsonRegistrationContext, void (*)(AZ::JsonRegistrationContext*)> jsonContext(
            aznew AZ::JsonRegistrationContext, [](AZ::JsonRegistrationContext* registration)
            {
                registration->EnableRemoveReflection();
                AZ::JsonSystemComponent::Reflect(registration);
                registration->DisableRemoveReflection();
                delete registration;
            });
        AZ::JsonSystemComponent::Reflect(jsonContext.get());
        const char* patches[]{
            nullptr,
            R"({"GpuQuerySystemDescriptor":{"TimestampQueryCount":17}})",
            R"({"ImageSystemDescriptor":{"SystemStreamingImagePoolMipBias":2}})",
            R"({"ImageSystemDescriptor":{"SystemAttachmentImagePoolSize":1048576}})" };
        for (size_t scenario = 0; scenario < AZ_ARRAY_SIZE(patches); ++scenario)
        {
            SCOPED_TRACE(scenario);
            AZ::SettingsRegistryImpl registry;
            registry.SetContext(&context);
            registry.SetContext(jsonContext.get());
            constexpr AZStd::string_view key = "/O3DE/Atom/RPI/Initialization";
            if (patches[scenario])
            {
                ASSERT_TRUE(registry.MergeSettings(patches[scenario], AZ::SettingsRegistryInterface::Format::JsonMergePatch, key));
            }
            auto effective = nativeDescriptor;
            EXPECT_EQ(registry.GetObject(&effective, azrtti_typeid<JE::Render::RPI::RPISystemDescriptor>(), key), scenario != 0);
            EXPECT_EQ(effective.m_imageSystemDescriptor.m_systemAttachmentImagePoolSize, scenario == 3 ? 1048576 : 0);
            EXPECT_EQ(effective.m_imageSystemDescriptor.m_systemStreamingImagePoolMipBias, scenario == 2 ? 2 : 0);
            EXPECT_EQ(effective.m_imageSystemDescriptor.m_systemStreamingImagePoolSize, 0);
            EXPECT_EQ(effective.m_gpuQuerySystemDescriptor.m_timestampQueryCount, scenario == 1 ? 17 : 256);
            EXPECT_EQ(effective.m_gpuQuerySystemDescriptor.m_occlusionQueryCount, 128);
            EXPECT_EQ(effective.m_gpuQuerySystemDescriptor.m_statisticsQueryCount, 256);
            EXPECT_EQ(effective.m_commonSrgsShaderAssetPath, nativeDescriptor.m_commonSrgsShaderAssetPath);
            EXPECT_EQ(effective.m_dynamicDrawSystemDescriptor.m_dynamicBufferPoolSize, 16 * 1024 * 1024);
            EXPECT_EQ(effective.m_rayTracingSystemDescriptor.m_enableBlasCompaction,
                nativeDescriptor.m_rayTracingSystemDescriptor.m_enableBlasCompaction);
            EXPECT_FALSE(effective.m_isNullRenderer);
        }
    }
#endif

}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
