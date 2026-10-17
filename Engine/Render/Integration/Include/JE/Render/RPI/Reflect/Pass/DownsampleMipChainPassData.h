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
#include <JE/Render/RPI/Reflect/Pass/PassData.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Custom data for the DownsampleMipChainPass. Should be specified in the PassRequest.
        struct JE_RENDER_INTEGRATION_API DownsampleMipChainPassData
            : public PassData
        {
            AZ_RTTI(DownsampleMipChainPassData, "{EB240B6F-91CB-401A-A099-8F06329BDF35}", PassData);
            AZ_CLASS_ALLOCATOR(DownsampleMipChainPassData, AZ::SystemAllocator);

            DownsampleMipChainPassData() = default;
            virtual ~DownsampleMipChainPassData() = default;

            static void Reflect(AZ::ReflectContext* context);

            //! Reference to the Compute Shader that will be used by DownsampleMipeChainPass
            AssetReference m_shaderReference;
        };
    } // namespace RPI
} // namespace AZ

