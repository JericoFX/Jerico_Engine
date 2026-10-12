/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include <AzCore/base.h>
#if defined(AZ_MONOLITHIC_BUILD)
#   define JE_PREFAB_BUILDER_API
#elif defined(JE_PREFAB_BUILDER_EXPORTS)
#   define JE_PREFAB_BUILDER_API AZ_DLL_EXPORT
#else
#   define JE_PREFAB_BUILDER_API AZ_DLL_IMPORT
#endif
