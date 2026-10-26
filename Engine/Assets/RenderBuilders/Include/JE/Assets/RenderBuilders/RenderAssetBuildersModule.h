/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Assets/RenderBuilders/RenderAssetBuildersExports.h>
#include <AzCore/Module/Module.h>

namespace JE::Assets::RenderBuilders
{
    class JE_RENDER_ASSET_BUILDERS_API RenderAssetBuildersModule final : public AZ::Module
    {
    public:
        // Retain the original module RTTI name and UUID for existing module identity.
        AZ_RTTI_WITH_NAME(RenderAssetBuildersModule, "BuilderModule", "{CA15BD7F-01B4-4959-BEF2-81FA3AD2C834}", AZ::Module);

        RenderAssetBuildersModule();
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };

    JE_RENDER_ASSET_BUILDERS_API AZ::Module* CreateToolsModule();
    JE_RENDER_ASSET_BUILDERS_API AZ::Module* CreateShaderCompilerToolsModule();
    JE_RENDER_ASSET_BUILDERS_API AZ::Module* CreateSceneProcessingToolsModule();
} // namespace JE::Assets::RenderBuilders
