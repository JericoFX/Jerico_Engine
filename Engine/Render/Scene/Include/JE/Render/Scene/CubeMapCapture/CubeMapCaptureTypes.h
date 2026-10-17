/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace JE::Render
{
    namespace Scene
    {
        enum class CubeMapCaptureType : uint32_t
        {
            Specular,
            Diffuse
        };

        enum class CubeMapSpecularQualityLevel : uint32_t
        {
            VeryLow,    // 64
            Low,        // 128
            Medium,     // 256
            High,       // 512
            VeryHigh,   // 1024

            Count
        };

    }
}
