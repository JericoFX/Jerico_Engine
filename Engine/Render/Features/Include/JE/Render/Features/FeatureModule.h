// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once
#include <JE/Render/Features/RenderFeaturesExports.h>
#include <AzCore/Module/Module.h>

namespace JE::Render::Features
{
    //! Ownership is transferred to the application ModuleManager.
    JE_RENDER_FEATURES_API AZ::Module* CreateModule();
}
