# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

ly_add_target(
    NAME EditorMaterials ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    AUTOMOC
    FILES_CMAKE materials_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_EDITOR_MATERIALS_EXPORTS
    BUILD_DEPENDENCIES
        PUBLIC
            JE::RenderMaterials
            AZ::AzToolsFramework
            Gem::Atom_RPI.Edit
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::CommonFeaturesAtom.Public
            Legacy::Editor.Headers
            Legacy::CryCommon
        PRIVATE
            JE::RenderDebug
)

set_target_properties(EditorMaterials PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME EditorMaterials.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        AUTOMOC
        FILES_CMAKE materials_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source
        BUILD_DEPENDENCIES
            PRIVATE JE::EditorMaterials AZ::AzToolsFramework AZ::AzTest AZ::AzTestShared
    )
    set_target_properties(EditorMaterials.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::EditorMaterials.Tests)
endif()
