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
        graphmodel_files.cmake
        graphcanvas_files.cmake
        ${tools_pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE Source Source/GraphModel Source/GraphCanvas ${tools_pal_dir}
        PUBLIC Include
    COMPILE_DEFINITIONS
        PUBLIC GRAPH_CANVAS_ENABLE_DETAILED_PROFILING=0
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
            Gem::ImageProcessingAtom.Headers
        PRIVATE
            AZ::AzFramework
            AZ::AssetBuilderSDK
    RUNTIME_DEPENDENCIES
        3rdParty::Qt::Xml
        JE::RenderIntegration
        Gem::Atom_RHI.Private
        Gem::ImageProcessingAtom.Editor
)

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

    # GraphModel retains its own existing test environment and unit-test hook.
    ly_add_target(
        NAME EditorGraphModel.Tests MODULE
        NAMESPACE JE
        FILES_CMAKE graphmodel_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                AZ::AzToolsFramework
                JE::EditorTools
    )
    set_target_properties(EditorGraphModel.Tests PROPERTIES JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_googletest(NAME JE::EditorGraphModel.Tests LABELS REQUIRES_tiaf)
    # Retain the existing GraphCanvas test environment and unit-test hook.
    ly_add_target(
        NAME EditorGraphCanvas.Tests MODULE
        NAMESPACE JE
        FILES_CMAKE graphcanvas_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source Source/GraphCanvas Tests/GraphCanvas
        BUILD_DEPENDENCIES PRIVATE AZ::AzTest AZ::AssetBuilderSDK JE::EditorTools
    )
    set_target_properties(EditorGraphCanvas.Tests PROPERTIES JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_googletest(NAME JE::EditorGraphCanvas.Tests LABELS REQUIRES_tiaf)
endif()
