# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Host composition only: implementation libraries do not load the remnant Gem modules.
set(JE_RENDER_INTEGRATION_RUNTIME_DEPENDENCIES
    Gem::Atom_RHI.Private
    JE::WorldCamera
    Gem::LmbrCentral.Clients
    Gem::GradientSignal.Clients
    JE::Geometry
    Gem::EMotionFX_Atom
    Gem::AtomFont
    Gem::AtomViewportDisplayInfo
)
set(JE_RENDER_INTEGRATION_TOOL_DEPENDENCIES
    Gem::Atom_RHI.Private
    JE::RenderAssetBuilders
    JE::WorldCamera
    Gem::AtomRenderOptions.Editor
    Gem::LmbrCentral.Editor
    Gem::GradientSignal.Editor
    JE::Geometry.Authoring
    Gem::EMotionFX_Atom.Editor
    Gem::ImageProcessingAtom.Editor
    Gem::AtomFont
    JE::EditorTools
    Gem::AtomViewportDisplayInfo
    Gem::AtomViewportDisplayIcons.Editor
    Gem::ShaderManagementConsole.Builders
)

if(PAL_PLATFORM_NAME STREQUAL "Windows")
    list(APPEND JE_RENDER_INTEGRATION_RUNTIME_DEPENDENCIES
        Gem::Atom_RHI_Vulkan.Private)
elseif(PAL_PLATFORM_NAME STREQUAL "Linux")
    list(APPEND JE_RENDER_INTEGRATION_RUNTIME_DEPENDENCIES Gem::Atom_RHI_Vulkan.Private)
endif()

# Preserve host cross-cook dependencies for the explicitly enabled desktop tool platforms.
foreach(je_render_integration_tools_platform ${PAL_PLATFORM_NAME} ${LY_PAL_TOOLS_ENABLED})
    if(je_render_integration_tools_platform STREQUAL "Windows")
        list(APPEND JE_RENDER_INTEGRATION_TOOL_DEPENDENCIES
            Gem::Atom_RHI_Vulkan.Private Gem::Atom_RHI_Vulkan.Builders)
    elseif(je_render_integration_tools_platform STREQUAL "Linux")
        list(APPEND JE_RENDER_INTEGRATION_TOOL_DEPENDENCIES
            Gem::Atom_RHI_Vulkan.Private Gem::Atom_RHI_Vulkan.Builders)
    endif()
endforeach()
unset(je_render_integration_tools_platform)
list(REMOVE_DUPLICATES JE_RENDER_INTEGRATION_RUNTIME_DEPENDENCIES)
list(REMOVE_DUPLICATES JE_RENDER_INTEGRATION_TOOL_DEPENDENCIES)
