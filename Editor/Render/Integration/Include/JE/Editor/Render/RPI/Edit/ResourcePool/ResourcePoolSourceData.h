/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <JE/Editor/Render/RPI/Edit/Configuration.h>
#include <Atom/RHI.Reflect/Base.h>
#include <JE/Render/RPI/Reflect/ResourcePoolAsset.h>

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{             
        // Enum type used to identify the type of resource pool source asset.
        enum class ResourcePoolAssetType : AZ::u8
        {
            BufferPool = 0,
            ImagePool,
            StreamingImagePool,
            Unknown,
        };
        
        // Source data of resource pool asset. It may generate different kind of production asset based on m_poolType
        struct JE_EDITOR_RENDER_INTEGRATION_API ResourcePoolSourceData
        {
            AZ_TYPE_INFO(ResourcePoolSourceData, "{8BFF0760-20E3-446D-9E3D-39D0266F7104}");
            AZ_CLASS_ALLOCATOR(ResourcePoolSourceData, AZ::SystemAllocator);
            
            static void Reflect(AZ::ReflectContext* context);

            ResourcePoolAssetType m_poolType = ResourcePoolAssetType::Unknown;
            AZStd::string m_poolName = "Unknown";
            AZ::u64 m_budgetInBytes = 0;

            // Configuration for buffer pool
            AZ::RHI::HeapMemoryLevel m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Device;
            AZ::RHI::HostMemoryAccess m_hostMemoryAccess = AZ::RHI::HostMemoryAccess::Write;
            AZ::RHI::BufferBindFlags m_bufferPoolBindFlags = AZ::RHI::BufferBindFlags::None;

            // Configuration for image pool
            AZ::RHI::ImageBindFlags m_imagePoolBindFlags = AZ::RHI::ImageBindFlags::Color;
        };

    }

namespace AZ
{ // namespace RPI

    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(JE::Render::RPI::ResourcePoolAssetType, "{09D35821-2D42-456B-B53C-C02BF387C5C4}", "RPI::ResourcePoolAssetType");
} // namespace AZ
