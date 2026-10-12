/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Debug/ImGui/ImGuiModule.h>
#include "ImGuiManager.h"
#ifdef IMGUI_ENABLED
#include "LYCommonMenu/ImGuiLYCommonMenu.h"
#endif

namespace JE::Debug
{
    struct ImGuiModule::Implementation
    {
#ifdef IMGUI_ENABLED
        ImGuiLYCommonMenu commonMenu;
        ImGuiManager manager;
#endif
    };

    ImGuiModule::ImGuiModule()
        : m_implementation(AZStd::make_unique<Implementation>())
    {
    }
    ImGuiModule::~ImGuiModule() = default;
    ImGuiEditorWindowModule::ImGuiEditorWindowModule() = default;
    ImGuiEditorWindowModule::~ImGuiEditorWindowModule() = default;

    void ImGuiModule::OnSystemEvent([[maybe_unused]] ESystemEvent event, [[maybe_unused]] uintptr_t wparam, [[maybe_unused]] uintptr_t lparam)
    {
#ifdef IMGUI_ENABLED
        switch (event)
        {
        case ESYSTEM_EVENT_GAME_POST_INIT:
            m_implementation->manager.Initialize();
            m_implementation->commonMenu.Initialize();
            break;
        case ESYSTEM_EVENT_FULL_SHUTDOWN:
        case ESYSTEM_EVENT_FAST_SHUTDOWN:
            m_implementation->manager.Shutdown();
            m_implementation->commonMenu.Shutdown();
            break;
        case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
            m_implementation->manager.RegisterImGuiCVARs();
            break;
        default:
            break;
        }
#endif
    }

    AZ::Module* CreateRuntimeModule() { return aznew ImGuiModule(); }
    AZ::Module* CreateToolsModule() { return aznew ImGuiEditorWindowModule(); }
}
