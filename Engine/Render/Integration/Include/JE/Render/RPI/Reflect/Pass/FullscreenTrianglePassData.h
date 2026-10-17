/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Reflect/Asset/AssetReference.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <JE/Render/RPI/Reflect/Pass/RenderPassData.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Custom data for the FullscreenTrianglePass. Should be specified in the PassRequest.
        struct JE_RENDER_INTEGRATION_API FullscreenTrianglePassData
            : public RenderPassData
        {
            AZ_RTTI(FullscreenTrianglePassData, "{564738A1-9690-4446-8CC1-615EA2567434}", RenderPassData);
            AZ_CLASS_ALLOCATOR(FullscreenTrianglePassData, AZ::SystemAllocator);

            FullscreenTrianglePassData() = default;
            virtual ~FullscreenTrianglePassData() = default;

            static void Reflect(AZ::ReflectContext* context);

            //! Reference to the shader asset used by the fullscreen triangle pass
            AssetReference m_shaderAsset;

            //! Stencil reference value to use for the draw call
            uint32_t m_stencilRef = 0;
        };
    } // namespace RPI
} // namespace AZ

