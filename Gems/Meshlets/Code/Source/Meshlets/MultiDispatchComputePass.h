/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI/DispatchItem.h>

#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

#include <MeshletsRenderObject.h>

namespace AZ
{
    namespace Meshlets
    {
        //! Multi Dispatch Pass - this pass will handle multiple dispatch submission
        //! during each frame - one dispatch per mesh, each represents group of compute
        //! threads that will be working to create meshlets of the given mesh.
        //! This class can be generalized in the future to become a base class for this
        //! dispatch submission.
        //! [To Do] - revisit the 'BuildCommandListInternal' method and refactor to handle
        //! 'under the hood' RHI CPU threads that carries the submissions in parallel
        class MultiDispatchComputePass final
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(MultiDispatchComputePass);

        public:
            AZ_RTTI(MultiDispatchComputePass, "{13B3BAC7-0F12-4C23-BD9E-F82A7830195E}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(MultiDispatchComputePass, SystemAllocator);
            ~MultiDispatchComputePass() = default;

            static JE::Render::RPI::Ptr<MultiDispatchComputePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            //! Thread-safe function for adding the frame's dispatch items
            void AddDispatchItems(AZStd::list<RHI::DispatchItem*>& dispatchItems);

            // Pass behavior overrides
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            //! Returns the shader held by the ComputePass
            Data::Instance<JE::Render::RPI::Shader> GetShader();

        protected:
            MultiDispatchComputePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Overriding methods
            void BuildInternal() override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const JE::Render::RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<JE::Render::RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const JE::Render::RPI::ShaderVariant& shaderVariant) override;

            void BuildShaderAndRenderData();

        private:
            AZStd::unordered_set<const RHI::DispatchItem*> m_dispatchItems;
        };

    }   // namespace Meshlets
}   // namespace AZ

