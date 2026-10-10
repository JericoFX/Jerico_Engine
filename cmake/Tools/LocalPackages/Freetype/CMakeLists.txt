# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Adapted from o3de/3p-package-source at 70f0b7b5257bf7cf935bf1cfd270e1b53ccead18.

cmake_minimum_required(VERSION 3.24)

set(JE_FREETYPE_SOURCE_WORK "" CACHE PATH "Audited FreeType 2.11.1 source COPY in local build outputs")
set(JE_ZLIB_PACKAGE_ROOT "" CACHE PATH "Extracted audited local zlib 1.2.11 package root")
set(JE_FREETYPE_STAGE_DIR "" CACHE PATH "New local package payload staging directory")
get_filename_component(je_engine_root "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(je_build_root "${je_engine_root}/build")
foreach(je_path IN ITEMS JE_FREETYPE_SOURCE_WORK JE_ZLIB_PACKAGE_ROOT JE_FREETYPE_STAGE_DIR CMAKE_BINARY_DIR)
    if("${${je_path}}" MATCHES "://")
        message(FATAL_ERROR "${je_path} must not be a URI")
    endif()
    file(TO_CMAKE_PATH "${${je_path}}" je_local_path)
    if(NOT IS_ABSOLUTE "${je_local_path}" OR je_local_path MATCHES "^//")
        message(FATAL_ERROR "${je_path} must be an explicit absolute local path")
    endif()
    cmake_path(NORMAL_PATH je_local_path)
    set(${je_path} "${je_local_path}")
endforeach()
foreach(je_path IN ITEMS JE_FREETYPE_SOURCE_WORK JE_FREETYPE_STAGE_DIR CMAKE_BINARY_DIR)
    cmake_path(IS_PREFIX je_build_root "${${je_path}}" NORMALIZE je_inside_build)
    if(NOT je_inside_build OR "${${je_path}}" STREQUAL je_build_root)
        message(FATAL_ERROR "${je_path} must be beneath this checkout/build; external immutable snapshots are not work directories")
    endif()
endforeach()
foreach(je_path IN ITEMS JE_FREETYPE_SOURCE_WORK JE_ZLIB_PACKAGE_ROOT JE_FREETYPE_STAGE_DIR CMAKE_BINARY_DIR)
    set(je_ancestor "${${je_path}}")
    while(TRUE)
        if(IS_SYMLINK "${je_ancestor}")
            message(FATAL_ERROR "${je_path} must not use symlinks or linked ancestors")
        endif()
        get_filename_component(je_parent "${je_ancestor}" DIRECTORY)
        if(je_parent STREQUAL je_ancestor)
            break()
        endif()
        set(je_ancestor "${je_parent}")
    endwhile()
endforeach()
project(JE_LocalFreetype LANGUAGES C)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR
   NOT CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022" OR NOT CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    message(FATAL_ERROR "This reconstructed package recipe requires Visual Studio 2022 Windows x64")
endif()
if(EXISTS "${JE_FREETYPE_STAGE_DIR}")
    message(FATAL_ERROR "JE_FREETYPE_STAGE_DIR must be new; preserve existing package outputs")
endif()
foreach(je_source_file IN ITEMS CMakeLists.txt include/freetype/freetype.h LICENSE.TXT docs/FTL.TXT docs/GPLv2.TXT
    src/bdf/README src/pcf/README src/base/fthash.c include/freetype/internal/fthash.h)
    if(NOT EXISTS "${JE_FREETYPE_SOURCE_WORK}/${je_source_file}")
        message(FATAL_ERROR "Missing audited FreeType source file: ${je_source_file}")
    endif()
endforeach()
file(READ "${JE_FREETYPE_SOURCE_WORK}/include/freetype/freetype.h" je_freetype_header)
if(NOT je_freetype_header MATCHES "#define[ \t]+FREETYPE_MAJOR[ \t]+2([\r\n]|$)" OR
   NOT je_freetype_header MATCHES "#define[ \t]+FREETYPE_MINOR[ \t]+11([\r\n]|$)" OR
   NOT je_freetype_header MATCHES "#define[ \t]+FREETYPE_PATCH[ \t]+1([\r\n]|$)")
    message(FATAL_ERROR "The explicit source copy must be FreeType 2.11.1")
endif()
if(NOT EXISTS "${JE_ZLIB_PACKAGE_ROOT}/FindZLIB.cmake")
    message(FATAL_ERROR "JE_ZLIB_PACKAGE_ROOT must contain the audited local FindZLIB.cmake")
endif()
list(PREPEND CMAKE_MODULE_PATH "${JE_ZLIB_PACKAGE_ROOT}")
find_package(ZLIB 1.2.11 EXACT REQUIRED MODULE)

# Same feature set as the original recipe; zlib cannot silently fall back to the internal copy.
set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Audited package configuration" FORCE)
set(CMAKE_CXX_STANDARD 17)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Original static package recipe" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Original PIC package recipe" FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB OFF CACHE BOOL "Original zlib support" FORCE)
set(FT_DISABLE_ZLIB OFF CACHE BOOL "Use audited external zlib" FORCE)
set(FT_REQUIRE_ZLIB ON CACHE BOOL "Require audited external zlib" FORCE)
foreach(je_optional IN ITEMS BZip2 PNG HarfBuzz BrotliDec)
    set(CMAKE_DISABLE_FIND_PACKAGE_${je_optional} ON CACHE BOOL "Disabled in the original recipe" FORCE)
endforeach()
set(SKIP_INSTALL_ALL ON CACHE BOOL "Install only the original package payload formats" FORCE)
set(CMAKE_INSTALL_PREFIX "${JE_FREETYPE_STAGE_DIR}" CACHE PATH "New local package payload stage" FORCE)
add_subdirectory("${JE_FREETYPE_SOURCE_WORK}" "${CMAKE_CURRENT_BINARY_DIR}/freetype")
file(READ "${CMAKE_CURRENT_BINARY_DIR}/freetype/include/freetype/config/ftoption.h" je_freetype_options)
if(NOT je_freetype_options MATCHES "(^|[\r\n])[ \t]*#define[ \t]+FT_CONFIG_OPTION_SYSTEM_ZLIB")
    message(FATAL_ERROR "FreeType must compile with the explicit external zlib package")
endif()

install(TARGETS freetype ARCHIVE DESTINATION freetype/lib CONFIGURATIONS Release)
install(DIRECTORY "${JE_FREETYPE_SOURCE_WORK}/include/" DESTINATION freetype/include/freetype2
    FILES_MATCHING PATTERN "*.h" PATTERN internal EXCLUDE PATTERN ftconfig.h EXCLUDE PATTERN ftoption.h EXCLUDE)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/freetype/include/freetype/config/ftconfig.h"
    "${CMAKE_CURRENT_BINARY_DIR}/freetype/include/freetype/config/ftoption.h"
    DESTINATION freetype/include/freetype2/freetype/config)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/FindFreetype.cmake" DESTINATION .)
install(FILES "${JE_FREETYPE_SOURCE_WORK}/LICENSE.TXT" "${JE_FREETYPE_SOURCE_WORK}/docs/FTL.TXT"
    "${JE_FREETYPE_SOURCE_WORK}/docs/GPLv2.TXT" "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt"
    DESTINATION LICENSES/Freetype)
install(FILES "${JE_FREETYPE_SOURCE_WORK}/src/bdf/README"
    DESTINATION LICENSES/Freetype RENAME BDF-README.txt)
install(FILES "${JE_FREETYPE_SOURCE_WORK}/src/pcf/README"
    DESTINATION LICENSES/Freetype RENAME PCF-README.txt)
# Preserve the complete original notices in the hash utility source/header compiled into ftbase.
install(FILES "${JE_FREETYPE_SOURCE_WORK}/src/base/fthash.c"
    "${JE_FREETYPE_SOURCE_WORK}/include/freetype/internal/fthash.h" DESTINATION LICENSES/Freetype/fthash)
install(FILES "${je_engine_root}/LICENSE_APACHE2.TXT" "${je_engine_root}/LICENSE_MIT.TXT"
    DESTINATION LICENSES/O3DE)
