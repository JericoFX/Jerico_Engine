#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(gem_path ${CMAKE_CURRENT_LIST_DIR})
set(gem_json ${gem_path}/gem.json)
o3de_restricted_path(${gem_json} gem_restricted_path gem_parent_relative_path)

add_subdirectory(Tools)
add_subdirectory(Utils)

# Transitional discovery for the remaining renderer Gems; integration is composed natively.
include(${LY_ROOT_FOLDER}/Engine/Render/Integration/render_integration_dependencies.cmake)
ly_create_alias(NAME Atom.Clients NAMESPACE Gem TARGETS JE::RenderIntegration ${JE_RENDER_INTEGRATION_RUNTIME_DEPENDENCIES})
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME Atom.Builders NAMESPACE Gem
        TARGETS JE::EditorRenderIntegration ${JE_RENDER_INTEGRATION_TOOL_DEPENDENCIES}
            Gem::Atom_RHI_Null.Private Gem::Atom_RHI_Null.Builders)
    ly_create_alias(NAME Atom.Tools    NAMESPACE Gem TARGETS JE::EditorRenderIntegration ${JE_RENDER_INTEGRATION_TOOL_DEPENDENCIES})
endif()
