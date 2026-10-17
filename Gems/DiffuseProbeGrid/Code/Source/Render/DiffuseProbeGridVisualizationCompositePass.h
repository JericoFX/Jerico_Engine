/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/Pass.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        //! This pass composites the DiffuseProbeGrid visualization image onto the main scene
        class DiffuseProbeGridVisualizationCompositePass
            : public JE::Render::RPI::FullscreenTrianglePass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridVisualizationCompositePass);

            AZ_RTTI(Render::DiffuseProbeGridVisualizationCompositePass, "{64BD5779-AB30-41C1-81B7-B93D864355E5}", JE::Render::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::DiffuseProbeGridVisualizationCompositePass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static JE::Render::RPI::Ptr<DiffuseProbeGridVisualizationCompositePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            ~DiffuseProbeGridVisualizationCompositePass() = default;

        private:
            explicit DiffuseProbeGridVisualizationCompositePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            bool IsEnabled() const override;
        };
    }   // namespace RPI
}   // namespace AZ
