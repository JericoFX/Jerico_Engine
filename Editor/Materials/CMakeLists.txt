# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Native material authoring owns its node templates and viewport preview assets.
ly_install_directory(DIRECTORIES Assets)
ly_install_directory(DIRECTORIES Preview/Assets)
ly_install_directory(DIRECTORIES RenderPass/Assets)

ly_add_target(
    NAME EditorMaterials ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    AUTOMOC
    AUTORCC
    FILES_CMAKE materials_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_EDITOR_MATERIALS_EXPORTS
    BUILD_DEPENDENCIES
        PUBLIC
            JE::RenderMaterials
            AZ::AzToolsFramework
            JE::EditorRenderIntegration
            JE::EditorRenderFeatures
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            JE::RenderScene.Public
            Legacy::Editor.Headers
            Legacy::CryCommon
        PRIVATE
            JE::RenderDebug
            Gem::ImageProcessingAtom.Headers
)

set_target_properties(EditorMaterials PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME EditorMaterials.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        AUTOMOC
        FILES_CMAKE materials_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source ${LY_ROOT_FOLDER}/Code/Framework/AzToolsFramework
        BUILD_DEPENDENCIES
            PRIVATE JE::EditorMaterials AZ::AzToolsFramework AZ::AzTest AZ::AzTestShared
                Gem::ImageProcessingAtom.Headers
        RUNTIME_DEPENDENCIES
            Gem::GraphCanvas.Editor
            Gem::GraphModel.Editor
    )
    set_target_properties(EditorMaterials.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::EditorMaterials.Tests)
    set_property(TEST JE::EditorMaterials.Tests.main::TEST_RUN APPEND PROPERTY ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(TARGET Qt6::QOffscreenIntegrationPlugin)
        set_property(TEST JE::EditorMaterials.Tests.main::TEST_RUN APPEND PROPERTY ENVIRONMENT
            "QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::QOffscreenIntegrationPlugin>")
    endif()
endif()
