/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/GpuQuery/GpuQuerySystemInterface.h>
#include <JE/Render/RPI/Public/GpuQuery/QueryPool.h>

#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    }

namespace JE::Render::RPI
{
        class CommandList;
        struct GpuQuerySystemDescriptor;

        class JE_RENDER_INTEGRATION_API GpuQuerySystem
            : public GpuQuerySystemInterface
        {
        public:
            AZ_TYPE_INFO(GpuQuerySystem, "{7A314998-AB21-464B-8AFC-525018D2150A}");
            AZ_CLASS_ALLOCATOR(GpuQuerySystem, AZ::SystemAllocator);

            GpuQuerySystem() = default;

            void Init(const GpuQuerySystemDescriptor& desc);
            void Shutdown();
            void Update();

        private:
            // GpuQuerySystemInterface Overrides...
            AZ::RHI::Ptr<Query> CreateQuery(AZ::RHI::QueryType queryType,
                AZ::RHI::QueryPoolScopeAttachmentType attachmentType,
                AZ::RHI::ScopeAttachmentAccess attachmentAccess) final;

            // Caches the QueryTypes that are supported for the device that is owned by the RHISystem.
            void CacheFeatureSupport();
            // Checks whether the QueryType is valid.
            bool IsQueryTypeValid(AZ::RHI::QueryType queryType);
            // Returns whether the QueryType is supported for the device that is owned by the RHISystem.
            bool IsQueryTypeSupported(AZ::RHI::QueryType queryType);

            // Gets the RPI QueryPool depending on the QueryType.
            JE::Render::RPI::QueryPool* GetQueryPoolByType(AZ::RHI::QueryType queryType);

            // Cache QueryTypeFlags support for all command queues associated with the device that is owned by the RHISystem.
            AZ::RHI::QueryTypeFlags m_queryTypeSupport = AZ::RHI::QueryTypeFlags::None;

            // Array of RPI QueryPools. One for each QueryType.
            AZStd::array<AZStd::unique_ptr<JE::Render::RPI::QueryPool>, static_cast<uint32_t>(AZ::RHI::QueryType::Count)> m_queryPoolArray = {};
        };

    }

namespace AZ
{; // namespace RPI
}; // namespace AZ
