#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(PAL_TRAIT_PHYSX_SUPPORTED TRUE)

ly_associate_package(PACKAGE_NAME PhysX-5.1.1-je-cpu-rev2-windows TARGETS PhysX5 PACKAGE_HASH 6a3655fa52ad06691f1b360b650557518c06494ac0e4ea7418158b7b63f4fc62)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
     ly_associate_package(PACKAGE_NAME poly2tri-7f0487a-je-rev1-windows TARGETS poly2tri PACKAGE_HASH f00af81d5124edb6f09b4a2de7a5c2855fa6d44f19fa7ac976ecb70fc79252a6)
endif()
