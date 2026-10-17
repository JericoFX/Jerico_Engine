/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Reflect/Configuration.h>
#include <JE/Render/RPI/Reflect/Pass/PassData.h>

namespace JE::Render
{
    namespace RPI
    {
        struct JE_RENDER_INTEGRATION_API RenderToTexturePassData
            : public PassData
        {
            AZ_RTTI(RenderToTexturePassData, "{A2DEDE8A-C203-4BEA-896F-7F29A58F6978}", PassData);
            AZ_CLASS_ALLOCATOR(RenderToTexturePassData, AZ::SystemAllocator);

            RenderToTexturePassData() = default;
            virtual ~RenderToTexturePassData() = default;

            static void Reflect(AZ::ReflectContext* context);

            uint32_t m_width = 256;
            uint32_t m_height = 256;
            AZ::RHI::Format m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        };
    } // namespace RPI
} // namespace AZ

