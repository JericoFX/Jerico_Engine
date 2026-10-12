/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <AzCore/Module/Module.h>
#include <JE/Editor/RenderIntegration/RenderIntegrationExports.h>

namespace JE::Editor::RenderIntegration
{
    JE_EDITOR_RENDER_INTEGRATION_API AZ::Module* CreateAuthoringModule();
}
