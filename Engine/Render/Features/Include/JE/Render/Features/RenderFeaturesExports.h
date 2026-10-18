/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformDef.h>

#if defined(AZ_MONOLITHIC_BUILD)
    #define JE_RENDER_FEATURES_API
    #define JE_RENDER_FEATURES_API_EXPORT
#else
    #if defined(JE_RENDER_FEATURES_EXPORTS)
        #define JE_RENDER_FEATURES_API        AZ_DLL_EXPORT
        #define JE_RENDER_FEATURES_API_EXPORT AZ_DLL_EXPORT
    #else
        #define JE_RENDER_FEATURES_API        AZ_DLL_IMPORT
        #define JE_RENDER_FEATURES_API_EXPORT
    #endif
#endif
