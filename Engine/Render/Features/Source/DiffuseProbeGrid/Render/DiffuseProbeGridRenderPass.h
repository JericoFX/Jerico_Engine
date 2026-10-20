/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Size.h>
#include <JE/Render/RPI/Public/Pass/RasterPass.h>

namespace JE::Render
{
    namespace Features
    {
        class DiffuseProbeGrid;

        //! This pass renders the diffuse global illumination in the area covered by
        //! each DiffuseProbeGrid.
        class DiffuseProbeGridRenderPass final
            : public JE::Render::RPI::RasterPass
        {
            using Base = JE::Render::RPI::RasterPass;
            AZ_RPI_PASS(DiffuseProbeGridRenderPass);
      
        public:
            AZ_RTTI(DiffuseProbeGridRenderPass, "{33F79A39-2DB3-46FC-8BA1-9E43E222C322}", Base);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridRenderPass, AZ::SystemAllocator);

            ~DiffuseProbeGridRenderPass() = default;
            static JE::Render::RPI::Ptr<DiffuseProbeGridRenderPass> Create(const JE::Render::RPI::PassDescriptor& descriptor);
        
        private:
            DiffuseProbeGridRenderPass() = delete;
            explicit DiffuseProbeGridRenderPass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            bool IsEnabled() const override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

            // helper function to determine if a DiffuseProbeGrid should be rendered based on its state
            bool ShouldRender(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid);

            AZ::Data::Instance<JE::Render::RPI::Shader> m_shader;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_srgLayout;
        };
    } // namespace Features
} // namespace JE::Render
