/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Debug/Profiler/ProfilerExports.h>

#include <JE/Debug/Profiler/ProfilerImGuiBus.h>

#include <ImGuiCpuProfiler.h>
#include <ImGuiHeapMemoryProfiler.h>

#include "ImGuiTreemapImpl.h"

#include <AzCore/Component/Component.h>

#if defined(IMGUI_ENABLED)
#include <JE/Debug/ImGui/ImGuiBus.h>
#include <imgui/imgui.h>
#endif // defined(IMGUI_ENABLED)

namespace JE::Debug::Profiler
{
    class JE_PROFILER_VIEW_API ProfilerImGuiSystemComponent
        : public AZ::Component
#if defined(IMGUI_ENABLED)
        , public ProfilerImGuiRequests
        , public JE::Debug::ImGuiUpdateListenerBus::Handler
#endif // defined(IMGUI_ENABLED)
    {
    public:
        AZ_COMPONENT(ProfilerImGuiSystemComponent, "{E59A8A53-6784-4CCB-A8B5-9F91DA9BF1C5}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ProfilerImGuiSystemComponent();
        ~ProfilerImGuiSystemComponent();

    protected:
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

#if defined(IMGUI_ENABLED)
        // ProfilerImGuiRequests interface implementation
        void ShowCpuProfilerWindow(bool& keepDrawing) override;

        // ImGuiUpdateListenerBus overrides
        void OnImGuiUpdate() override;
        void OnImGuiMainMenuUpdate() override;
#endif // defined(IMGUI_ENABLED)

    private:
#if defined(IMGUI_ENABLED)
        ImGuiTreemapFactoryImpl m_imguiTreemapFactory;
        ImGuiCpuProfiler m_imguiCpuProfiler;
        ImGuiHeapMemoryProfiler m_imguiHeapMemoryProfiler;
        bool m_showCpuProfiler{ false };
        bool m_showHeapMemoryProfiler{ false };
#endif // defined(IMGUI_ENABLED)
    };

} // namespace JE::Debug::Profiler
