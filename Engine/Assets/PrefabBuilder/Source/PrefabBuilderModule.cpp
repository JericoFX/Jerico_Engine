/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Assets/Prefab/PrefabBuilderModule.h>
#include <JE/Assets/Prefab/PrefabBuilderComponent.h>
#include <JE/Assets/Prefab/PrefabGroup/PrefabGroupBehavior.h>

namespace JE::Assets::Prefab
{
    PrefabBuilderModule::PrefabBuilderModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            PrefabBuilderComponent::CreateDescriptor(),
            PrefabGroupBehavior::CreateDescriptor() });
    }

    PrefabBuilderModule::~PrefabBuilderModule() = default;

    AZ::Module* CreateToolsModule()
    {
        return aznew PrefabBuilderModule;
    }
}
