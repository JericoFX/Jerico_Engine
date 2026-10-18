# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_add_target(
    NAME RenderMaterials ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE materials_files.cmake
    INCLUDE_DIRECTORIES PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_RENDER_MATERIALS_EXPORTS
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            JE::RenderIntegration
            JE::RenderFeatures
)

set_target_properties(RenderMaterials PROPERTIES UNITY_BUILD OFF)
