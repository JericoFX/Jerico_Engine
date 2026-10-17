/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/Origin.h>

#include <JE/Render/RPI/Reflect/Pass/PassData.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Custom data for the CopyPass. Should be specified in the PassRequest.
        struct JE_RENDER_INTEGRATION_API CopyPassData
            : public PassData
        {
            AZ_RTTI(CopyPassData, "{A0E4DBBF-2FE7-4027-B6B1-4F15AEB03B04}", PassData);
            AZ_CLASS_ALLOCATOR(CopyPassData, AZ::SystemAllocator);

            CopyPassData() = default;
            virtual ~CopyPassData() = default;

            static void Reflect(AZ::ReflectContext* context);

            // Size
            uint32_t m_bufferSize = 0;
            AZ::RHI::Size m_sourceSize;

            // Buffer Source
            uint32_t m_bufferSourceOffset = 0;
            uint32_t m_bufferSourceBytesPerRow = 0;
            uint32_t m_bufferSourceBytesPerImage = 0;

            // Buffer Destination
            uint32_t m_bufferDestinationOffset = 0;
            uint32_t m_bufferDestinationBytesPerRow = 0;
            uint32_t m_bufferDestinationBytesPerImage = 0;

            // Image Source
            AZ::RHI::ImageSubresource m_imageSourceSubresource;
            AZ::RHI::Origin m_imageSourceOrigin;

            // Image Destination
            AZ::RHI::ImageSubresource m_imageDestinationSubresource;
            AZ::RHI::Origin m_imageDestinationOrigin;

            // Device indices
            int m_sourceDeviceIndex = AZ::RHI::MultiDevice::InvalidDeviceIndex;
            int m_destinationDeviceIndex = AZ::RHI::MultiDevice::InvalidDeviceIndex; //@TODO maybe a mask so we can broadcast?

            // If set to true, pass will automatically create a transient output attachment based on input
            // If false, the output target of the copy will need to be specified
            bool m_cloneInput = true;

            // Whether the pass should use the copy queue.
            bool m_useCopyQueue = false;
        };
    } // namespace RPI
} // namespace AZ

