#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_RUNTIME_DEPENDENCIES
    Gem::Atom_RHI_Metal.Private
    Gem::Atom_RHI_Vulkan.Private
    Gem::Atom_RHI_DX12.Private
)

# Cross-cook metadata does not activate additional backends in desktop product hosts.
if(PAL_PLATFORM_NAME STREQUAL "Windows" OR PAL_PLATFORM_NAME STREQUAL "Linux")
    list(REMOVE_ITEM LY_RUNTIME_DEPENDENCIES
        Gem::Atom_RHI_DX12.Private Gem::Atom_RHI_Metal.Private)
endif()
