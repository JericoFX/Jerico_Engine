/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <JE/Assets/RenderBuilders/RenderAssetBuildersExports.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace JE::Assets::RenderBuilders::SceneProcessing
{
    // The state belongs to the existing owner image, including callers from test OBJECT files.
    JE_RENDER_ASSET_BUILDERS_API AZStd::unique_ptr<AZ::DynamicModuleHandle>& GetSceneCoreModule();
    JE_RENDER_ASSET_BUILDERS_API AZStd::unique_ptr<AZ::DynamicModuleHandle>& GetSceneDataModule();
    JE_RENDER_ASSET_BUILDERS_API AZStd::unique_ptr<AZ::DynamicModuleHandle>& GetSceneBuilderModule();
} // namespace JE::Assets::RenderBuilders::SceneProcessing
