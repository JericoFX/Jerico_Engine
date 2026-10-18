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
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <JE/Render/RPI/Public/Pass/ComputePass.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>

namespace JE::Render
{
    namespace Features
    {
        //! LightCullingTilePrepare
        //! This pass analyzes the depth buffer and builds min/max depth bounds per each tile.
        class LightCullingTilePreparePass final
            : public JE::Render::RPI::ComputePass
        {
            AZ_RPI_PASS(LightCullingTilePreparePass);

        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::LightCullingTilePreparePass, "AZ::Render::LightCullingTilePreparePass", "{3D8AEEF2-F679-4C0C-ADF0-2F7E347D8BF1}", JE::Render::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(LightCullingTilePreparePass, AZ::SystemAllocator);
            virtual ~LightCullingTilePreparePass() = default;

            static JE::Render::RPI::Ptr<LightCullingTilePreparePass> Create(const JE::Render::RPI::PassDescriptor& descriptor);

            static AZ::Name GetLightCullingTilePreparePassTemplateName()
            {
                return AZ::Name("LightCullingTilePrepareMSAATemplate");
            }

        private:

            LightCullingTilePreparePass(const JE::Render::RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildInternal() override;

            ///////////////////////////////////////////////////////////////////
            // ShaderReloadNotificationBus overrides...
            void OnShaderReinitialized(const JE::Render::RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const AZ::Data::Asset<JE::Render::RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const JE::Render::RPI::ShaderVariant& shaderVariant) override;

            // Scope producer functions...
            void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

            AZ::RHI::MultisampleState GetMultiSampleState();
            AZ::Name GetMultiSampleName();

            JE::Render::RPI::ShaderOptionGroup CreateShaderOptionGroup();

            AZStd::array<float, 2> ComputeUnprojectConstants() const;
            AZ::RHI::Size GetDepthBufferDimensions();
            void ChooseShaderVariant();
            AZStd::pair<const JE::Render::RPI::ShaderVariant&, JE::Render::RPI::ShaderOptionGroup> CreateShaderVariant();
            void CreatePipelineStateFromShaderVariant(const JE::Render::RPI::ShaderVariant& shaderVariant, const JE::Render::RPI::ShaderOptionGroup& options);
            void SetConstantData();
            void OnShaderReloaded();

            AZ::RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";

            // The shader variant (for choosing the different MSAA version) is sent to the RHI via the PipelineState
            AZ::RHI::ConstPtr<AZ::RHI::PipelineState> m_msaaPipelineState;

            AZ::Name m_msaaNoneName;
            AZ::Name m_msaaMode2xName;
            AZ::Name m_msaaMode4xName;
            AZ::Name m_msaaMode8xName;
            AZ::Name m_msaaOptionName;
            AZ::Name m_constantDataName;
        };
    } // namespace Features
} // namespace JE::Render
