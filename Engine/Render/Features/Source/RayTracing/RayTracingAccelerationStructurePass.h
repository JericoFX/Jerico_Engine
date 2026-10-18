/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ScopeProducer.h>
#include <JE/Render/RPI/Public/GpuQuery/Query.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>

namespace JE::Render
{
    namespace Features
    {
        //! This pass builds the RayTracing acceleration structures for a scene
        class RayTracingAccelerationStructurePass final
            : public JE::Render::RPI::Pass
            , public AZ::RHI::ScopeProducer
        {
        public:
            AZ_RPI_PASS(RayTracingAccelerationStructurePass);

            using ScopeQuery = AZStd::array<AZ::RHI::Ptr<JE::Render::RPI::Query>, static_cast<size_t>(JE::Render::RPI::ScopeQueryType::Count)>;

            AZ_RTTI(RayTracingAccelerationStructurePass, "{6BAA1755-D7D2-497F-BCDB-CA28B42728DC}", Pass);
            AZ_CLASS_ALLOCATOR(RayTracingAccelerationStructurePass, AZ::SystemAllocator);

            //! Creates a RayTracingAccelerationStructurePass
            static JE::Render::RPI::Ptr<RayTracingAccelerationStructurePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            ~RayTracingAccelerationStructurePass() = default;

            void AddScopeQueryToFrameGraph(AZ::RHI::FrameGraphInterface frameGraph);

        private:
            explicit RayTracingAccelerationStructurePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Scope producer functions
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void BuildCommandList(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // Pass overrides
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            bool RaytracingRevisionOutdated() const;

            // Helper function to get the query by the scope index and query type
            AZ::RHI::Ptr<JE::Render::RPI::Query> GetQuery(JE::Render::RPI::ScopeQueryType queryType);

            // Executes a lambda depending on the passed ScopeQuery types
            template<typename Func>
            void ExecuteOnTimestampQuery(Func&& func);
            template<typename Func>
            void ExecuteOnPipelineStatisticsQuery(Func&& func);

            JE::Render::RPI::TimestampResult GetTimestampResultInternal() const override;
            JE::Render::RPI::PipelineStatisticsResult GetPipelineStatisticsResultInternal() const override;

            // Begin recording commands for the ScopeQueries
            void BeginScopeQuery(const AZ::RHI::FrameGraphExecuteContext& context);

            // End recording commands for the ScopeQueries
            void EndScopeQuery(const AZ::RHI::FrameGraphExecuteContext& context);

            // Readback the results from the ScopeQueries
            void ReadbackScopeQueryResults();

            // buffer view descriptor for the TLAS
            AZ::RHI::BufferViewDescriptor m_tlasBufferViewDescriptor;

            // keeps track of the current frame to determine updates or rebuilds of the skinned BLASes
            uint64_t m_frameCount = 0;

            // the bits of this constant are used to check if a skinned BLAS is going to be rebuilt in any given frame
            static constexpr uint32_t SKINNED_BLAS_REBUILD_FRAME_INTERVAL = 8;

            // Readback results from the Timestamp queries
            JE::Render::RPI::TimestampResult m_timestampResult{};

            // Readback results from the PipelineStatistics queries
            JE::Render::RPI::PipelineStatisticsResult m_statisticsResult{};

            // The device index the pass ran on during the last frame, necessary to read the queries.
            int m_lastDeviceIndex = AZ::RHI::MultiDevice::DefaultDeviceIndex;

            // For each ScopeProducer an instance of the ScopeQuery is created, which consists
            // of a Timestamp and PipelineStatistic query.
            ScopeQuery m_scopeQueries;
        };
    } // namespace Features
} // namespace JE::Render
