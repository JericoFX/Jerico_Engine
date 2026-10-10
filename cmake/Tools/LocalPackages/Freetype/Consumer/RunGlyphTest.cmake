# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT

include("${CMAKE_CURRENT_LIST_DIR}/VerifyFont.cmake")
if(NOT EXISTS "${JE_GLYPH_CONSUMER}")
    message(FATAL_ERROR "Build the real Freetype glyph consumer before running this test")
endif()
execute_process(COMMAND "${JE_GLYPH_CONSUMER}" "${JE_FONT_FILE}"
    RESULT_VARIABLE je_glyph_result)
if(NOT je_glyph_result EQUAL 0)
    message(FATAL_ERROR "Freetype glyph consumer failed: ${je_glyph_result}")
endif()
