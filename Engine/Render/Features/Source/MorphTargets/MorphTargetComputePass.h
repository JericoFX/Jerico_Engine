/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <Atom/RHI.Reflect/Base.h>

namespace JE::Render
{
    namespace Features
    {
        class SkinnedMeshFeatureProcessor;

        //! The morph target compute pass submits dispatch items for morph targets. The dispatch items are cleared every frame, so it needs to be re-populated.
        class MorphTargetComputePass
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(MorphTargetComputePass);
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::MorphTargetComputePass, "AZ::Render::MorphTargetComputePass", "{14EEACDF-C1BB-4BFC-BB27-6821FDE276B0}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(MorphTargetComputePass, AZ::SystemAllocator);

            MorphTargetComputePass(const JE::Render::RPI::PassDescriptor& descriptor);

            static JE::Render::RPI::Ptr<MorphTargetComputePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            AZ::Data::Instance<JE::Render::RPI::Shader> GetShader() const;

        private:
            void BuildInternal() override;
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            SkinnedMeshFeatureProcessor* GetSkinnedMeshFeatureProcessor();
        };
    }
}
