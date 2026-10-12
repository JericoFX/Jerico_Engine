# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME PrefabBuilder ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE prefabbuilder_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_PREFAB_BUILDER_EXPORTS
    BUILD_DEPENDENCIES PUBLIC
        AZ::AzCore AZ::AzToolsFramework AZ::AssetBuilderSDK
        AZ::SceneCore AZ::SceneData 3rdParty::RapidJSON
)
set_target_properties(PrefabBuilder PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME PrefabBuilder.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE prefabbuilder_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Tests
        BUILD_DEPENDENCIES PRIVATE
            AZ::AzTest AZ::AzToolsFrameworkTestCommon JE::PrefabBuilder
    )
    set_target_properties(PrefabBuilder.Tests PROPERTIES UNITY_BUILD OFF)
    ly_add_googletest(NAME JE::PrefabBuilder.Tests LABELS REQUIRES_tiaf)
endif()
