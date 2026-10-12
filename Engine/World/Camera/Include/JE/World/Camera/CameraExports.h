/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <AzCore/base.h>
#if defined(AZ_MONOLITHIC_BUILD)
#   define JE_WORLD_CAMERA_API
#elif defined(JE_WORLD_CAMERA_EXPORTS)
#   define JE_WORLD_CAMERA_API AZ_DLL_EXPORT
#else
#   define JE_WORLD_CAMERA_API AZ_DLL_IMPORT
#endif
