# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_install_directory(DIRECTORIES Assets)

# Public contracts do not link back to the component implementations.
ly_add_target(
    NAME RenderScene.Public HEADERONLY
    NAMESPACE JE
    FILES_CMAKE scene_public_files.cmake
    INCLUDE_DIRECTORIES INTERFACE Include
    BUILD_DEPENDENCIES INTERFACE
        Gem::Atom_Feature_Common.Public JE::RenderMaterials JE::RenderDebug.Public
)

ly_add_target(
    NAME RenderScene STATIC
    NAMESPACE JE
    FILES_CMAKE scene_files.cmake
    INCLUDE_DIRECTORIES PRIVATE . Source PUBLIC Include
    BUILD_DEPENDENCIES PUBLIC
        AZ::AzCore AZ::AzFramework JE::RenderScene.Public
        Gem::LmbrCentral.Static Gem::GradientSignal.Static Gem::SurfaceData.Static
        Gem::Atom_Feature_Common.Public JE::RenderMaterials
        JE::RenderIntegration.Public JE::RenderDebug
)
set_target_properties(RenderScene PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    add_subdirectory(${LY_ROOT_FOLDER}/Editor/Render/Scene ${CMAKE_CURRENT_BINARY_DIR}/Editor)
endif()
