# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

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
    FILES_CMAKE features_files.cmake
    PLATFORM_INCLUDE_FILES ${features_runtime_tools}
    INCLUDE_DIRECTORIES PRIVATE Source ${features_pal_dir} ${LY_ROOT_FOLDER}/Engine/Render/Features/Source
        PUBLIC Include
    BUILD_DEPENDENCIES PUBLIC JE::RenderFeatures JE::EditorRenderIntegration
        AZ::AzToolsFramework AZ::AssetBuilderSDK AZ::SceneCore
    RUNTIME_DEPENDENCIES Gem::Atom_RHI.Private
)
set_target_properties(EditorRenderFeatures PROPERTIES UNITY_BUILD OFF)
