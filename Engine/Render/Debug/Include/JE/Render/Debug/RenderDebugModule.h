/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <JE/Render/Debug/RenderDebugExports.h>
#include <AzCore/Module/Module.h>

namespace JE::Render::Debug
{
    JE_RENDER_DEBUG_API AZ::Module* CreateViewportModule();
    JE_RENDER_DEBUG_API AZ::Module* CreateToolsModule();
}
