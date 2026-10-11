# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

function(JE_vvl_apply_spirv_provenance JE_PRIVATE_SOURCE)
    set(JE_VERSION_SCRIPT "${JE_PRIVATE_SOURCE}/utils/update_build_version.py")
    JE_python_verify_file("${JE_VERSION_SCRIPT}"
        "417a730bde8a2ddaf27c95db16a8c9d923edba27a2ecfd2196c0d3d6ec641515" "pristine SPIRV version generator")
    file(READ "${JE_VERSION_SCRIPT}" JE_BEFORE)
    string(REGEX MATCHALL "description = describe\\(repo_path\\)" JE_HUNKS "${JE_BEFORE}")
    list(LENGTH JE_HUNKS JE_COUNT)
    if(NOT JE_COUNT EQUAL 1)
        message(FATAL_ERROR "JE VVL: SPIRV provenance hunk must occur exactly once")
    endif()
    set(JE_OLD "description = describe(repo_path)")
    set(JE_NEW "description = \"04cdb2d344706052c7a2d359294e830ebac63e74 (archive)\"")
    string(REPLACE "${JE_OLD}" "${JE_NEW}" JE_AFTER "${JE_BEFORE}")
    string(REPLACE "${JE_NEW}" "${JE_OLD}" JE_INVERSE "${JE_AFTER}")
    if(NOT JE_INVERSE STREQUAL JE_BEFORE)
        message(FATAL_ERROR "JE VVL: SPIRV provenance inverse differs from the original source")
    endif()
    # Only this fresh work copy changes; source archives/snapshots remain intact.
    file(CONFIGURE OUTPUT "${JE_VERSION_SCRIPT}" CONTENT "${JE_AFTER}" @ONLY NEWLINE_STYLE UNIX)
    JE_python_verify_file("${JE_VERSION_SCRIPT}"
        "021ef8f618ec1a11c8ec0dd63fb6c661af3eaac2ab00dc6199e2efe20acf65c6" "archive-pinned SPIRV version generator")
endfunction()
