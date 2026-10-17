/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <JE/Render/RPI/Public/Pass/FullscreenTrianglePass.h>

namespace AZ::Render
{
    class SilhouetteFeatureProcessor;

    // Wrapper on top of the RPI::FullscreenTrianglePass to skip the submit of the draw item
    // if there's no objects that draw into the silhouette gather pass.
    // We don't enable/disable the pass because that causes render pass rebuilding when using subpass merging.
    class SilhouetteCompositePass final
        : public JE::Render::RPI::FullscreenTrianglePass
    {
        AZ_RPI_PASS(SilhouetteCompositePass);

    public:
        AZ_RTTI(SilhouetteCompositePass, "{D5185238-790C-4B1D-A12E-8193EA25EF76}", JE::Render::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(SilhouetteCompositePass, SystemAllocator);
        ~SilhouetteCompositePass() = default;

        static JE::Render::RPI::Ptr<SilhouetteCompositePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

    protected:
        SilhouetteCompositePass(const JE::Render::RPI::PassDescriptor& descriptor);

        // Pass behavior overrides...
        void InitializeInternal() override;

        // Scope producer functions...
        void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
        void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

        const SilhouetteFeatureProcessor* m_featureProcessor = nullptr;
    };
}   // namespace AZ::Render
