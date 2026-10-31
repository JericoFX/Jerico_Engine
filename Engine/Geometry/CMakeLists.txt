# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

# Native factories transfer modules to the hosts' existing lifecycle.
o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME}
    "${O3DE_ENGINE_RESTRICTED_PATH}" "${LY_ROOT_FOLDER}")
include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
if(NOT PAL_TRAIT_JE_GEOMETRY_SUPPORTED)
    return()
endif()

# Preserve the native scan-root layout in installed Editor/SDK distributions.
ly_install_directory(DIRECTORIES Assets)

# Surface contracts expose headers only; their implementations live in the existing Geometry image.
ly_add_target(
    NAME Geometry.SurfaceData.Public HEADERONLY
    NAMESPACE JE
    FILES_CMAKE surface_data_public_files.cmake
    INCLUDE_DIRECTORIES INTERFACE Include
    BUILD_DEPENDENCIES INTERFACE AZ::AzCore AZ::AzFramework Gem::LmbrCentral.API
)

ly_add_target(
    NAME Geometry ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE JE
    FILES_CMAKE geometry_runtime_files.cmake surface_data_runtime_files.cmake
    INCLUDE_DIRECTORIES PRIVATE Source PUBLIC Include
    COMPILE_DEFINITIONS PRIVATE JE_GEOMETRY_EXPORTS
    BUILD_DEPENDENCIES PUBLIC
        AZ::AzCore
        JE::RenderIntegration
        JE::RenderFeatures
        JE::RenderScene.Public
        JE::RenderMaterials
        Legacy::CryCommon
        JE::Geometry.SurfaceData.Public
        PRIVATE Gem::LmbrCentral
)

# Verify the absorbed public/private include boundaries without unity-build masking.
set_target_properties(Geometry PROPERTIES UNITY_BUILD OFF)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    o3de_register_3rdparty_root("${CMAKE_CURRENT_LIST_DIR}/3rdParty")
    ly_add_target(
        NAME Geometry.Authoring ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
        NAMESPACE JE
        FILES_CMAKE geometry_authoring_files.cmake surface_data_authoring_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source Source/SurfaceData PUBLIC Include
        COMPILE_DEFINITIONS
            PRIVATE _USE_MATH_DEFINES OM_STATIC_BUILD JE_GEOMETRY_AUTHORING_EXPORTS
            PUBLIC JE_GEOMETRY_AUTHORING
        BUILD_DEPENDENCIES
            PUBLIC AZ::AzToolsFramework Legacy::CryCommon Legacy::Editor.Headers JE::Geometry
            PRIVATE 3rdParty::OpenMesh 3rdParty::Manifold Gem::LmbrCentral.Editor.Static
    )
    set_target_properties(Geometry.Authoring PROPERTIES UNITY_BUILD OFF)
    target_compile_options(Geometry.Authoring ${O3DE_COMPILE_OPTION_DISABLE_DEPRECATED_ENUM_ENUM_CONVERSION})
endif()

if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # Private mocks and the historical environment remain outside installed product headers.
    ly_add_target(
        NAME Geometry.SurfaceData.TestUtils HEADERONLY
        NAMESPACE JE
        FILES_CMAKE surface_data_testutils_files.cmake
        INCLUDE_DIRECTORIES INTERFACE Tests
        BUILD_DEPENDENCIES INTERFACE AZ::AzTest JE::Geometry
    )
    set_target_properties(Geometry.SurfaceData.TestUtils PROPERTIES JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_target(
        NAME Geometry.SurfaceData.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE surface_data_tests_files.cmake surface_data_benchmarks_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source Tests Tests/SurfaceData
        BUILD_DEPENDENCIES PRIVATE
            AZ::AzTest AZ::AzTestShared JE::Geometry JE::Geometry.SurfaceData.TestUtils Gem::LmbrCentral
        RUNTIME_DEPENDENCIES Gem::LmbrCentral
    )
    set_target_properties(Geometry.SurfaceData.Tests PROPERTIES UNITY_BUILD OFF JE_EXCLUDE_FROM_SDK TRUE)
    ly_add_googletest(NAME JE::Geometry.SurfaceData.Tests LABELS REQUIRES_tiaf)
    ly_add_googlebenchmark(NAME JE::Geometry.SurfaceData.Benchmarks TARGET JE::Geometry.SurfaceData.Tests)

    ly_add_target(
        NAME Geometry.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE JE
        FILES_CMAKE geometry_runtime_tests_files.cmake
        INCLUDE_DIRECTORIES PRIVATE Source Tests
        BUILD_DEPENDENCIES PRIVATE AZ::AzTestShared AZ::AzTest JE::Geometry
    )
    ly_add_googletest(NAME JE::Geometry.Tests LABELS REQUIRES_tiaf)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # A separate hook/application owns real factories; never combine it with GemTestEnvironment's descriptors.
        ly_add_target(
            NAME Geometry.SurfaceData.Native.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE JE
            FILES_CMAKE surface_data_authoring_tests_files.cmake
            BUILD_DEPENDENCIES PRIVATE
                AZ::AzTest AZ::AzTestShared AZ::AzToolsFramework JE::Geometry JE::Geometry.Authoring
        )
        set_target_properties(Geometry.SurfaceData.Native.Tests PROPERTIES UNITY_BUILD OFF JE_EXCLUDE_FROM_SDK TRUE)
        ly_add_googletest(NAME JE::Geometry.SurfaceData.Native.Tests)

        ly_add_target(
            NAME Geometry.Authoring.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE JE
            FILES_CMAKE geometry_authoring_tests_files.cmake
            INCLUDE_DIRECTORIES PRIVATE Source Tests
            BUILD_DEPENDENCIES PRIVATE
                AZ::AzTestShared AZ::AzManipulatorTestFramework.Static JE::Geometry.Authoring
        )
        ly_add_googletest(NAME JE::Geometry.Authoring.Tests)
        ly_add_target(
            NAME Geometry.Authoring.Physics.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE JE
            FILES_CMAKE geometry_physics_tests_files.cmake
            INCLUDE_DIRECTORIES PRIVATE Source Tests
            BUILD_DEPENDENCIES PRIVATE
                AZ::AzTestShared AZ::AzManipulatorTestFramework.Static JE::Geometry.Authoring
            RUNTIME_DEPENDENCIES Gem::PhysX5.Editor
        )
        ly_add_googletest(NAME JE::Geometry.Authoring.Physics.Tests)
    endif()
endif()