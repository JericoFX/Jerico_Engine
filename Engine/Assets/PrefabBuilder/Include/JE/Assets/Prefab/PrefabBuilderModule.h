/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <JE/Assets/Prefab/PrefabBuilderExports.h>
#include <AzCore/Module/Module.h>

namespace JE::Assets::Prefab
{
    class JE_PREFAB_BUILDER_API PrefabBuilderModule final : public AZ::Module
    {
    public:
        AZ_RTTI_WITH_NAME(PrefabBuilderModule, "AZ::Prefab::PrefabBuilderModule", "{088B2BA8-9F19-469C-A0B5-1DD523879C70}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PrefabBuilderModule, AZ::SystemAllocator);
        PrefabBuilderModule();
        ~PrefabBuilderModule() override;
    };

    //! Ownership transfers to the host's existing module lifecycle. Activation remains tag/SceneAPI driven.
    JE_PREFAB_BUILDER_API AZ::Module* CreateToolsModule();
}
