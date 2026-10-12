/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <JE/Debug/Profiler/ProfilerExports.h>
#include <AzCore/Module/Module.h>

namespace JE::Debug::Profiler
{
    class JE_PROFILER_CAPTURE_API ProfilerModule : public AZ::Module
    {
    public:
        AZ_RTTI(ProfilerModule, "{4A286414-B387-4D20-9A7E-2F792755B769}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ProfilerModule, AZ::SystemAllocator);
        ProfilerModule();
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };

    class JE_PROFILER_VIEW_API ProfilerImGuiModule : public AZ::Module
    {
    public:
        AZ_RTTI(ProfilerImGuiModule, "{5946991E-A96C-4E7A-A9B3-605E3C8EC3CB}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ProfilerImGuiModule, AZ::SystemAllocator);
        ProfilerImGuiModule();
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };

    // Hosts select one factory; the visual module includes the shared capture descriptor.
    JE_PROFILER_CAPTURE_API AZ::Module* CreateCaptureModule();
    JE_PROFILER_VIEW_API AZ::Module* CreateVisualModule();
}
