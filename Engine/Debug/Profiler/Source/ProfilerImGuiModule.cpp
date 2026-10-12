/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Debug/Profiler/ProfilerModule.h>
#include "ProfilerSystemComponent.h"
#include "ProfilerImGuiSystemComponent.h"

namespace JE::Debug::Profiler
{
    ProfilerImGuiModule::ProfilerImGuiModule()
    {
        m_descriptors.push_back(ProfilerSystemComponent::CreateDescriptor());
        m_descriptors.push_back(ProfilerImGuiSystemComponent::CreateDescriptor());
    }

    AZ::ComponentTypeList ProfilerImGuiModule::GetRequiredSystemComponents() const
    {
        return { azrtti_typeid<ProfilerSystemComponent>(), azrtti_typeid<ProfilerImGuiSystemComponent>() };
    }

    AZ::Module* CreateVisualModule()
    {
        return aznew ProfilerImGuiModule();
    }
}
