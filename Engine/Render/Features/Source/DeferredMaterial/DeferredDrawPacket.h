/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/DrawPacket.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Public/PipelineState.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Shader/ShaderReloadNotificationBus.h>
#include <JE/Render/RPI/Reflect/Material/ShaderCollection.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace JE::Render::Features
{
    // Forward declaration
    class DeferredDrawPacketManager;
    // This is a drawpacket with a single fullscreen draw item for one material-type and it's unique set of shader options
    class DeferredDrawPacket
        : public AZStd::intrusive_base
        , private JE::Render::RPI::ShaderReloadNotificationBus::Handler
    {
    public:
        DeferredDrawPacket() = default;
        ~DeferredDrawPacket();
        DeferredDrawPacket(
            DeferredDrawPacketManager* drawPacketManager,
            const JE::Render::RPI::Scene* scene,
            JE::Render::RPI::Material* material,
            const AZ::Name& materialPipelineName,
            const JE::Render::RPI::ShaderCollection::Item& shaderItem,
            const uint32_t drawPacketId);

        void CompileDrawSrg(AZ::Data::Instance<JE::Render::RPI::Buffer> drawPacketIdBuffer);

        const AZ::RHI::DrawPacket* GetRHIDrawPacket()
        {
            return m_drawPacket.get();
        }
        const AZ::RHI::DrawPacket* GetRHIDrawPacket() const
        {
            return m_drawPacket.get();
        }
        const AZ::RHI::ConstPtr<AZ::RHI::ConstantsLayout> GetRootConstantsLayout() const
        {
            return m_rootConstantsLayout;
        }

        AZ::RHI::DrawListTag GetDrawListTag() const
        {
            return m_drawListTag;
        }

        int32_t GetDrawPacketId() const
        {
            return m_drawPacketId;
        }

        void IncreaseUseCount()
        {
            m_useCount++;
        }

        void DecreaseUseCount()
        {
            m_useCount--;
        }

        int32_t GetUseCount() const
        {
            return m_useCount;
        }

        bool NeedsRebuild() const
        {
            return m_needsRebuild;
        }

        bool IsInitialized() const
        {
            return m_drawPacket != nullptr;
        }

        JE::Render::RPI::ShaderVariantId GetShaderVariantId() const
        {
            return m_shaderVariantId;
        }

#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
            const AZ::Data::Asset<JE::Render::RPI::MaterialAsset>& GetInstigatingMaterialAsset() const
            {
                return m_instigatingMaterialAsset;
            }
            const AZ::Data::Asset<JE::Render::RPI::MaterialTypeAsset>& GetInstigatingMaterialTypeAsset() const
            {
                return m_instigatingMaterialTypeAsset;
            }
#endif /* DEFERRED_DRAWPACKET_DEBUG_PRINT */

            // TODO: Who sets shader-options for what?
            // We can only draw multiple meshes with one deferred draw-call if they have the same shader-options,
            // so if these shader-options are somehow linked to a mesh, that mesh needs a different draw-packet.
            // But if these are global (e.g. DebugRendering), this might still be useful
            // bool SetShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value);
            // bool UnsetShaderOption(const Name& shaderOptionName);
            // void ClearShaderOptions();

        private:
            void Init(
                const JE::Render::RPI::Scene* scene,
                JE::Render::RPI::Material* material,
                const AZ::Name& MaterialPipelineName,
                const JE::Render::RPI::ShaderCollection::Item& shaderItem);

            DeferredDrawPacketManager* m_drawPacketManager;

            // unique Id of the draw-packet
            uint32_t m_drawPacketId;

            int32_t m_useCount = 0;

#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
            // Non-valid Reference to the material-Asset that was used to create this DeferredDrawPacket.
            // Useful for debugging / logprints, but this should never be used to load the asset
            AZ::Data::Asset<JE::Render::RPI::MaterialAsset> m_instigatingMaterialAsset;
            AZ::Data::Asset<JE::Render::RPI::MaterialTypeAsset> m_instigatingMaterialTypeAsset;
#endif /* DEFERRED_DRAWPACKET_DEBUG_PRINT */

            AZ::Data::Instance<JE::Render::RPI::Shader> m_shader;
            JE::Render::RPI::ShaderVariantId m_shaderVariantId;
            JE::Render::RPI::ShaderOptionGroup m_shaderOptions;
            AZ::Name m_materialPipelineName;
            AZ::Name m_shaderTag;
            AZ::RHI::DrawListTag m_drawListTag;
            const AZ::RHI::PipelineState* m_pipelineState;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_materialSrg;
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_drawSrg;
            AZ::RHI::ConstPtr<AZ::RHI::ConstantsLayout> m_rootConstantsLayout;
            AZStd::vector<uint8_t> m_rootConstants;
            AZStd::shared_ptr<AZ::RHI::GeometryView> m_geometryView;

            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_drawPacket;

            bool m_needsRebuild = false;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const JE::Render::RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const AZ::Data::Asset<JE::Render::RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const JE::Render::RPI::ShaderVariant& shaderVariant) override;
    };

} // namespace JE::Render::Features