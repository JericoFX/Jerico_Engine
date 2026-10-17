/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Base.h>
#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/GpuQuery/GpuQuerySystemInterface.h>
#include <JE/Render/RPI/Public/GpuQuery/Query.h>

#include <Atom/RHI.Reflect/Interval.h>

#include <Atom/RHI/QueryPool.h>

#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RHI
    {
        class Query;
    };

    }

namespace JE::Render::RPI
{
        //! A RPI QueryPool keeps track of all the RPI Query instances that are created with this pool instance. The tracking of RPI Queries is intrusive,
        //! (i.e. each RPI Query has a refrence to the RPI QueryPool which it was created from). Upon removing a RPI Query, it unregisters itself from the RPI QueryPool.
        //! The RPI QueryPool also manages the underlying RHI Query related resources.
        class JE_RENDER_INTEGRATION_API QueryPool
        {
            friend class JE::Render::RPI::Query;

            static constexpr uint32_t InvalidQueryIndex = static_cast<uint32_t>(-1);

        public:
            AZ_RTTI(QueryPool, "{9BE78927-35F3-4BFB-9A4C-5B93F570C675}");
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator);

            //! Only use this function to create a new QueryPool object. And force using smart pointer to manage pool's life time.
            static QueryPoolPtr CreateQueryPool(uint32_t queryCount, uint32_t rhiQueriesPerResult, AZ::RHI::QueryType queryType, AZ::RHI::PipelineStatisticsFlags pipelineStatisticsFlags);

            virtual ~QueryPool();

            //! Increments the pool FrameIndex.
            void Update();

            //! Creates a new RPI Query instance, and returns the pointer.
            AZ::RHI::Ptr<Query> CreateQuery(AZ::RHI::QueryPoolScopeAttachmentType attachmentType, AZ::RHI::ScopeAttachmentAccess attachmentAccess);

            //! Returns the query result size in bytes.
            uint32_t GetQueryResultSize() const;

            //! Unregisters the RPI Query from the RPI QueryPool, and invalidates it.
            void UnregisterQuery(Query* query);

        protected:
            QueryPool(uint32_t queryCapacity, uint32_t queriesPerResult, AZ::RHI::QueryType queryType, AZ::RHI::PipelineStatisticsFlags statisticsFlags);

            // Returns the RHI Query array as a span.
            AZStd::span<const AZ::RHI::Ptr<AZ::RHI::Query>> GetRhiQueryArray() const;

        private:
            // Distributes the RHI Query indices into sub-intervals. Each sub interval is assigned to a RPI Query.
            void CreateRhiQueryIntervals();

            // Returns a span of RHI Queries depending on the indices that are provided.
            AZStd::span<const AZ::RHI::Ptr<AZ::RHI::Query>> GetRhiQueriesFromInterval(const AZ::RHI::Interval& rhiQueryIndices) const;
            // Returns an array of raw RHI Query pointers depending on the indices that are provided.
            AZStd::vector<AZ::RHI::DeviceQuery*> GetRawRhiQueriesFromInterval(const AZ::RHI::Interval& rhiQueryIndices, int deviceIndex) const;

            // Readback results from the provided RHI Query indices.
            QueryResultCode GetQueryResultFromIndices(uint64_t* result, AZ::RHI::Interval rhiQueryIndices, AZ::RHI::QueryResultFlagBits queryResultFlag, int deviceIndex);

            // Depending on the QueryType, the method to poll data from the queries vary.
            virtual AZ::RHI::ResultCode BeginQueryInternal(AZ::RHI::Interval rhiQueryIndices, const AZ::RHI::FrameGraphExecuteContext& context);
            virtual AZ::RHI::ResultCode EndQueryInternal(AZ::RHI::Interval rhiQueryIndices, const AZ::RHI::FrameGraphExecuteContext& context);

            // Calculate the query result size in bytes. The size of the result depends on the QueryType.
            void CalculateResultSize();

            // Returns the current FrameIndex of the pool.
            uint64_t GetPoolFrameIndex() const;
            // Returns how many RHI Queries are required for a single result.
            uint32_t GetQueriesPerResult() const;

            // The number of RPI Queries this RPI QueryPool supports.
            uint32_t m_queryCapacity = 0u;
            // The amount of RHI Queries the QueryType needs to produce a single result.
            uint32_t m_queriesPerResult = 0u;
            // The total amount of RHI Queries the RHI QueryPool supports.
            uint32_t m_rhiQueryCapacity = 0u;
            // The size of the result structure that the queries return in bytes.
            uint32_t m_queryResultSize = 0u;
            // The statistics flags used to initialize the RHI QueryPool(Only relevant for query statistics).
            AZ::RHI::PipelineStatisticsFlags m_statisticsFlags = AZ::RHI::PipelineStatisticsFlags::None;
            // The QueryType of this pool.
            AZ::RHI::QueryType m_queryType = AZ::RHI::QueryType::Invalid;

            // The FrameIndex is used to identify the lifetime of the RPI Queries.
            uint64_t m_poolFrameIndex = 0u;

            // A registry of active RPI Queries.
            AZStd::unordered_set<JE::Render::RPI::Query*> m_queryRegistry;
            AZStd::mutex m_queryRegistryMutex;

            // Array of available RHI Query indices.
            AZStd::vector<AZ::RHI::Interval> m_availableIntervalArray;

            // RHI Query related resources.
            AZStd::vector<AZ::RHI::Ptr<AZ::RHI::Query>> m_rhiQueryArray;
            AZ::RHI::Ptr<AZ::RHI::QueryPool> m_rhiQueryPool;
        };

    }

namespace AZ
{; // namespace RPI
}; // namespace AZ
