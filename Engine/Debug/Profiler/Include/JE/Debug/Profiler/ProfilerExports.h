/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <AzCore/PlatformDef.h>

#if defined(AZ_MONOLITHIC_BUILD)
#define JE_PROFILER_CAPTURE_API
#define JE_PROFILER_VIEW_API
#else
#if defined(JE_PROFILER_CAPTURE_EXPORTS)
#define JE_PROFILER_CAPTURE_API AZ_DLL_EXPORT
#else
#define JE_PROFILER_CAPTURE_API AZ_DLL_IMPORT
#endif
#if defined(JE_PROFILER_VIEW_EXPORTS)
#define JE_PROFILER_VIEW_API AZ_DLL_EXPORT
#else
#define JE_PROFILER_VIEW_API AZ_DLL_IMPORT
#endif
#endif
