/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Debug/Profiler/ProfilerModule.h>
#include <JE/Debug/Profiler/CpuProfiler.h>
#include <JE/Debug/Profiler/ProfilerImGuiBus.h>
#include <JE/Debug/Profiler/ImGuiTreemap.h>
#include <JE/Debug/ImGui/ImGuiBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>

namespace JE::Debug::Profiler::Tests
{
    class NativeProfilerTests : public UnitTest::LeakDetectionFixture
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

        bool m_ownsNames = false;

        struct ShutdownOnExit
        {
            CpuProfiler& profiler;
            ~ShutdownOnExit() { profiler.Shutdown(); }
        };
    };

    TEST_F(NativeProfilerTests, FactoriesPreserveModuleComponentAndServiceIdentities)
    {
        const AZ::Uuid captureType("{3f52c1d7-d920-4781-8ed7-88077ec4f305}");
        const AZ::Uuid viewType("{E59A8A53-6784-4CCB-A8B5-9F91DA9BF1C5}");
        {
            AZStd::unique_ptr<AZ::Module> module(CreateCaptureModule());
            ASSERT_NE(module, nullptr);
            EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{4A286414-B387-4D20-9A7E-2F792755B769}"));
            const auto descriptors = module->GetComponentDescriptors();
            ASSERT_EQ(descriptors.size(), 1);
            EXPECT_EQ(descriptors.front()->GetUuid(), captureType);
            EXPECT_STREQ(descriptors.front()->GetName(), "ProfilerSystemComponent");
            EXPECT_EQ(module->GetRequiredSystemComponents(), AZ::ComponentTypeList{ captureType });
            AZ::ComponentDescriptor::DependencyArrayType services;
            descriptors.front()->GetProvidedServices(services, nullptr);
            ASSERT_EQ(services.size(), 1);
            EXPECT_EQ(services[0], AZ_CRC_CE("ProfilerService"));
        }
        {
            AZStd::unique_ptr<AZ::Module> module(CreateVisualModule());
            ASSERT_NE(module, nullptr);
            EXPECT_EQ(module->RTTI_GetType(), AZ::Uuid("{5946991E-A96C-4E7A-A9B3-605E3C8EC3CB}"));
            const auto descriptors = module->GetComponentDescriptors();
            ASSERT_EQ(descriptors.size(), 2);
            EXPECT_EQ(descriptors.front()->GetUuid(), captureType);
            EXPECT_EQ(descriptors.back()->GetUuid(), viewType);
            EXPECT_STREQ(descriptors.back()->GetName(), "ProfilerImGuiSystemComponent");
            const AZ::ComponentTypeList expected{ captureType, viewType };
            EXPECT_EQ(module->GetRequiredSystemComponents(), expected);
        }
    }

    TEST_F(NativeProfilerTests, CaptureOwnerRegistersCoreInterfacesAndDisconnectsOnEntityShutdown)
    {
        EXPECT_EQ(AZ::Debug::ProfilerSystemInterface::Get(), nullptr);
        EXPECT_EQ(AZ::Interface<AZ::Debug::Profiler>::Get(), nullptr);
        AZStd::unique_ptr<AZ::Module> module(CreateCaptureModule());
        {
            AZ::Entity entity("Native profiler capture");
            ASSERT_TRUE(entity.AddComponent(module->GetComponentDescriptors().front()->CreateComponent()));
            ASSERT_NE(AZ::Debug::ProfilerSystemInterface::Get(), nullptr);
            entity.Init();
            entity.Activate();
            ASSERT_NE(AZ::Interface<AZ::Debug::Profiler>::Get(), nullptr);
            AZ::Debug::ProfilerSystemInterface::Get()->SetActive(true);
            EXPECT_TRUE(AZ::Debug::ProfilerSystemInterface::Get()->IsActive());
            AZ::Debug::ProfilerSystemInterface::Get()->SetActive(false);
            EXPECT_FALSE(AZ::Debug::ProfilerSystemInterface::Get()->IsActive());
            entity.Deactivate();
            EXPECT_EQ(AZ::Interface<AZ::Debug::Profiler>::Get(), nullptr);
        }
        EXPECT_EQ(AZ::Debug::ProfilerSystemInterface::Get(), nullptr);
    }

    TEST_F(NativeProfilerTests, DisabledMarkersDoNotDereferenceBudgetOrRegisterRegions)
    {
        CpuProfiler profiler;
        profiler.Init();
        ShutdownOnExit shutdown{ profiler };
        EXPECT_FALSE(profiler.IsProfilerEnabled());
        profiler.BeginRegion(nullptr, "disabled");
        profiler.EndRegion(nullptr);
        profiler.OnSystemTick();
        EXPECT_TRUE(profiler.GetTimeRegionMap().empty());
        profiler.Shutdown();
    }

    TEST_F(NativeProfilerTests, LongLiteralNamesRetainDistinctSuffixesAndTransientInputLifetime)
    {
        CpuProfiler profiler;
        profiler.Init();
        ShutdownOnExit shutdown{ profiler };
        profiler.SetProfilerEnabled(true);
        const AZStd::string prefix(638, 'x');
        const AZStd::string first = prefix + "A";
        const AZStd::string second = prefix + "B";
        AZStd::thread worker([&]
        {
            AZ::Debug::Budget budget("NativeProfilerTest");
            AZStd::string temporary = first;
            profiler.BeginRegion(&budget, temporary.c_str());
            profiler.EndRegion(&budget);
            temporary = second;
            profiler.BeginRegion(&budget, temporary.c_str());
            profiler.EndRegion(&budget);
            temporary.assign(temporary.size(), '?');
        });
        worker.join();
        profiler.OnSystemTick();
        const auto& map = profiler.GetTimeRegionMap();
        ASSERT_EQ(map.size(), 1);
        const auto& regions = map.begin()->second;
        ASSERT_EQ(regions.size(), 2);
        ASSERT_TRUE(regions.contains(first));
        ASSERT_TRUE(regions.contains(second));
        EXPECT_EQ(regions.at(first)[0].m_groupRegionName.m_regionName.GetStringView(), first);
        EXPECT_EQ(regions.at(second)[0].m_groupRegionName.m_regionName.GetStringView(), second);
        profiler.Shutdown();
    }

    TEST_F(NativeProfilerTests, FormattedLongAndNestedRegionsRoundTripThroughOriginalCaptureJsonSchema)
    {
        CpuProfiler profiler;
        profiler.Init();
        ShutdownOnExit shutdown{ profiler };
        ASSERT_TRUE(profiler.BeginContinuousCapture());
        EXPECT_TRUE(profiler.IsContinuousCaptureInProgress());
        const AZStd::string prefix(638, 'y');
        const AZStd::string first = prefix + "A";
        const AZStd::string second = prefix + "B";
        AZStd::thread worker([&]
        {
            AZ::Debug::Budget budget("NativeProfilerTest");
            AZStd::string temporary = prefix;
            profiler.BeginRegion(&budget, "%s%s", temporary.c_str(), "A");
            profiler.BeginRegion(&budget, "nested_%d", 7);
            profiler.EndRegion(&budget);
            profiler.EndRegion(&budget);
            profiler.BeginRegion(&budget, "%s%s", temporary.c_str(), "B");
            profiler.EndRegion(&budget);
            temporary.assign(temporary.size(), '?');
        });
        worker.join();
        profiler.OnSystemTick();
        profiler.OnSystemTick();
        AZStd::ring_buffer<TimeRegionMap> frames;
        ASSERT_TRUE(profiler.EndContinuousCapture(frames));
        EXPECT_FALSE(profiler.IsContinuousCaptureInProgress());
        EXPECT_FALSE(profiler.IsProfilerEnabled());
        profiler.Shutdown();
        CpuProfilingStatisticsSerializer original(frames);
        ASSERT_EQ(original.m_cpuProfilingStatisticsSerializerEntries.size(), 3);

        AZ::SerializeContext context;
        AZ::JsonRegistrationContext registration;
        AZ::Name::Reflect(&context);
        AZ::Name::Reflect(&registration);
        AZ::JsonSystemComponent::Reflect(&registration);
        CpuProfilingStatisticsSerializer::Reflect(&context);
        // Always undo registrations, including an assertion that exits this test early.
        struct Unreflect
        {
            AZ::JsonRegistrationContext& context;
            ~Unreflect()
            {
                context.EnableRemoveReflection();
                AZ::Name::Reflect(&context);
                AZ::JsonSystemComponent::Reflect(&context);
                context.DisableRemoveReflection();
            }
        } unreflect{ registration };
        const auto* data = context.FindClassData(azrtti_typeid<CpuProfilingStatisticsSerializer>());
        ASSERT_NE(data, nullptr);
        EXPECT_STREQ(data->m_name, "CpuProfilingStatisticsSerializer");
        EXPECT_EQ(data->m_version, 1);
        EXPECT_EQ(data->m_typeId, AZ::Uuid("{D5B02946-0D27-474F-9A44-364C2706DD41}"));
        AZ::JsonSerializerSettings save;
        save.m_serializeContext = &context;
        save.m_registrationContext = &registration;
        save.m_keepDefaults = true;
        AZStd::vector<char> bytes;
        AZ::IO::ByteContainerStream stream(&bytes);
        const auto saved = AZ::JsonSerializationUtils::SaveObjectToStream(&original, stream,
            static_cast<const CpuProfilingStatisticsSerializer*>(nullptr), &save);
        ASSERT_TRUE(saved.IsSuccess());
        const AZStd::string_view json(bytes.data(), bytes.size());
        EXPECT_NE(json.find(first), AZStd::string_view::npos);
        EXPECT_NE(json.find(second), AZStd::string_view::npos);
        EXPECT_NE(json.find("\"ClassData\""), AZStd::string_view::npos);
        EXPECT_NE(json.find("\"cpuProfilingStatisticsSerializerEntries\""), AZStd::string_view::npos);
        EXPECT_NE(json.find("\"timeTicksPerSecond\""), AZStd::string_view::npos);
        AZ::JsonDeserializerSettings load;
        load.m_serializeContext = &context;
        load.m_registrationContext = &registration;
        CpuProfilingStatisticsSerializer decoded;
        const auto loaded = AZ::JsonSerializationUtils::LoadObjectFromString(decoded, json, &load);
        ASSERT_TRUE(loaded.IsSuccess());
        ASSERT_EQ(decoded.m_cpuProfilingStatisticsSerializerEntries.size(), 3);
        EXPECT_EQ(decoded.m_timeTicksPerSecond, original.m_timeTicksPerSecond);
        bool foundFirst = false;
        bool foundSecond = false;
        bool foundNested = false;
        for (size_t i = 0; i < original.m_cpuProfilingStatisticsSerializerEntries.size(); ++i)
        {
            const auto& expected = original.m_cpuProfilingStatisticsSerializerEntries[i];
            const auto& actual = decoded.m_cpuProfilingStatisticsSerializerEntries[i];
            EXPECT_EQ(actual.m_groupName, AZ::Name("NativeProfilerTest"));
            EXPECT_EQ(actual.m_regionName, expected.m_regionName);
            EXPECT_EQ(actual.m_stackDepth, expected.m_stackDepth);
            EXPECT_EQ(actual.m_startTick, expected.m_startTick);
            EXPECT_EQ(actual.m_endTick, expected.m_endTick);
            EXPECT_EQ(actual.m_threadId, expected.m_threadId);
            EXPECT_GE(actual.m_endTick, actual.m_startTick);
            foundFirst |= actual.m_regionName.GetStringView() == first;
            foundSecond |= actual.m_regionName.GetStringView() == second;
            if (actual.m_regionName.GetStringView() == "nested_7")
            {
                foundNested = true;
                EXPECT_EQ(actual.m_stackDepth, 1);
            }
        }
        EXPECT_TRUE(foundFirst);
        EXPECT_TRUE(foundSecond);
        EXPECT_TRUE(foundNested);
    }

#if defined(IMGUI_ENABLED)
    TEST_F(NativeProfilerTests, VisualOwnerProvidesSharedInterfacesAndTreemapDrawThenDisconnects)
    {
        AZStd::unique_ptr<AZ::Module> module(CreateVisualModule());
        EXPECT_EQ(ProfilerImGuiInterface::Get(), nullptr);
        EXPECT_EQ(ImGuiTreemapFactory::Interface::Get(), nullptr);
        {
            AZ::Entity entity("Native profiler view");
            for (auto* descriptor : module->GetComponentDescriptors())
            {
                ASSERT_TRUE(entity.AddComponent(descriptor->CreateComponent()));
            }
            entity.Init();
            entity.Activate();
            ASSERT_NE(AZ::Interface<AZ::Debug::Profiler>::Get(), nullptr);
            ASSERT_NE(ProfilerImGuiInterface::Get(), nullptr);
            ASSERT_NE(ImGuiTreemapFactory::Interface::Get(), nullptr);
            EXPECT_TRUE(JE::Debug::ImGuiUpdateListenerBus::HasHandlers());
            struct Context
            {
                ImGuiContext* value = ImGui::CreateContext();
                ~Context() { ImGui::DestroyContext(value); }
            } context;
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.DisplaySize = ImVec2(640, 480);
            io.DeltaTime = 1.0f / 60.0f;
            ASSERT_NE(io.Fonts->AddFontDefault(), nullptr);
            ASSERT_TRUE(io.Fonts->Build());
            auto* factory = ImGuiTreemapFactory::Interface::Get();
            auto& treemap = factory->Create(AZ::Name("Native treemap"), "MiB");
            AZStd::vector<TreemapNode> roots(2);
            roots[0].m_name = AZ::Name("large");
            roots[0].m_weight = 3;
            roots[1].m_name = AZ::Name("small");
            roots[1].m_weight = 1;
            treemap.SetRoots(AZStd::move(roots));
            treemap.WeighAndComputeLayout(320, 240);
            EXPECT_EQ(treemap.GetName(), AZ::Name("Native treemap"));
            ImGui::NewFrame();
            treemap.Render(0, 0, 320, 240);
            ImGui::Render();
            ASSERT_NE(ImGui::GetDrawData(), nullptr);
            EXPECT_GT(ImGui::GetDrawData()->TotalVtxCount, 0);
            factory->Destroy(&treemap);
            entity.Deactivate();
            EXPECT_EQ(AZ::Interface<AZ::Debug::Profiler>::Get(), nullptr);
            EXPECT_FALSE(JE::Debug::ImGuiUpdateListenerBus::HasHandlers());
        }
        EXPECT_EQ(ProfilerImGuiInterface::Get(), nullptr);
        EXPECT_EQ(ImGuiTreemapFactory::Interface::Get(), nullptr);
        EXPECT_EQ(AZ::Debug::ProfilerSystemInterface::Get(), nullptr);
    }
#endif
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
