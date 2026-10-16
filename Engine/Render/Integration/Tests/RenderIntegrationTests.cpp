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
#include <AzCore/Component/Entity.h>
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
                shared->RegisterDynamicDrawContext(name, [&factoryCalls](AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext>)
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
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
