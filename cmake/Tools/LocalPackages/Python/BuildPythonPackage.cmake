# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Adapted from o3de/3p-package-source@70f0b7b5257bf7cf935bf1cfd270e1b53ccead18,
# package-system/python/win_x64/build_python.bat. Source/work copies are provisioned
# separately; this script does not acquire dependencies or install globally.
cmake_minimum_required(VERSION 3.24)
include("${CMAKE_CURRENT_LIST_DIR}/JERequireLocalPath.cmake")
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "JE local Python recipe requires Windows x64 and Visual Studio 2022 v143")
endif()
foreach(JE_ARGUMENT IN ITEMS SOURCE_WORK SOURCE_ARCHIVE SOURCE_SHA256 EXPAT_WORK EXPAT_ARCHIVE
    EXPAT_SHA256 EXTERNALS_DIR EXTERNALS_MANIFEST BOOTSTRAP BOOTSTRAP_SHA256 MSBUILD MSBUILD_SHA256 OUTPUT)
    if(NOT DEFINED JE_PYTHON_${JE_ARGUMENT} OR "${JE_PYTHON_${JE_ARGUMENT}}" STREQUAL "")
        message(FATAL_ERROR "JE local Python: JE_PYTHON_${JE_ARGUMENT} is required")
    endif()
endforeach()
foreach(JE_PATH_VAR IN ITEMS JE_PYTHON_SOURCE_WORK JE_PYTHON_SOURCE_ARCHIVE JE_PYTHON_EXPAT_WORK
    JE_PYTHON_EXPAT_ARCHIVE JE_PYTHON_EXTERNALS_DIR JE_PYTHON_EXTERNALS_MANIFEST
    JE_PYTHON_BOOTSTRAP JE_PYTHON_MSBUILD JE_PYTHON_OUTPUT)
    JE_python_require_local_path(${JE_PATH_VAR})
endforeach()
get_filename_component(JE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(JE_BUILD_ROOT "${JE_ENGINE_ROOT}/build")
foreach(JE_WORK_VAR IN ITEMS JE_PYTHON_SOURCE_WORK JE_PYTHON_EXPAT_WORK JE_PYTHON_EXTERNALS_DIR JE_PYTHON_OUTPUT)
    cmake_path(IS_PREFIX JE_BUILD_ROOT "${${JE_WORK_VAR}}" NORMALIZE JE_IN_BUILD)
    if(NOT JE_IN_BUILD OR "${${JE_WORK_VAR}}" STREQUAL "${JE_BUILD_ROOT}")
        message(FATAL_ERROR "JE local Python: ${JE_WORK_VAR} must be beneath this checkout/build")
    endif()
endforeach()
if(EXISTS "${JE_PYTHON_OUTPUT}")
    message(FATAL_ERROR "JE local Python: output already exists: ${JE_PYTHON_OUTPUT}")
endif()
foreach(JE_SOURCE_DIR IN ITEMS "${JE_PYTHON_SOURCE_WORK}" "${JE_PYTHON_EXPAT_WORK}" "${JE_PYTHON_EXTERNALS_DIR}")
    if(NOT IS_DIRECTORY "${JE_SOURCE_DIR}")
        message(FATAL_ERROR "JE local Python: prepared source/work directory is missing: ${JE_SOURCE_DIR}")
    endif()
    cmake_path(IS_PREFIX JE_SOURCE_DIR "${JE_PYTHON_OUTPUT}" NORMALIZE JE_OUTPUT_IN_SOURCE)
    cmake_path(IS_PREFIX JE_PYTHON_OUTPUT "${JE_SOURCE_DIR}" NORMALIZE JE_SOURCE_IN_OUTPUT)
    if(JE_OUTPUT_IN_SOURCE OR JE_SOURCE_IN_OUTPUT)
        message(FATAL_ERROR "JE local Python: source/work and output directories must be separate")
    endif()
    file(GLOB_RECURSE JE_ENTRIES LIST_DIRECTORIES TRUE "${JE_SOURCE_DIR}/*")
    foreach(JE_ENTRY IN LISTS JE_ENTRIES)
        if(IS_SYMLINK "${JE_ENTRY}")
            message(FATAL_ERROR "JE local Python: source/work symlink rejected: ${JE_ENTRY}")
        endif()
    endforeach()
endforeach()
JE_python_verify_file("${JE_PYTHON_SOURCE_ARCHIVE}" "${JE_PYTHON_SOURCE_SHA256}" "CPython source archive")
JE_python_verify_file("${JE_PYTHON_EXPAT_ARCHIVE}" "${JE_PYTHON_EXPAT_SHA256}" "Expat source archive")
JE_python_verify_file("${JE_PYTHON_BOOTSTRAP}" "${JE_PYTHON_BOOTSTRAP_SHA256}" "Python 3.12 bootstrap")
JE_python_verify_file("${JE_PYTHON_MSBUILD}" "${JE_PYTHON_MSBUILD_SHA256}" "MSBuild")
if(NOT EXISTS "${JE_PYTHON_EXTERNALS_MANIFEST}" OR IS_DIRECTORY "${JE_PYTHON_EXTERNALS_MANIFEST}")
    message(FATAL_ERROR "JE local Python: external archive receipts are missing")
endif()
foreach(JE_REQUIRED IN ITEMS PCbuild/pcbuild.proj PCbuild/pythoncore.vcxproj PCbuild/regen.targets PC/layout/main.py
    Include/patchlevel.h LICENSE PC/crtlicense.txt Lib/ensurepip/_bundled/pip-23.0.1-py3-none-any.whl
    Lib/ensurepip/_bundled/setuptools-65.5.0-py3-none-any.whl)
    if(NOT EXISTS "${JE_PYTHON_SOURCE_WORK}/${JE_REQUIRED}")
        message(FATAL_ERROR "JE local Python: required source file is missing: ${JE_REQUIRED}")
    endif()
endforeach()
if(EXISTS "${JE_PYTHON_SOURCE_WORK}/PCbuild/amd64" OR EXISTS "${JE_PYTHON_SOURCE_WORK}/PCbuild/obj")
    message(FATAL_ERROR "JE local Python: source work already contains build outputs; provision a fresh copy")
endif()
file(STRINGS "${JE_PYTHON_SOURCE_WORK}/Include/patchlevel.h" JE_PY_VERSION_LINE REGEX "^#define PY_VERSION ")
if(NOT JE_PY_VERSION_LINE MATCHES "^#define PY_VERSION +\"3[.]10[.]13\"$")
    message(FATAL_ERROR "JE local Python: explicit source work must be CPython 3.10.13")
endif()
set(JE_EXPAT_LIB "${JE_PYTHON_EXPAT_WORK}/expat/lib")
if(NOT EXISTS "${JE_EXPAT_LIB}/expat.h" OR NOT EXISTS "${JE_PYTHON_EXPAT_WORK}/expat/COPYING")
    message(FATAL_ERROR "JE local Python: original Expat 2.4.6 overlay and license are missing")
endif()
file(STRINGS "${JE_EXPAT_LIB}/expat.h" JE_EXPAT_VERSION_LINES REGEX "^#define XML_(MAJOR|MINOR|MICRO)_VERSION ")
foreach(JE_VERSION_LINE IN ITEMS "#define XML_MAJOR_VERSION 2" "#define XML_MINOR_VERSION 4" "#define XML_MICRO_VERSION 6")
    if(NOT JE_VERSION_LINE IN_LIST JE_EXPAT_VERSION_LINES)
        message(FATAL_ERROR "JE local Python: explicit Expat overlay must be 2.4.6")
    endif()
endforeach()

# Manifest is a JSON array of {name,commit,sha256,archive,url}. Archives are local;
# URLs are provenance only and must match the reviewed immutable codeload origins.
set(JE_EXTERNAL_NAMES bzip2-1.0.8 sqlite-3.40.1.0 xz-5.2.5 zlib-1.2.13
    libffi-3.3.0 openssl-bin-1.1.1u tcltk-8.6.12.0)
set(JE_EXTERNAL_COMMITS 05301997b2f9590f49c672cf3dfd3d3dfa7ad521 db4ca556b25787c263ee72a4b764c4be474c322f
    c6bc0c612605622aaef101a33a751f9de2ecc193 9071e5ae1bd90c30518404b95ec0be0d4cf6cf84
    1cf06233e3ceb49dc0a73c55e04b1174b436b632 bbd07240c6414fa314ca606b73247054eb81b59c
    e3c3e9a2856124aa32b608632a52742d479eb7a9)
file(READ "${JE_PYTHON_EXTERNALS_MANIFEST}" JE_RECEIPTS)
string(JSON JE_COUNT LENGTH "${JE_RECEIPTS}")
if(NOT JE_COUNT EQUAL 7)
    message(FATAL_ERROR "JE local Python: exactly seven original external archive receipts are required")
endif()
set(JE_COMPONENTS "[]")
set(JE_SEEN_NAMES "")
foreach(JE_INDEX RANGE 0 6)
    foreach(JE_FIELD IN ITEMS name commit sha256 archive url)
        string(JSON JE_${JE_FIELD} GET "${JE_RECEIPTS}" ${JE_INDEX} ${JE_FIELD})
    endforeach()
    list(FIND JE_EXTERNAL_NAMES "${JE_name}" JE_PIN_INDEX)
    if(JE_PIN_INDEX LESS 0 OR JE_name IN_LIST JE_SEEN_NAMES)
        message(FATAL_ERROR "JE local Python: unexpected or duplicate external receipt: ${JE_name}")
    endif()
    list(APPEND JE_SEEN_NAMES "${JE_name}")
    list(GET JE_EXTERNAL_COMMITS ${JE_PIN_INDEX} JE_PIN_COMMIT)
    if(NOT JE_commit STREQUAL JE_PIN_COMMIT)
        message(FATAL_ERROR "JE local Python: external commit mismatch: ${JE_name}")
    endif()
    if(JE_PIN_INDEX LESS 4)
        set(JE_REPO cpython-source-deps)
    else()
        set(JE_REPO cpython-bin-deps)
    endif()
    set(JE_PIN_URL "https://codeload.github.com/python/${JE_REPO}/tar.gz/${JE_PIN_COMMIT}")
    if(NOT JE_url STREQUAL JE_PIN_URL)
        message(FATAL_ERROR "JE local Python: external receipt origin mismatch: ${JE_name}")
    endif()
    JE_python_verify_file("${JE_archive}" "${JE_sha256}" "${JE_name} archive")
    if(NOT IS_DIRECTORY "${JE_PYTHON_EXTERNALS_DIR}/${JE_name}")
        message(FATAL_ERROR "JE local Python: prepared external work directory is missing: ${JE_name}")
    endif()
    set(JE_COMPONENT "{}")
    foreach(JE_FIELD IN ITEMS name commit sha256 url)
        string(JSON JE_COMPONENT SET "${JE_COMPONENT}" ${JE_FIELD} "\"${JE_${JE_FIELD}}\"")
    endforeach()
    string(JSON JE_COMPONENTS SET "${JE_COMPONENTS}" ${JE_INDEX} "${JE_COMPONENT}")
endforeach()
set(JE_REQUIRED_EXTERNAL_FILES bzip2-1.0.8/bzlib.h bzip2-1.0.8/LICENSE
    sqlite-3.40.1.0/sqlite3.c sqlite-3.40.1.0/sqlite3.h
    xz-5.2.5/COPYING xz-5.2.5/COPYING.GPLv2 xz-5.2.5/COPYING.GPLv3 xz-5.2.5/COPYING.LGPLv2.1
    zlib-1.2.13/zlib.h zlib-1.2.13/LICENSE libffi-3.3.0/LICENSE
    libffi-3.3.0/amd64/libffi-7.dll libffi-3.3.0/amd64/libffi-7.lib libffi-3.3.0/amd64/include/ffi.h
    openssl-bin-1.1.1u/amd64/LICENSE openssl-bin-1.1.1u/amd64/include/openssl/ssl.h
    openssl-bin-1.1.1u/amd64/libcrypto-1_1.dll openssl-bin-1.1.1u/amd64/libssl-1_1.dll
    openssl-bin-1.1.1u/amd64/libcrypto.lib openssl-bin-1.1.1u/amd64/libssl.lib
    tcltk-8.6.12.0/amd64/include/tcl.h tcltk-8.6.12.0/amd64/bin/tcl86t.dll
    tcltk-8.6.12.0/amd64/bin/tk86t.dll tcltk-8.6.12.0/amd64/lib/tcl86t.lib tcltk-8.6.12.0/amd64/lib/tk86t.lib
    tcltk-8.6.12.0/amd64/tcllicense.terms tcltk-8.6.12.0/amd64/tklicense.terms
    tcltk-8.6.12.0/amd64/tixlicense.terms)
foreach(JE_REQUIRED IN LISTS JE_REQUIRED_EXTERNAL_FILES)
    if(NOT EXISTS "${JE_PYTHON_EXTERNALS_DIR}/${JE_REQUIRED}")
        message(FATAL_ERROR "JE local Python: required external payload/license is missing: ${JE_REQUIRED}")
    endif()
endforeach()

# Verify the exact upstream project and a unique target before preparing a work-copy patch.
set(JE_CORE_PROJECT "${JE_PYTHON_SOURCE_WORK}/PCbuild/pythoncore.vcxproj")
set(JE_CORE_UPSTREAM_SHA256 e7ff6ea2427ffe0c24605b5fa46cd09af5d55e536c47187c8dcc803dd5178b2a)
JE_python_verify_file("${JE_CORE_PROJECT}" "${JE_CORE_UPSTREAM_SHA256}" "CPython pythoncore project")
file(READ "${JE_CORE_PROJECT}" JE_CORE_CONTENT)
string(FIND "${JE_CORE_CONTENT}" "\r\n" JE_CORE_CRLF)
string(REPLACE "\r\n" "\n" JE_CORE_CONTENT "${JE_CORE_CONTENT}")
set(JE_CORE_GIT_TARGET [=[  <Target Name="_GetBuildInfo" BeforeTargets="PrepareForBuild">
    <PropertyGroup>
      <GIT Condition="$(GIT) == ''">git</GIT>
      <_GIT>$(GIT)</_GIT>
      <_GIT Condition="$(GIT.Contains(` `))">"$(GIT)"</_GIT>
    </PropertyGroup>
    <Message Text="Getting build info from $(_GIT)" Importance="high" />
    <MakeDir Directories="$(IntDir)" Condition="!Exists($(IntDir))" />
    <Exec Command="$(_GIT) name-rev --name-only HEAD &gt; &quot;$(IntDir)gitbranch.txt&quot;" ContinueOnError="true" />
    <Exec Command="$(_GIT) rev-parse --short HEAD &gt; &quot;$(IntDir)gitversion.txt&quot;" ContinueOnError="true" />
    <Exec Command="$(_GIT) describe --all --always --dirty &gt; &quot;$(IntDir)gittag.txt&quot;" ContinueOnError="true" />
    <PropertyGroup>
      <GitBranch Condition="Exists('$(IntDir)gitbranch.txt')">$([System.IO.File]::ReadAllText('$(IntDir)gitbranch.txt').Trim())</GitBranch>
      <GitVersion Condition="Exists('$(IntDir)gitversion.txt')">$([System.IO.File]::ReadAllText('$(IntDir)gitversion.txt').Trim())</GitVersion>
      <GitTag Condition="Exists('$(IntDir)gittag.txt')">$([System.IO.File]::ReadAllText('$(IntDir)gittag.txt').Trim())</GitTag>
    </PropertyGroup>
    <Message Text="Building $(GitTag):$(GitVersion) $(GitBranch)" Importance="high" />
    <ItemGroup>
      <ClCompile Include="..\Modules\getbuildinfo.c">
        <PreprocessorDefinitions>GITVERSION="$(GitVersion)";GITTAG="$(GitTag)";GITBRANCH="$(GitBranch)";%(PreprocessorDefinitions)</PreprocessorDefinitions>
      </ClCompile>
    </ItemGroup>
  </Target>]=])
string(FIND "${JE_CORE_CONTENT}" "${JE_CORE_GIT_TARGET}" JE_CORE_TARGET_OFFSET)
if(JE_CORE_TARGET_OFFSET LESS 0)
    message(FATAL_ERROR "JE local Python: upstream _GetBuildInfo target must match exactly once")
endif()
string(LENGTH "${JE_CORE_GIT_TARGET}" JE_CORE_TARGET_LENGTH)
math(EXPR JE_CORE_AFTER_OFFSET "${JE_CORE_TARGET_OFFSET} + ${JE_CORE_TARGET_LENGTH}")
string(SUBSTRING "${JE_CORE_CONTENT}" ${JE_CORE_AFTER_OFFSET} -1 JE_CORE_AFTER_TARGET)
string(FIND "${JE_CORE_AFTER_TARGET}" "${JE_CORE_GIT_TARGET}" JE_CORE_DUPLICATE_TARGET)
if(NOT JE_CORE_DUPLICATE_TARGET LESS 0)
    message(FATAL_ERROR "JE local Python: upstream _GetBuildInfo target must match exactly once")
endif()
# Keep upstream messages/directories/compile definitions, skipping only Git I/O when all pins exist.
set(JE_GIT_MISSING "Condition=\"'$(GitVersion)' == '' Or '$(GitTag)' == '' Or '$(GitBranch)' == ''\"")
string(REPLACE "<Exec Command=" "<Exec ${JE_GIT_MISSING} Command=" JE_CORE_PATCHED_TARGET "${JE_CORE_GIT_TARGET}")
string(REPLACE "    <PropertyGroup>\n      <GitBranch " "    <PropertyGroup ${JE_GIT_MISSING}>\n      <GitBranch "
    JE_CORE_PATCHED_TARGET "${JE_CORE_PATCHED_TARGET}")
string(REPLACE "${JE_CORE_GIT_TARGET}" "${JE_CORE_PATCHED_TARGET}" JE_CORE_PATCHED_CONTENT "${JE_CORE_CONTENT}")
if(NOT JE_CORE_CRLF LESS 0)
    string(REPLACE "\n" "\r\n" JE_CORE_PATCHED_CONTENT "${JE_CORE_PATCHED_CONTENT}")
endif()

# All acquisition and receipt checks precede executable invocation or source edits.
set(ENV{PYTHONNOUSERSITE} 1)
set(ENV{PYTHONPATH} "")
set(ENV{PYTHONHOME} "")
set(ENV{VIRTUAL_ENV} "")
set(ENV{TCL_LIBRARY} "")
set(ENV{PIP_NO_INDEX} 1)
set(ENV{PIP_DISABLE_PIP_VERSION_CHECK} 1)
set(ENV{PIP_CONFIG_FILE} NUL)
execute_process(COMMAND "${JE_PYTHON_BOOTSTRAP}" -I -c
    "import sys; assert sys.version_info[:2] == (3, 12), sys.version; print(sys.version)"
    RESULT_VARIABLE JE_BOOTSTRAP_RESULT)
if(NOT JE_BOOTSTRAP_RESULT EQUAL 0)
    message(FATAL_ERROR "JE local Python: bootstrap must be the audited Python 3.12 generator")
endif()
file(MAKE_DIRECTORY "${JE_PYTHON_OUTPUT}")
file(WRITE "${JE_CORE_PROJECT}" "${JE_CORE_PATCHED_CONTENT}")
file(SHA256 "${JE_CORE_PROJECT}" JE_CORE_PATCHED_SHA256)
file(GLOB JE_EXPAT_OVERLAY "${JE_EXPAT_LIB}/*.h" "${JE_EXPAT_LIB}/*.c")
file(COPY ${JE_EXPAT_OVERLAY} DESTINATION "${JE_PYTHON_SOURCE_WORK}/Modules/expat")
# MSBuild global properties override python.props, including its trailing-separator fix.
string(REGEX REPLACE "/+$" "" JE_MSBUILD_EXTERNALS_DIR "${JE_PYTHON_EXTERNALS_DIR}")
string(APPEND JE_MSBUILD_EXTERNALS_DIR "/")
foreach(JE_CONFIGURATION IN ITEMS Debug Release)
    # The guarded work-copy target preserves these explicit archive provenance properties.
    execute_process(COMMAND "${JE_PYTHON_MSBUILD}" "${JE_PYTHON_SOURCE_WORK}/PCbuild/pcbuild.proj"
        /t:Build /m /nologo /v:m "/p:Configuration=${JE_CONFIGURATION}" /p:Platform=x64
        /p:PlatformToolset=v143 /p:IncludeExternals=true /p:IncludeExtensions=true
        /p:IncludeCTypes=true /p:IncludeSSL=true /p:IncludeTkinter=true /p:IncludeTests=true
        /p:SupportSigning=false /p:KillPython=false "/p:ExternalsDir=${JE_MSBUILD_EXTERNALS_DIR}"
        /p:GitVersion=49965601d6afedafe47cc85556d99b7a24981051
        /p:GitTag=cpython-3.10.13-archive /p:GitBranch=archive
        "/p:PythonForBuild=\"${JE_PYTHON_BOOTSTRAP}\" -E -s"
        WORKING_DIRECTORY "${JE_PYTHON_SOURCE_WORK}" RESULT_VARIABLE JE_BUILD_RESULT)
    if(NOT JE_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "JE local Python: ${JE_CONFIGURATION} build failed (${JE_BUILD_RESULT}); outputs retained")
    endif()
endforeach()
set(JE_STAGE "${JE_PYTHON_OUTPUT}/stage")
set(JE_RUNTIME "${JE_PYTHON_SOURCE_WORK}/PCbuild/amd64/python.exe")
set(JE_LAYOUT_OPTIONS --copy "${JE_STAGE}/python" -v --source "${JE_PYTHON_SOURCE_WORK}"
    --build "${JE_PYTHON_SOURCE_WORK}/PCbuild/amd64" --arch amd64 --temp "${JE_PYTHON_OUTPUT}/layout-temp"
    --include-stable --include-pip --include-distutils --include-tcltk --include-idle
    --include-tools --include-venv --include-dev --include-launchers)
foreach(JE_CONFIGURATION IN ITEMS Debug Release)
    set(JE_DEBUG_FLAG "")
    if(JE_CONFIGURATION STREQUAL "Debug")
        set(JE_DEBUG_FLAG -d)
    endif()
    execute_process(COMMAND "${JE_RUNTIME}" -E -s "${JE_PYTHON_SOURCE_WORK}/PC/layout/main.py"
        ${JE_LAYOUT_OPTIONS} ${JE_DEBUG_FLAG} WORKING_DIRECTORY "${JE_PYTHON_SOURCE_WORK}"
        RESULT_VARIABLE JE_LAYOUT_RESULT)
    if(NOT JE_LAYOUT_RESULT EQUAL 0)
        message(FATAL_ERROR "JE local Python: ${JE_CONFIGURATION} layout failed (${JE_LAYOUT_RESULT}); outputs retained")
    endif()
endforeach()
file(GLOB JE_PYTHON_PDBS "${JE_PYTHON_SOURCE_WORK}/PCbuild/amd64/python*.pdb")
file(COPY ${JE_PYTHON_PDBS} DESTINATION "${JE_STAGE}/python")
foreach(JE_REQUIRED_PAYLOAD IN ITEMS python.exe python_d.exe python310.dll python310_d.dll python3.dll python3_d.dll
    python310.pdb python310_d.pdb python3.pdb python3_d.pdb libs/python310.lib libs/python310_d.lib
    include/Python.h include/pyconfig.h Lib/venv/__init__.py Lib/ensurepip/__init__.py
    Lib/site-packages/pip/__init__.py Lib/site-packages/setuptools/__init__.py)
    if(NOT EXISTS "${JE_STAGE}/python/${JE_REQUIRED_PAYLOAD}")
        message(FATAL_ERROR "JE local Python: original package layout is incomplete: ${JE_REQUIRED_PAYLOAD}")
    endif()
endforeach()
file(COPY "${CMAKE_CURRENT_LIST_DIR}/FindPython.cmake" "${CMAKE_CURRENT_LIST_DIR}/python-config.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/python-config-version.cmake" "${CMAKE_CURRENT_LIST_DIR}/JERequireLocalPath.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/NOTICE.txt" DESTINATION "${JE_STAGE}")
# Preserve original legal files, including notices not aggregated by CPython.
function(JE_copy_python_notices JE_ROOT JE_DESTINATION)
    file(GLOB_RECURSE JE_FILES LIST_DIRECTORIES FALSE RELATIVE "${JE_ROOT}" "${JE_ROOT}/*")
    foreach(JE_FILE IN LISTS JE_FILES)
        get_filename_component(JE_BASENAME "${JE_FILE}" NAME)
        string(TOUPPER "${JE_BASENAME}" JE_UPPER_NAME)
        if(JE_UPPER_NAME MATCHES "^(LICENSE|LICENCE|COPYING|NOTICE)([._-].*)?$" OR JE_UPPER_NAME MATCHES "LICENSE[.]TERMS$")
            get_filename_component(JE_SUBDIR "${JE_FILE}" DIRECTORY)
            file(MAKE_DIRECTORY "${JE_DESTINATION}/${JE_SUBDIR}")
            configure_file("${JE_ROOT}/${JE_FILE}" "${JE_DESTINATION}/${JE_FILE}" COPYONLY)
        endif()
    endforeach()
endfunction()
JE_copy_python_notices("${JE_PYTHON_SOURCE_WORK}" "${JE_STAGE}/LICENSES/CPython")
file(MAKE_DIRECTORY "${JE_STAGE}/LICENSES/CPython/PC")
configure_file("${JE_PYTHON_SOURCE_WORK}/PC/crtlicense.txt" "${JE_STAGE}/LICENSES/CPython/PC/crtlicense.txt" COPYONLY)
JE_copy_python_notices("${JE_PYTHON_EXPAT_WORK}/expat" "${JE_STAGE}/LICENSES/Expat-2.4.6")
JE_copy_python_notices("${JE_PYTHON_EXTERNALS_DIR}" "${JE_STAGE}/LICENSES/externals")
file(MAKE_DIRECTORY "${JE_STAGE}/LICENSES/externals/sqlite-3.40.1.0")
configure_file("${JE_PYTHON_EXTERNALS_DIR}/sqlite-3.40.1.0/sqlite3.h"
    "${JE_STAGE}/LICENSES/externals/sqlite-3.40.1.0/sqlite3.h" COPYONLY)
file(COPY "${JE_ENGINE_ROOT}/LICENSE.txt" "${JE_ENGINE_ROOT}/LICENSE_APACHE2.TXT" "${JE_ENGINE_ROOT}/LICENSE_MIT.TXT"
    DESTINATION "${JE_STAGE}/LICENSES/O3DE")
set(JE_SOURCE_URL "https://codeload.github.com/python/cpython/tar.gz/49965601d6afedafe47cc85556d99b7a24981051")
set(JE_EXPAT_URL "https://codeload.github.com/libexpat/libexpat/tar.gz/65a21f2b2a306d29b44e70264aca948aa0454219")
set(JE_PROVENANCE "{}")
string(JSON JE_PROVENANCE SET "${JE_PROVENANCE}" externals "${JE_COMPONENTS}")
string(JSON JE_PROVENANCE SET "${JE_PROVENANCE}" expat
    "{\"version\":\"2.4.6\",\"upstreamBundledVersion\":\"2.5.0\",\"commit\":\"65a21f2b2a306d29b44e70264aca948aa0454219\",\"sha256\":\"${JE_PYTHON_EXPAT_SHA256}\",\"url\":\"${JE_EXPAT_URL}\"}")
string(JSON JE_PROVENANCE SET "${JE_PROVENANCE}" tools
    "{\"toolset\":\"v143\",\"originalToolset\":\"v141\",\"bootstrapVersion\":\"3.12\",\"bootstrapExeSha256\":\"${JE_PYTHON_BOOTSTRAP_SHA256}\",\"msbuildExeSha256\":\"${JE_PYTHON_MSBUILD_SHA256}\"}")
string(JSON JE_PROVENANCE SET "${JE_PROVENANCE}" buildInfo
    "{\"project\":\"PCbuild/pythoncore.vcxproj\",\"upstreamSha256\":\"${JE_CORE_UPSTREAM_SHA256}\",\"patchedSha256\":\"${JE_CORE_PATCHED_SHA256}\",\"gitVersion\":\"49965601d6afedafe47cc85556d99b7a24981051\",\"gitTag\":\"cpython-3.10.13-archive\",\"gitBranch\":\"archive\",\"adaptation\":\"Skip Git queries and git-file reads only when all three explicit provenance values exist\"}")
file(WRITE "${JE_STAGE}/JEProvenance.json" "${JE_PROVENANCE}\n")
set(JE_PACKAGE_STAGE "${JE_STAGE}")
set(JE_PACKAGE_OUTPUT "${JE_PYTHON_OUTPUT}/packages")
set(JE_PACKAGE_NAME python-3.10.13-je-rev3-windows)
set(JE_PACKAGE_VERSION 3.10.13-je-rev3)
set(JE_PACKAGE_LICENSE PSF-2.0)
set(JE_PACKAGE_SOURCE_COMMIT 49965601d6afedafe47cc85556d99b7a24981051)
set(JE_PACKAGE_SOURCE_SHA256 "${JE_PYTHON_SOURCE_SHA256}")
set(JE_PACKAGE_SOURCE_URL "${JE_SOURCE_URL}")
include("${CMAKE_CURRENT_LIST_DIR}/../JECreateLocalPackage.cmake")
