/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SkinnedMesh/SkinnedMeshShaderOptionsCache.h>

#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <Atom/RHI.Reflect/Base.h>

namespace JE::Render
{
    namespace Features
    {
        class SkinnedMeshFeatureProcessor;

        //! The skinned mesh compute pass submits dispatch items for skinning. The dispatch items are cleared every frame, so it needs to be re-populated.
        class SkinnedMeshComputePass
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(SkinnedMeshComputePass);
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::SkinnedMeshComputePass, "AZ::Render::SkinnedMeshComputePass", "{CE046FFC-B870-40EE-872A-DB0958B97CC3}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(SkinnedMeshComputePass, AZ::SystemAllocator);

            SkinnedMeshComputePass(const JE::Render::RPI::PassDescriptor& descriptor);

            static JE::Render::RPI::Ptr<SkinnedMeshComputePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            AZ::Data::Instance<JE::Render::RPI::Shader> GetShader() const;

        private:
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const JE::Render::RPI::Shader& shader) override;
            void OnShaderVariantReinitialized(const JE::Render::RPI::ShaderVariant& shaderVariant) override;

            SkinnedMeshFeatureProcessor* GetSkinnedMeshFeatureProcessor();
        };
    }
}
