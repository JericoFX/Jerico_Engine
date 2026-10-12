/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Debug/ImGui/ImGuiModule.h>
#include <JE/Debug/ImGui/ImGuiBus.h>
#include <JE/Debug/ImGui/ImGuiContextScope.h>
#include <JE/Debug/ImGui/HistogramContainer.h>
#include <JE/Debug/ImGui/HistogramGroup.h>
#include <JE/Debug/ImGui/CurveEditor.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <ISystemMock.h>

namespace JE::Debug::Tests
{
    class NativeImGuiTests : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(NativeImGuiTests, FactoriesPreserveModuleIdentitiesWithoutSystemComponents)
    {
        AZStd::unique_ptr<AZ::Module> runtime(CreateRuntimeModule());
        AZStd::unique_ptr<AZ::Module> tools(CreateToolsModule());
        ASSERT_NE(runtime, nullptr);
        ASSERT_NE(tools, nullptr);
        EXPECT_EQ(runtime->RTTI_GetType(), AZ::Uuid("{ECA9F41C-716E-4395-A096-5A519227F9A4}"));
        EXPECT_EQ(tools->RTTI_GetType(), AZ::Uuid("{DDC7A763-A36F-46D8-9885-43E0293C1D03}"));
        EXPECT_TRUE(runtime->GetComponentDescriptors().empty());
        EXPECT_TRUE(tools->GetComponentDescriptors().empty());
        EXPECT_TRUE(runtime->GetRequiredSystemComponents().empty());
        EXPECT_TRUE(tools->GetRequiredSystemComponents().empty());
    }

#ifdef IMGUI_ENABLED
    class NativeImGuiLifecycleTests : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_system = AZStd::make_unique<testing::NiceMock<SystemMock>>();
            ON_CALL(*m_system, GetGlobalEnvironment()).WillByDefault(testing::Return(&m_environment));
            m_module.reset(azrtti_cast<ImGuiModule*>(CreateRuntimeModule()));
            SSystemInitParams parameters;
            CrySystemEventBus::Broadcast(&CrySystemEvents::OnCrySystemPreInitialize, *m_system, parameters);
        }

        void TearDown() override
        {
            if (m_started)
            {
                m_module->OnSystemEvent(ESYSTEM_EVENT_FULL_SHUTDOWN, 0, 0);
            }
            CrySystemEventBus::Broadcast(&CrySystemEvents::OnCrySystemPostShutdown);
            m_module.reset();
            m_system.reset();
            LeakDetectionFixture::TearDown();
        }

        SSystemGlobalEnvironment m_environment;
        AZStd::unique_ptr<testing::NiceMock<SystemMock>> m_system;
        AZStd::unique_ptr<ImGuiModule> m_module;
        bool m_started = false;
    };

    TEST_F(NativeImGuiLifecycleTests, OriginalCryLifecycleCreatesOneContextAndSharedManagerThenDisconnects)
    {
        class Listener final : public ImGuiUpdateListenerBus::Handler
        {
        public:
            void OnImGuiInitialize() override { ++initialized; }
            int initialized = 0;
        } listener;
        listener.BusConnect();
        EXPECT_EQ(AZ::Interface<IImGuiManager>::Get(), nullptr);
        EXPECT_EQ(ImGui::GetCurrentContext(), nullptr);
        m_started = true;
        m_module->OnSystemEvent(ESYSTEM_EVENT_GAME_POST_INIT, 0, 0);
        ASSERT_NE(AZ::Interface<IImGuiManager>::Get(), nullptr);
        EXPECT_NE(ImGui::GetCurrentContext(), nullptr);
        EXPECT_EQ(listener.initialized, 1);
        EXPECT_TRUE(ImGuiManagerBus::HasHandlers());
        ImGuiManagerBus::Broadcast(&IImGuiManager::SetDisplayState, DisplayState::Visible);
        EXPECT_EQ(AZ::Interface<IImGuiManager>::Get()->GetDisplayState(), DisplayState::Visible);
        m_module->OnSystemEvent(ESYSTEM_EVENT_FULL_SHUTDOWN, 0, 0);
        m_started = false;
        EXPECT_EQ(AZ::Interface<IImGuiManager>::Get(), nullptr);
        EXPECT_EQ(ImGui::GetCurrentContext(), nullptr);
        EXPECT_FALSE(ImGuiManagerBus::HasHandlers());
        EXPECT_FALSE(ImGuiEntityOutlinerRequestBus::HasHandlers());
        listener.BusDisconnect();
    }

    class NativeImGuiDrawingTests : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_context = ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.DisplaySize = ImVec2(640, 480);
            io.DeltaTime = 1.0f / 60.0f;
            ASSERT_NE(io.Fonts->AddFontDefault(), nullptr);
            ASSERT_TRUE(io.Fonts->Build());
        }

        void TearDown() override
        {
            ImGui::DestroyContext(m_context);
            LeakDetectionFixture::TearDown();
        }
        ImGuiContext* m_context = nullptr;
    };

    TEST_F(NativeImGuiDrawingTests, ContextScopeRestoresTheSharedDearImGuiContext)
    {
        ImGuiContext* temporary = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_context);
        {
            ImGuiContextScope scope(temporary);
            EXPECT_EQ(ImGui::GetCurrentContext(), temporary);
        }
        EXPECT_EQ(ImGui::GetCurrentContext(), m_context);
        ImGui::DestroyContext(temporary);
    }

    TEST_F(NativeImGuiDrawingTests, HistogramKeepsBoundedHistoryInBothDirectionsAndProducesDrawData)
    {
        HistogramContainer histogram;
        histogram.Init("history", 3, HistogramContainer::ViewType::Histogram, false, 0.0f, 10.0f);
        histogram.PushValue(1);
        histogram.PushValue(2);
        histogram.PushValue(3);
        histogram.PushValue(4);
        EXPECT_EQ(histogram.GetSize(), 3);
        EXPECT_FLOAT_EQ(histogram.GetValue(0), 4);
        EXPECT_FLOAT_EQ(histogram.GetValue(2), 2);
        histogram.SetMoveDirection(HistogramContainer::PushRightMoveLeft);
        histogram.PushValue(5);
        EXPECT_FLOAT_EQ(histogram.GetValue(0), 3);
        EXPECT_FLOAT_EQ(histogram.GetValue(2), 5);
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(500, 300));
        ImGui::Begin("Native histogram");
        histogram.Draw(300, 120);
        ImGui::End();
        ImGui::Render();
        ASSERT_NE(ImGui::GetDrawData(), nullptr);
        EXPECT_GT(ImGui::GetDrawData()->TotalVtxCount, 0);
    }

    TEST_F(NativeImGuiDrawingTests, HistogramGroupReusesAnExistingNamedHistory)
    {
        HistogramGroup group("group", 3);
        const char* name = "CPU";
        group.PushHistogramValue(name, 1, AZ::Color::CreateOne());
        HistogramContainer* history = group.FindContainerByName(name);
        ASSERT_NE(history, nullptr);
        group.PushHistogramValue(name, 2, AZ::Color::CreateOne());
        EXPECT_EQ(group.FindContainerByName(name), history);
        EXPECT_FLOAT_EQ(history->GetValue(0), 0);
        EXPECT_FLOAT_EQ(history->GetValue(1), 1);
        EXPECT_FLOAT_EQ(history->GetValue(2), 2);
        EXPECT_FLOAT_EQ(history->GetLastValue(), 2);
    }

    TEST_F(NativeImGuiDrawingTests, CurveCoordinateConversionPreservesCornersAndInteriorPoints)
    {
        CurveGraphDataParams graph;
        graph.minPoint = ImVec2(-2, -4);
        graph.maxPoint = ImVec2(6, 8);
        graph.width = 8;
        graph.height = 12;
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(500, 300));
        ImGui::Begin("Native curve");
        const ImVec2 points[] = { graph.minPoint, graph.maxPoint, ImVec2(2, 2) };
        ImVec2 screen[3];
        for (size_t index = 0; index < 3; ++index)
        {
            screen[index] = TransformToEditorSpace(points[index], graph);
            const ImVec2 restored = InverseTransformToEditorSpace(screen[index], graph);
            EXPECT_NEAR(restored.x, points[index].x, 0.0001f);
            EXPECT_NEAR(restored.y, points[index].y, 0.0001f);
        }
        EXPECT_LT(screen[0].x, screen[2].x);
        EXPECT_LT(screen[2].x, screen[1].x);
        EXPECT_GT(screen[0].y, screen[2].y);
        EXPECT_GT(screen[2].y, screen[1].y);
        EXPECT_NEAR(screen[2].x, (screen[0].x + screen[1].x) * 0.5f, 0.0001f);
        EXPECT_NEAR(screen[2].y, (screen[0].y + screen[1].y) * 0.5f, 0.0001f);
        ImGui::End();
        ImGui::Render();
    }
#endif
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
