/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Geometry/GeometryExports.h>

#include <AzCore/Debug/Profiler.h>

JE_GEOMETRY_API AZ_DECLARE_BUDGET(JE_Geometry_SurfaceData);

//#define JE_ENABLE_SURFACE_DATA_PROFILE_VERBOSE
#ifdef JE_ENABLE_SURFACE_DATA_PROFILE_VERBOSE
// Add verbose profile markers
#define JE_SURFACE_DATA_PROFILE_SCOPE_VERBOSE(...) AZ_PROFILE_SCOPE(JE_Geometry_SurfaceData, __VA_ARGS__);
#define JE_SURFACE_DATA_PROFILE_FUNCTION_VERBOSE AZ_PROFILE_FUNCTION(JE_Geometry_SurfaceData);
#else
// Define JE_ENABLE_SURFACE_DATA_PROFILE_VERBOSE to get verbose profile markers
#define JE_SURFACE_DATA_PROFILE_SCOPE_VERBOSE(...)
#define JE_SURFACE_DATA_PROFILE_FUNCTION_VERBOSE
#endif
