/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameGraphExecuteContext.h>

#include <JE/Render/RPI/Public/GpuQuery/TimestampQueryPool.h>

namespace AZ
{
    }

namespace JE::Render::RPI
{
        TimestampQueryPool::TimestampQueryPool(uint32_t queryCapacity, uint32_t queriesPerResult, AZ::RHI::QueryType queryType, AZ::RHI::PipelineStatisticsFlags statisticsFlags) :
            QueryPool(queryCapacity, queriesPerResult, queryType, statisticsFlags)
        {
        }

        QueryPoolPtr JE::Render::RPI::TimestampQueryPool::CreateTimestampQueryPool(uint32_t queryCount)
        {
            // The number of RHI Queries required to calculate a single Timestamp Result.
            const uint32_t RhiQueriesPerTimestampResult = 2u;
            return AZStd::unique_ptr<QueryPool>(aznew TimestampQueryPool(queryCount, RhiQueriesPerTimestampResult, AZ::RHI::QueryType::Timestamp, AZ::RHI::PipelineStatisticsFlags::None));
        }

        AZ::RHI::ResultCode TimestampQueryPool::BeginQueryInternal(AZ::RHI::Interval rhiQueryIndices, const AZ::RHI::FrameGraphExecuteContext& context)
        {
            AZStd::span<const AZ::RHI::Ptr<AZ::RHI::Query>> rhiQueryArray = GetRhiQueryArray();
            AZ::RHI::Ptr<AZ::RHI::Query> beginQuery = rhiQueryArray[rhiQueryIndices.m_min];

            return beginQuery->GetDeviceQuery(context.GetDeviceIndex())->WriteTimestamp(*context.GetCommandList());
        }

        AZ::RHI::ResultCode TimestampQueryPool::EndQueryInternal(AZ::RHI::Interval rhiQueryIndices, const AZ::RHI::FrameGraphExecuteContext& context)
        {
            AZStd::span<const AZ::RHI::Ptr<AZ::RHI::Query>> rhiQueryArray = GetRhiQueryArray();
            AZ::RHI::Ptr<AZ::RHI::Query> endQuery = rhiQueryArray[rhiQueryIndices.m_max];

            return endQuery->GetDeviceQuery(context.GetDeviceIndex())->WriteTimestamp(*context.GetCommandList());
        }
    }

namespace AZ
{;  // Namespace RPI
};  // Namespace AZ
