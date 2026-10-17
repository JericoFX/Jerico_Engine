/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ClearValue.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <JE/Render/RPI/Reflect/Pass/RenderPassData.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Custom data for the SlowClearPass. Should be specified in the PassRequest.
        struct JE_RENDER_INTEGRATION_API SlowClearPassData
            : public RenderPassData
        {
            AZ_RTTI(SlowClearPassData, "{5F2C24A4-62D0-4E60-91EC-C207C10D15C6}", RenderPassData);
            AZ_CLASS_ALLOCATOR(SlowClearPassData, AZ::SystemAllocator);

            SlowClearPassData() = default;
            virtual ~SlowClearPassData() = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::RHI::ClearValue m_clearValue;
        };

    } // namespace RPI
} // namespace AZ

