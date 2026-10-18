# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

set(scene_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
ly_add_target(
    NAME EditorRenderScene STATIC
    NAMESPACE JE
    AUTOUIC AUTOMOC AUTORCC
    FILES_CMAKE scene_files.cmake ${scene_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES ${scene_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE . Source ${LY_ROOT_FOLDER}/Engine/Render/Scene/Source ${LY_ROOT_FOLDER}/Engine/Render/Scene
        PUBLIC Include
    BUILD_DEPENDENCIES PUBLIC
        JE::RenderScene JE::EditorRenderFeatures JE::EditorRenderIntegration JE::EditorMaterials
        Gem::AtomToolsFramework.Static Gem::AtomToolsFramework.Editor
        AZ::SceneCore AZ::SceneData Legacy::Editor.Headers Legacy::CryCommon
    RUNTIME_DEPENDENCIES 3rdParty::OpenImageIO::Tools::Binaries
)
set_target_properties(EditorRenderScene PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME EditorRenderScene.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE scene_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Tests Source ${LY_ROOT_FOLDER}/Engine/Render/Scene/Source
        BUILD_DEPENDENCIES PRIVATE
            AZ::AzTest AZ::AzTestShared AZ::AzToolsFramework AZ::AzToolsFrameworkTestCommon
            JE::RenderScene JE::EditorRenderScene
        RUNTIME_DEPENDENCIES Gem::LmbrCentral.Editor
    )
    set_target_properties(EditorRenderScene.Tests PROPERTIES UNITY_BUILD OFF JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_googletest(NAME JE::EditorRenderScene.Tests LABELS REQUIRES_tiaf)
endif()
