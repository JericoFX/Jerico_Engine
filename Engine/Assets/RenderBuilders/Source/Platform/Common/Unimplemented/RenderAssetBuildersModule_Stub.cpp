/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Assets/RenderBuilders/RenderAssetBuildersModule.h>

namespace JE::Assets::RenderBuilders
{
    RenderAssetBuildersModule::RenderAssetBuildersModule() = default;

    AZ::ComponentTypeList RenderAssetBuildersModule::GetRequiredSystemComponents() const
    {
        return {};
    }

    AZ::Module* CreateToolsModule()
    {
        return aznew RenderAssetBuildersModule();
    }
} // namespace JE::Assets::RenderBuilders
