/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

#include <JE/Render/RPI/Reflect/Asset/AssetReference.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <JE/Render/RPI/Reflect/Pass/RenderPassData.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Custom data for the RasterPass. Should be specified in the PassRequest.
        struct JE_RENDER_INTEGRATION_API RasterPassData
            : public RenderPassData
        {
            AZ_RTTI(RasterPassData, "{48AAC4A1-EFD5-46E8-9376-E08243F88F54}", RenderPassData);
            AZ_CLASS_ALLOCATOR(RasterPassData, AZ::SystemAllocator);

            RasterPassData() = default;
            virtual ~RasterPassData() = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::Name m_drawListTag;

            //! Reference to the shader asset that contains the PassSrg. This will be used to load the SRG in the raster pass.
            AssetReference m_passSrgShaderReference;

            AZ::RHI::Viewport m_overrideViewport = AZ::RHI::Viewport::CreateNull();
            AZ::RHI::Scissor m_overrideScissor = AZ::RHI::Scissor::CreateNull();

            AZ::RHI::DrawListSortType m_drawListSortType = AZ::RHI::DrawListSortType::KeyThenDepth;

            // Forces viewport and scissor to match width/height of output image at specified index.
            // Does nothing if index is negative.
            AZ::s32 m_viewportAndScissorTargetOutputIndex = -1;

            // Specifies whether to enable DrawItems with this DrawListTag when they are created
            // Set to false if you want to manually enable draw items for your Pass/DrawListTag
            bool m_enableDrawItemsByDefault = true;
        };
    } // namespace RPI
} // namespace AZ

