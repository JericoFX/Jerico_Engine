/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <AzCore/base.h>

#if defined(AZ_MONOLITHIC_BUILD)
#   define JE_GEOMETRY_API
#   define JE_GEOMETRY_AUTHORING_API
#else
#   if defined(JE_GEOMETRY_EXPORTS)
#       define JE_GEOMETRY_API AZ_DLL_EXPORT
#   else
#       define JE_GEOMETRY_API AZ_DLL_IMPORT
#   endif
#   if defined(JE_GEOMETRY_AUTHORING_EXPORTS)
#       define JE_GEOMETRY_AUTHORING_API AZ_DLL_EXPORT
#   else
#       define JE_GEOMETRY_AUTHORING_API AZ_DLL_IMPORT
#   endif
#endif