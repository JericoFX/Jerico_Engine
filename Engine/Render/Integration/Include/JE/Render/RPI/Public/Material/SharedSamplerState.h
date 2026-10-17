/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/SamplerState.h>

namespace JE::Render::RPI
{
    struct SharedSamplerState
    {
        uint32_t m_samplerIndex;
        AZ::RHI::SamplerState m_samplerState;
    };

} // namespace AZ::RPI