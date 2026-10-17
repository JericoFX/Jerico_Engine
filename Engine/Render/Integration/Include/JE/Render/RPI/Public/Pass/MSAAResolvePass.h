/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducer.h>

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Pass/RenderPass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

namespace JE::Render
{
    namespace RPI
    {
        class JE_RENDER_INTEGRATION_API MSAAResolvePass
            : public RenderPass
        {
            AZ_RPI_PASS(MSAAResolvePass);

        public:
            AZ_RTTI(MSAAResolvePass, "{0234AE92-3EE3-4796-9138-C9B28F9F7AEE}", RenderPass);
            AZ_CLASS_ALLOCATOR(MSAAResolvePass, AZ::SystemAllocator);
            virtual ~MSAAResolvePass() = default;

            static Ptr<MSAAResolvePass> Create(const PassDescriptor& descriptor);

        protected:
            MSAAResolvePass(const PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:

            // Scope producer functions...
            void SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;
        };
    }   // namespace RPI
}   // namespace AZ
