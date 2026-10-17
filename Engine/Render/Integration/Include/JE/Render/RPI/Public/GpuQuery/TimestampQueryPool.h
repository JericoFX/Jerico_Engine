/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/GpuQuery/GpuQueryTypes.h>
#include <JE/Render/RPI/Public/GpuQuery/QueryPool.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Timestamp pool context specialization for timestamp queries.
        class JE_RENDER_INTEGRATION_API TimestampQueryPool final
            : public QueryPool
        {
            // Inherit the constructors of RPI::QueryPool.
            using JE::Render::RPI::QueryPool::QueryPool;

        public:
            AZ_RTTI(TimestampQueryPool, "{95A8D7ED-9BAD-4EC4-A201-AF4FD6345D17}", QueryPool);
            AZ_CLASS_ALLOCATOR(TimestampQueryPool, AZ::SystemAllocator);

            //! Only use this function to create a new Timestamp QueryPool object. And force using smart pointer to manage pool's life time.
            static QueryPoolPtr CreateTimestampQueryPool(uint32_t queryCount);

        protected:
            // RPI::QueryPool overrides...
            AZ::RHI::ResultCode BeginQueryInternal(AZ::RHI::Interval rhiQueryIndices, const AZ::RHI::FrameGraphExecuteContext& context);
            AZ::RHI::ResultCode EndQueryInternal(AZ::RHI::Interval rhiQueryIndices, const AZ::RHI::FrameGraphExecuteContext& context);

        private:
            TimestampQueryPool(uint32_t queryCapacity, uint32_t queriesPerResult, AZ::RHI::QueryType queryType, AZ::RHI::PipelineStatisticsFlags statisticsFlags);
        };
    }
}
