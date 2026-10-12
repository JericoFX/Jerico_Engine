/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <JE/Debug/ImGui/ImGuiExports.h>
#include <IGem.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace JE::Debug
{
    class JE_DEBUG_IMGUI_API ImGuiModule : public CryHooksModule
    {
    public:
        AZ_CLASS_ALLOCATOR(ImGuiModule, AZ::SystemAllocator);
        AZ_RTTI_WITH_NAME(ImGuiModule, "ImGui::ImGuiModule", "{ECA9F41C-716E-4395-A096-5A519227F9A4}", CryHooksModule);

        ImGuiModule();
        ~ImGuiModule() override;
        void OnSystemEvent(ESystemEvent event, uintptr_t wparam, uintptr_t lparam) override;

    private:
        struct Implementation;
        AZStd::unique_ptr<Implementation> m_implementation;
    };

    class JE_DEBUG_IMGUI_API ImGuiEditorWindowModule final : public ImGuiModule
    {
    public:
        AZ_RTTI_WITH_NAME(ImGuiEditorWindowModule, "ImGui::ImGuiEditorWindowModule", "{DDC7A763-A36F-46D8-9885-43E0293C1D03}", ImGuiModule);
        ImGuiEditorWindowModule();
        ~ImGuiEditorWindowModule() override;
    };

    //! Ownership transfers to the existing host Module lifecycle. Register only one factory per host.
    JE_DEBUG_IMGUI_API AZ::Module* CreateRuntimeModule();
    JE_DEBUG_IMGUI_API AZ::Module* CreateToolsModule();
}
