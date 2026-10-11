# Copyright (c) Contributors to the Open 3D Engine Project.
# SPDX-License-Identifier: Apache-2.0 OR MIT
# Apply only to the producer's already-verified private source-work copy.
cmake_path(IS_PREFIX JE_SAMPLERATE_OUTPUT "${JE_WORK}" NORMALIZE JE_PATCH_IN_OUTPUT)
if(NOT JE_PATCH_IN_OUTPUT OR JE_WORK STREQUAL JE_SAMPLERATE_OUTPUT)
    message(FATAL_ERROR "JE local libsamplerate: patch source must be inside the fresh producer output")
endif()
set(JE_PATCH_SOURCE "${JE_WORK}/src/samplerate.c")
set(JE_PATCH_BEFORE_SHA "415263500672638f67c36f1a34332cb2f96762efc801d313fd0ac125ef21524b")
set(JE_PATCH_AFTER_SHA "14238804216c63fc3cd5b42cdb2fde1086cc510cac2f6c13da4bafab55076fae")
JE_python_verify_file("${JE_PATCH_SOURCE}" "${JE_PATCH_BEFORE_SHA}" "libsamplerate channel guard original source")
file(SIZE "${JE_PATCH_SOURCE}" JE_PATCH_BEFORE_SIZE)
if(NOT JE_PATCH_BEFORE_SIZE EQUAL 12951)
    message(FATAL_ERROR "JE local libsamplerate: channel guard original size differs")
endif()
file(READ "${JE_PATCH_SOURCE}" JE_PATCH_TEXT)
set(JE_PATCH_BEFORE [=[src_new (int converter_type, int channels, int *error)
{
	return psrc_set_converter (converter_type, channels, error) ;
} /* src_new */]=])
set(JE_PATCH_AFTER [=[src_new (int converter_type, int channels, int *error)
{
	if (channels < 1)
	{	if (error)
			*error = SRC_ERR_BAD_CHANNEL_COUNT ;
		return NULL ;
		} ;

	return psrc_set_converter (converter_type, channels, error) ;
} /* src_new */]=])
string(FIND "${JE_PATCH_TEXT}" "${JE_PATCH_BEFORE}" JE_PATCH_OFFSET)
if(JE_PATCH_OFFSET LESS 0)
    message(FATAL_ERROR "JE local libsamplerate: channel guard hunk is missing")
endif()
string(LENGTH "${JE_PATCH_BEFORE}" JE_PATCH_HUNK_SIZE)
math(EXPR JE_PATCH_REMAINDER_OFFSET "${JE_PATCH_OFFSET} + ${JE_PATCH_HUNK_SIZE}")
string(SUBSTRING "${JE_PATCH_TEXT}" ${JE_PATCH_REMAINDER_OFFSET} -1 JE_PATCH_REMAINDER)
string(FIND "${JE_PATCH_REMAINDER}" "${JE_PATCH_BEFORE}" JE_PATCH_SECOND_OFFSET)
if(NOT JE_PATCH_SECOND_OFFSET LESS 0)
    message(FATAL_ERROR "JE local libsamplerate: channel guard hunk is not unique")
endif()
# Keep original and modified notices distinct; never replace the upstream snapshot.
file(MAKE_DIRECTORY "${JE_SAMPLERATE_OUTPUT}/patch-provenance")
configure_file("${JE_PATCH_SOURCE}" "${JE_SAMPLERATE_OUTPUT}/patch-provenance/samplerate.c.original" COPYONLY)
string(REPLACE "${JE_PATCH_BEFORE}" "${JE_PATCH_AFTER}" JE_PATCH_TEXT "${JE_PATCH_TEXT}")
string(SHA256 JE_PATCH_CANDIDATE_SHA "${JE_PATCH_TEXT}")
if(NOT JE_PATCH_CANDIDATE_SHA STREQUAL JE_PATCH_AFTER_SHA)
    message(FATAL_ERROR "JE local libsamplerate: channel guard candidate SHA256 differs")
endif()
file(CONFIGURE OUTPUT "${JE_PATCH_SOURCE}" CONTENT "${JE_PATCH_TEXT}" @ONLY NEWLINE_STYLE UNIX)
JE_python_verify_file("${JE_PATCH_SOURCE}" "${JE_PATCH_AFTER_SHA}" "libsamplerate channel guard patched source")
file(SIZE "${JE_PATCH_SOURCE}" JE_PATCH_AFTER_SIZE)
if(NOT JE_PATCH_AFTER_SIZE EQUAL 13047)
    message(FATAL_ERROR "JE local libsamplerate: channel guard patched size differs")
endif()
