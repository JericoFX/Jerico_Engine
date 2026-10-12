#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Transitional discovery for the remaining renderer Gems; integration is composed natively.
include(${LY_ROOT_FOLDER}/Engine/Render/Integration/render_integration_dependencies.cmake)
ly_create_alias(NAME AtomLyIntegration.Clients NAMESPACE Gem TARGETS JE::RenderIntegration ${JE_RENDER_INTEGRATION_RUNTIME_DEPENDENCIES})
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME AtomLyIntegration.Builders NAMESPACE Gem TARGETS JE::EditorRenderIntegration ${JE_RENDER_INTEGRATION_TOOL_DEPENDENCIES})
    ly_create_alias(NAME AtomLyIntegration.Tools    NAMESPACE Gem TARGETS JE::EditorRenderIntegration ${JE_RENDER_INTEGRATION_TOOL_DEPENDENCIES})
endif()
