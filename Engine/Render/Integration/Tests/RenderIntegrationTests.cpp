/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Render/Integration/RenderIntegrationModule.h>
#include <JE/Render/Integration/FlyCameraInputBus.h>
#include <JE/Render/Bootstrap/BootstrapRequestBus.h>
#include <JE/Render/Bootstrap/DefaultWindowBus.h>
#include <AtomBridgeSystemComponent.h>
#include <FlyCameraInputComponent.h>
#include <PerViewportDynamicDrawManager.h>
#include <Atom/RHI/Factory.h>
#include <JE/Render/RPI/Public/RPISystem.h>
#include <AzCore/PlatformIncl.h>
#include <JE/Render/RPI/Reflect/RPISystemDescriptor.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Component/Entity.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <JE/Render/RPI/Public/Image/ImageTagSystemComponent.h>
#include <JE/Render/RPI/Public/Model/ModelTagSystemComponent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>

namespace JE::Render::Integration
{
    class TestTransformComponent : public AzFramework::TransformComponent
    {
    public:
        AZ_TYPE_INFO(TestTransformComponent, "{E4D0DF34-AB2B-4BC5-823F-35A3E82B8FEB}");
        using AzFramework::TransformComponent::Activate;
        using AzFramework::TransformComponent::Deactivate;
    };

    class RenderIntegrationTests : public UnitTest::LeakDetectionFixture
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

    TEST_F(RenderIntegrationTests, RuntimeFactoryPreservesIdentityDescriptorsAndRendererServices)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateRuntimeModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{92196B90-6DF5-479D-8746-296AF56F0ABA}"));
        AZStd::unordered_set<AZ::Uuid> expected{
            AZ::Uuid("{7AE0D6AD-691C-41B6-9DD5-F23F78B1A02E}"),
            AZ::Uuid("{FFB99CE4-2C9E-476D-8140-50A8A696E242}") };
        ASSERT_EQ(module->GetComponentDescriptors().size(), 2);
        for (const auto* descriptor : module->GetComponentDescriptors())
        {
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(expected.erase(descriptor->GetUuid()), 1);
        }
        EXPECT_TRUE(expected.empty());
        const auto requiredComponents = module->GetRequiredSystemComponents();
        ASSERT_EQ(requiredComponents.size(), 1);
        EXPECT_EQ(requiredComponents.front(), AZ::Uuid("{FFB99CE4-2C9E-476D-8140-50A8A696E242}"));
        AZ::ComponentDescriptor::DependencyArrayType required;
        AtomBridgeSystemComponent::GetRequiredServices(required);
        AZStd::unordered_set<AZ::Crc32> services{
            AZ::RHI::Factory::GetComponentService(), AZ_CRC_CE("AssetDatabaseService"),
            AZ_CRC_CE("RPISystem"), AZ_CRC_CE("BootstrapSystemComponent") };
        ASSERT_EQ(required.size(), services.size());
        for (const auto service : required)
        {
            EXPECT_EQ(services.erase(service), 1);
        }
        EXPECT_TRUE(services.empty());
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        AtomBridgeSystemComponent::Reflect(&context);
        const auto* reflected = context.FindClassData(requiredComponents.front());
        ASSERT_NE(reflected, nullptr);
        EXPECT_STREQ(reflected->m_name, "AtomBridgeSystemComponent");
        EXPECT_EQ(reflected->m_version, 0);
    }

    TEST_F(RenderIntegrationTests, BootstrapFactoryPreservesLegacyIdentityDescriptorServicesAndReflection)
    {
        const AZ::Uuid moduleId("{ADDE20F4-03E6-4692-A736-E56B87952727}");
        const AZ::Uuid componentId("{1EAFD87D-A64A-4612-93D8-B3AFFA70F09B}");
        const AZ::ComponentTypeList requiredComponents{ componentId };
        AZStd::unique_ptr<AZ::Module> module(CreateBootstrapModule());
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), moduleId);
        EXPECT_STREQ(module->RTTI_GetTypeName(), "BootstrapModule");
        EXPECT_EQ(module->GetRequiredSystemComponents(), requiredComponents);
        ASSERT_EQ(module->GetComponentDescriptors().size(), 1);
        auto* descriptor = module->GetComponentDescriptors().front();
        ASSERT_NE(descriptor, nullptr);
        EXPECT_EQ(descriptor->GetUuid(), componentId);
        AZ::ComponentDescriptor::DependencyArrayType provided, incompatible, required, dependent;
        descriptor->GetProvidedServices(provided, nullptr);
        descriptor->GetIncompatibleServices(incompatible, nullptr);
        descriptor->GetRequiredServices(required, nullptr);
        descriptor->GetDependentServices(dependent, nullptr);
        const AZ::ComponentDescriptor::DependencyArrayType bootstrapService{ AZ_CRC_CE("BootstrapSystemComponent") };
        const AZ::ComponentDescriptor::DependencyArrayType requiredServices{
            AZ_CRC_CE("RPISystem"), AZ_CRC_CE("SceneSystemComponentService") };
        const AZ::ComponentDescriptor::DependencyArrayType dependentServices{
            AZ_CRC_CE("ImGuiSystemComponent"), AZ_CRC_CE("PrimitiveSystemComponent"), AZ_CRC_CE("MeshSystemComponent"),
            AZ_CRC_CE("CoreLightsService"), AZ_CRC_CE("DynamicDrawService"), AZ_CRC_CE("CommonService"), AZ_CRC_CE("HairService") };
        EXPECT_EQ(provided, bootstrapService);
        EXPECT_EQ(incompatible, bootstrapService);
        EXPECT_EQ(required, requiredServices);
        EXPECT_EQ(dependent, dependentServices);
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        descriptor->Reflect(&context);
        const auto* reflected = context.FindClassData(componentId);
        ASSERT_NE(reflected, nullptr);
        EXPECT_STREQ(reflected->m_name, "BootstrapSystemComponent");
        EXPECT_EQ(reflected->m_version, 1);
        {
            AZStd::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
            ASSERT_NE(component, nullptr);
            EXPECT_EQ(component->RTTI_GetType(), componentId);
        }
        module.reset();
        module.reset(CreateBootstrapModule(true));
        ASSERT_NE(module, nullptr);
        EXPECT_EQ(module->RTTI_GetType(), moduleId);
        EXPECT_EQ(module->GetRequiredSystemComponents(), requiredComponents);
        ASSERT_EQ(module->GetComponentDescriptors().size(), 1);
        ASSERT_NE(module->GetComponentDescriptors().front(), nullptr);
        EXPECT_EQ(module->GetComponentDescriptors().front()->GetUuid(), componentId);
    }

    TEST_F(RenderIntegrationTests, BootstrapDescriptorOnlyFactoryDoesNotRequireOrActivateRenderer)
    {
        const bool hadRequests = JE::Render::Bootstrap::RequestBus::HasHandlers();
        const bool hadWindow = JE::Render::Bootstrap::DefaultWindowBus::HasHandlers();
        for (int attempt = 0; attempt < 2; ++attempt)
        {
            AZStd::unique_ptr<AZ::Module> module(CreateBootstrapModule(false));
            ASSERT_NE(module, nullptr);
            EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{ADDE20F4-03E6-4692-A736-E56B87952727}"));
            EXPECT_STREQ(module->RTTI_GetTypeName(), "BootstrapModule");
            EXPECT_TRUE(module->GetRequiredSystemComponents().empty());
            ASSERT_EQ(module->GetComponentDescriptors().size(), 1);
            auto* descriptor = module->GetComponentDescriptors().front();
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(descriptor->GetUuid(), AZ::Uuid("{1EAFD87D-A64A-4612-93D8-B3AFFA70F09B}"));
            AZStd::unique_ptr<AZ::Component> component(descriptor->CreateComponent());
            ASSERT_NE(component, nullptr);
            EXPECT_EQ(component->RTTI_GetType(), descriptor->GetUuid());
            EXPECT_EQ(JE::Render::Bootstrap::RequestBus::HasHandlers(), hadRequests);
            EXPECT_EQ(JE::Render::Bootstrap::DefaultWindowBus::HasHandlers(), hadWindow);
        }
        EXPECT_EQ(JE::Render::Bootstrap::RequestBus::HasHandlers(), hadRequests);
        EXPECT_EQ(JE::Render::Bootstrap::DefaultWindowBus::HasHandlers(), hadWindow);
    }

    TEST_F(RenderIntegrationTests, FlyCameraXmlRoundTripPreservesLegacyIdentityAndAllFields)
    {
        AZ::SerializeContext context;
        AZ::Entity::Reflect(&context);
        FlyCameraInputComponent::Reflect(&context);
        const auto* reflected = context.FindClassData(AZ::Uuid("{7AE0D6AD-691C-41B6-9DD5-F23F78B1A02E}"));
        ASSERT_NE(reflected, nullptr);
        EXPECT_STREQ(reflected->m_name, "FlyCameraInputComponent");
        EXPECT_EQ(reflected->m_version, 1);
        FlyCameraInputComponent original;
        original.SetIsEnabled(false);
        AZStd::vector<char> bytes;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&bytes);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_XML, &original, &context));
        const AZStd::string xml(bytes.begin(), bytes.end());
        EXPECT_NE(xml.find("name=\"FlyCameraInputComponent\""), AZStd::string::npos);
        EXPECT_NE(xml.find("{7AE0D6AD-691C-41B6-9DD5-F23F78B1A02E}"), AZStd::string::npos);
        for (const char* field : { "Move Speed", "Rotation Speed", "Mouse Sensitivity",
                 "Invert Rotation Input X", "Invert Rotation Input Y", "Is enabled" })
        {
            EXPECT_NE(xml.find(AZStd::string::format("field=\"%s\"", field)), AZStd::string::npos);
        }
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZStd::unique_ptr<FlyCameraInputComponent> reopened(
            AZ::Utils::LoadObjectFromStream<FlyCameraInputComponent>(stream, &context));
        ASSERT_NE(reopened, nullptr);
        EXPECT_FALSE(reopened->GetIsEnabled());
        AZStd::vector<char> roundTrip;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> restoredStream(&roundTrip);
        ASSERT_TRUE(AZ::Utils::SaveObjectToStream(restoredStream, AZ::ObjectStream::ST_XML, reopened.get(), &context));
        EXPECT_EQ(roundTrip, bytes);
    }

    TEST_F(RenderIntegrationTests, FlyCameraBusLifecycleAndDisabledTickPreserveExistingTransform)
    {
        AZ::Entity entity("Native fly input");
        auto* transform = entity.CreateComponent<TestTransformComponent>();
        auto* fly = entity.CreateComponent<FlyCameraInputComponent>();
        ASSERT_NE(transform, nullptr);
        ASSERT_NE(fly, nullptr);
        entity.Init();
        transform->Activate();
        const AZ::Transform authored = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 4.0f, 8.0f));
        transform->SetWorldTM(authored);
        fly->Activate();
        EXPECT_TRUE(FlyCameraInputBus::HasHandlers(entity.GetId()));
        FlyCameraInputBus::Event(entity.GetId(), &FlyCameraInputInterface::SetIsEnabled, false);
        bool enabled = true;
        FlyCameraInputBus::EventResult(enabled, entity.GetId(), &FlyCameraInputInterface::GetIsEnabled);
        EXPECT_FALSE(enabled);
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.016f, AZ::ScriptTimePoint());
        EXPECT_EQ(transform->GetWorldTM(), authored);
        fly->Deactivate();
        EXPECT_FALSE(FlyCameraInputBus::HasHandlers(entity.GetId()));
        fly->Activate();
        FlyCameraInputBus::Event(entity.GetId(), &FlyCameraInputInterface::SetIsEnabled, true);
        FlyCameraInputBus::EventResult(enabled, entity.GetId(), &FlyCameraInputInterface::GetIsEnabled);
        EXPECT_TRUE(enabled);
        fly->Deactivate();
        EXPECT_FALSE(FlyCameraInputBus::HasHandlers(entity.GetId()));
        transform->Deactivate();
    }

    TEST_F(RenderIntegrationTests, DynamicDrawOwnerRegistersDisconnectsAndRejectsUnknownNamesWithoutGpuWork)
    {
        EXPECT_EQ(PerViewportDynamicDraw::Get(), nullptr);
        for (int attempt = 0; attempt < 2; ++attempt)
        {
            {
                PerViewportDynamicDrawManager owner;
                ASSERT_EQ(PerViewportDynamicDraw::Get(), &owner);
                const AZ::Name name("native.test.dynamic.draw");
                unsigned int factoryCalls = 0;
                auto* shared = PerViewportDynamicDraw::Get();
                shared->RegisterDynamicDrawContext(name, [&factoryCalls](AZ::RHI::Ptr<JE::Render::RPI::DynamicDrawContext>)
                {
                    ++factoryCalls;
                });
                EXPECT_EQ(shared->GetDynamicDrawContextForViewport(AZ::Name("unregistered"), 91), nullptr);
                shared->UnregisterDynamicDrawContext(name);
                EXPECT_EQ(shared->GetDynamicDrawContextForViewport(name, 91), nullptr);
                EXPECT_EQ(factoryCalls, 0);
            }
            EXPECT_EQ(PerViewportDynamicDraw::Get(), nullptr);
        }
    }
#if defined(AZ_PLATFORM_WINDOWS)
    TEST_F(RenderIntegrationTests, RpiRuntimeBodyResolvesFromNativeRenderIntegration)
    {
        HMODULE provider = nullptr;
        ASSERT_TRUE(::GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&JE::Render::RPI::RPISystem::Reflect), &provider));
        const HMODULE expected = ::GetModuleHandleW(L"RenderIntegration.dll");
        ASSERT_NE(expected, nullptr);
        EXPECT_EQ(provider, expected) << "The real RPI implementation still lives in its legacy DLL";
    }
    TEST_F(RenderIntegrationTests, NativeRpiRuntimeFactoryPreservesRegistrationAndLifecycle)
    {
        // Probe the native Windows C++ export without linking an as-yet absent factory.
        auto library = AZ::DynamicModuleHandle::Create("RenderIntegration");
        ASSERT_NE(library, nullptr);
        ASSERT_TRUE(library->Load(AZ::DynamicModuleHandle::LoadFlags::NoLoad));
        using Factory = AZ::Module* (*)();
        const auto factory = library->GetFunction<Factory>("?CreateRpiRuntimeModule@Integration@Render@JE@@YAPEAVModule@AZ@@XZ");
        ASSERT_NE(factory, nullptr) << "Native RPI factory is missing from RenderIntegration";
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
            EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{CDB54E96-717D-4DFC-BEA6-F809BDE601AE}"));
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
            EXPECT_EQ(FunctorRegistration::Get() != nullptr, false);
            component.reset();
            EXPECT_EQ(FunctorRegistration::Get(), nullptr);
        }
    }

    TEST_F(RenderIntegrationTests, NativeRpiDescriptorRoundTripPreservesDefaultsAndPartialRegistryOverrides)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateRpiRuntimeModule());
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
