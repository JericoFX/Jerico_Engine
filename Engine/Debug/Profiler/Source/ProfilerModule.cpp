/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Debug/Profiler/ProfilerModule.h>
#include "ProfilerSystemComponent.h"

namespace JE::Debug::Profiler
{
    ProfilerModule::ProfilerModule()
    {
        m_descriptors.push_back(ProfilerSystemComponent::CreateDescriptor());
    }

    AZ::ComponentTypeList ProfilerModule::GetRequiredSystemComponents() const
    {
        return { azrtti_typeid<ProfilerSystemComponent>() };
    }

    AZ::Module* CreateCaptureModule()
    {
        return aznew ProfilerModule();
    }
}
