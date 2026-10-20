# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

set(tools_pal_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME EditorTools STATIC
    NAMESPACE JE
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        editor_tools_files.cmake
        ${tools_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE Source ${tools_pal_dir}
        PUBLIC Include
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Qt::Core
            3rdParty::Qt::Gui
            3rdParty::Qt::Network
            3rdParty::Qt::Widgets
            AZ::AzCore
            AZ::AzQtComponents
            AZ::AzToolsFramework
            JE::ProfilerView
            JE::RenderDebug
            JE::EditorRenderIntegration
            JE::RenderAssetBuilders
            JE::RenderIntegration.Public
            JE::WorldCamera
            JE::RenderFeatures
            Gem::Atom_RHI.Public
            Gem::Atom_RHI.Reflect
            JE::RenderIntegration
            Gem::GraphCanvasWidgets
            Gem::GraphModel.Editor.Static
            Gem::ImageProcessingAtom.Headers
    RUNTIME_DEPENDENCIES
        JE::RenderIntegration
        Gem::Atom_RHI.Private
        Gem::GraphCanvas.Editor
        Gem::GraphCanvasWidgets
        Gem::GraphModel.Editor
        Gem::ImageProcessingAtom.Editor
)

# Native tool hosts load these graph services independently of project Gem selection.
foreach(editor_tools_host IN ITEMS Editor MaterialCanvas MaterialEditor PassCanvas ShaderManagementConsole)
    if(TARGET ${editor_tools_host})
        ly_add_target_dependencies(
            TARGETS ${editor_tools_host}
            DEPENDENT_TARGETS Gem::GraphCanvas.Editor Gem::GraphModel.Editor
            GEM_VARIANT Tools
        )
    endif()
endforeach()
foreach(editor_tools_host IN ITEMS AssetProcessor AssetProcessorBatch AssetBuilder)
    if(TARGET ${editor_tools_host})
        ly_add_target_dependencies(
            TARGETS ${editor_tools_host}
            DEPENDENT_TARGETS Gem::GraphCanvas.Editor Gem::GraphModel.Editor
            GEM_VARIANT Builders
        )
    endif()
endforeach()
unset(editor_tools_host)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME EditorTools.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE editor_tools_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE . Tests
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Test
                AZ::AzFrameworkTestShared
                AZ::AzTest
                AZ::AzTestShared
                Gem::Atom_Utils.TestUtils.Static
                JE::EditorTools
    )
    set_target_properties(EditorTools.Tests PROPERTIES JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_googletest(NAME JE::EditorTools.Tests LABELS REQUIRES_tiaf)
endif()
