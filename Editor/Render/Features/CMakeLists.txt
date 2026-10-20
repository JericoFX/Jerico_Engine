# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_install_directory(DIRECTORIES Assets)

set(features_pal_dir ${LY_ROOT_FOLDER}/Engine/Render/Features/Source/Platform/${PAL_PLATFORM_NAME})
set(features_runtime_tools ${features_pal_dir}/runtime_dependencies_tools.cmake)
foreach(features_tools_platform ${LY_PAL_TOOLS_ENABLED})
    ly_get_list_relative_pal_filename(features_tools_pal_dir ${LY_ROOT_FOLDER}/Engine/Render/Features/Source/Platform/${features_tools_platform})
    list(APPEND features_runtime_tools ${features_tools_pal_dir}/runtime_dependencies_tools.cmake)
endforeach()
# Preserve the retired descriptor-free builder module's platform deployment edges.
if(EXISTS "${features_pal_dir}/runtime_dependencies_builders.cmake")
    list(APPEND features_runtime_tools ${features_pal_dir}/runtime_dependencies_builders.cmake)
endif()
ly_add_target(
    NAME EditorRenderFeatures STATIC
    NAMESPACE JE
    FILES_CMAKE features_files.cmake features_diffuse_probe_grid_files.cmake features_editor_mode_feedback_files.cmake
    PLATFORM_INCLUDE_FILES ${features_runtime_tools}
    INCLUDE_DIRECTORIES PRIVATE Source Source/EditorModeFeedback ${features_pal_dir} ${LY_ROOT_FOLDER}/Engine/Render/Features/Source
        Source/DiffuseProbeGrid ${LY_ROOT_FOLDER}/Engine/Render/Features/Source/DiffuseProbeGrid
        PUBLIC Include
    BUILD_DEPENDENCIES PUBLIC JE::RenderFeatures JE::EditorRenderIntegration
        AZ::AzToolsFramework AZ::AssetBuilderSDK AZ::SceneCore
        PRIVATE Gem::LmbrCentral.API JE::RenderScene.Public
    RUNTIME_DEPENDENCIES Gem::Atom_RHI.Private
)
set_target_properties(EditorRenderFeatures PROPERTIES UNITY_BUILD OFF)
