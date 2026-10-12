/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <AzCore/PlatformDef.h>

#if defined(AZ_MONOLITHIC_BUILD)
#define JE_DEBUG_IMGUI_API
#elif defined(JE_DEBUG_IMGUI_EXPORTS)
#define JE_DEBUG_IMGUI_API AZ_DLL_EXPORT
#else
#define JE_DEBUG_IMGUI_API AZ_DLL_IMPORT
#endif
