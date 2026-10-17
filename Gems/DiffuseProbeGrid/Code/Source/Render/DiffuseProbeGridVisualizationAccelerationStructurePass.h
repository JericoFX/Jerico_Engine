/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ScopeProducer.h>
#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Buffer/Buffer.h>
#include <Atom/RHI/RayTracingBufferPools.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGrid;

        //! This pass builds the DiffuseProbeGrid visualization acceleration structure
        class DiffuseProbeGridVisualizationAccelerationStructurePass final
            : public JE::Render::RPI::Pass
            , public RHI::ScopeProducer
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridVisualizationAccelerationStructurePass);

            AZ_RTTI(DiffuseProbeGridVisualizationAccelerationStructurePass, "{103D8917-D4DC-4CA3-BFB4-CD62846D282A}", Pass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridVisualizationAccelerationStructurePass, SystemAllocator);

            //! Creates a DiffuseProbeGridVisualizationAccelerationStructurePass
            static JE::Render::RPI::Ptr<DiffuseProbeGridVisualizationAccelerationStructurePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            ~DiffuseProbeGridVisualizationAccelerationStructurePass() = default;

        private:
            explicit DiffuseProbeGridVisualizationAccelerationStructurePass(const JE::Render::RPI::PassDescriptor& descriptor);

            bool ShouldUpdate(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid) const;

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override;

            // Pass overrides
            bool IsEnabled() const override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;

            bool m_visualizationBlasBuilt = false;
        };
    }   // namespace RPI
}   // namespace AZ
