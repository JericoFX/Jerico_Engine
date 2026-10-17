/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Material/MaterialInstanceHandler.h>
#include <JE/Render/RPI/Public/Material/MaterialTextureRegistry.h>
#include <JE/Render/RPI/Public/Material/PersistentIndexAllocator.h>
#include <JE/Render/RPI/Public/Material/TextureSamplerRegistry.h>
#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/Image/Image.h>

namespace AZ
{
    class ReflectContext;
}

namespace JE::Render::RPI
{

    class Material;

    //! Manages system-wide initialization and support for material classes
    class JE_RENDER_INTEGRATION_API MaterialSystem
        : public MaterialInstanceHandlerInterface::Registrar
        , public AZ::Data::AssetBus::Handler
    {
    public:
        static void Reflect(AZ::ReflectContext* context);
        static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

        MaterialSystem() = default;
        MaterialSystem(const MaterialSystem& other) = delete;
        MaterialSystem& operator=(const MaterialSystem& other) = delete;

        // MaterialInstanceHandlerInterface
        MaterialInstanceData RegisterMaterialInstance(const AZ::Data::Instance<Material> material) override;
        void ReleaseMaterialInstance(const MaterialInstanceData& materialInstance) override;
        int32_t RegisterMaterialTexture(const int materialTypeIndex, const int materialInstanceIndex, AZ::Data::Instance<Image> image) override;
        void ReleaseMaterialTexture(const int materialTypeIndex, const int materialInstanceIndex, int32_t textureIndex) override;
        AZStd::shared_ptr<SharedSamplerState> RegisterTextureSampler(
            const int materialTypeIndex, const int materialInstanceIndex, const AZ::RHI::SamplerState& samplerState) override;
        const AZ::RHI::SamplerState GetRegisteredTextureSampler(
            const int materialTypeIndex, const int materialInstanceIndex, const uint32_t samplerIndex) override;

        void Compile() override;

        void DebugPrintMaterialInstances();

        void Init();
        void Shutdown();

    private:
        bool LoadMaterialSrgShaderAsset();
        void CreateSceneMaterialSrg();
        bool UpdateSceneMaterialSrg();
        bool UpdateSharedSamplerStates();
        void PrepareMaterialParameterBuffers();
        void UpdateChangedMaterialParameters();

        //  Data::AssetBus Interface
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        using MaterialIndexAllocator = PersistentIndexAllocator<int32_t>;

        struct InternalMaterialInstanceData
        {
            // either the sceneMaterialSRG, or a separate MaterialSrg for this Material-Instance only
            AZ::Data::Instance<ShaderResourceGroup> m_shaderResourceGroup;

            // this is only used if "Atom_RPI_Traits_Platform.h" defines AZ_TRAIT_REGISTER_TEXTURES_PER_MATERIAL, and
            // the material uses the SingleMaterial - Srg
            AZStd::unique_ptr<MaterialTextureRegistry> m_materialTextureRegistry = nullptr;
            bool m_materialTexturesDirty = false;

            // Texture samplers for this material instance. Used only if the material isn't using the SceneMaterialSrg
            AZStd::unique_ptr<TextureSamplerRegistry> m_textureSamplers = nullptr;

            AZ::Data::Instance<MaterialShaderParameter> m_shaderParameter;
            Material* m_material{ nullptr }; // can't use a smart pointer here, since the material de-registers itself in the destructor
            size_t m_compiledChangeId{ 0 };
        };

        struct MaterialTypeData
        {
            bool m_valid = false;
            // Note: The material either uses the SceneMaterialSrg, which is shared between all materials of all types, or it uses a
            // separate SRG for each Material Instance. We don't have anything shared based on the material-type only.
            bool m_useSceneMaterialSrg = false;
            AZ::Data::AssetId m_materialTypeAssetId;
            AZStd::string m_materialTypeAssetHint;
            MaterialIndexAllocator m_instanceIndices;
            AZ::Data::Instance<Buffer> m_parameterBuffer;
            AZStd::unordered_map<int, uint32_t> m_bindlessReadIndices;

            // we need our own 'raw' BufferView for the parameterBuffer so we can access it with the Bindless-SRG
            AZ::Data::Instance<AZ::RHI::BufferView> m_parameterBufferView;
            // The ShaderParameterLayout connects the Properties to an entry in the parameter-buffer, and/or to a named entry in the
            // MaterialSrg.
            // The layout of the parameter-buffer is constructed from the Properties in the .materialtype, and we can ensure only for
            // materials that use the MaterialPipelines that this layout actually matches, since we generate the
            // `struct MaterialParameters` for them the same way.
            AZStd::unique_ptr<MaterialShaderParameterLayout> m_shaderParameterLayout;
            AZStd::vector<InternalMaterialInstanceData> m_instanceData;
        };

        MaterialIndexAllocator m_materialTypeIndices;
        AZStd::vector<MaterialTypeData> m_materialTypeData;
        AZStd::unordered_map<AZ::Data::AssetId, int32_t> m_materialTypeIndicesMap;

        AZ::RHI::ShaderInputNameIndex m_materialTypeBufferInputIndex = { "m_materialTypeBufferIndices" };
        AZ::Data::Asset<ShaderAsset> m_sceneMaterialSrgShaderAsset;
        AZ::Data::Instance<ShaderResourceGroup> m_sceneMaterialSrg;

        AZ::RHI::ShaderInputNameIndex m_nullTextureIndexInputIndex = { "m_nullTextureIndex" };
        AZ::Data::Instance<JE::Render::RPI::Image> m_nullTexture;

        // Texture samplers shared between all materials that use the SceneMaterialSrg
        TextureSamplerRegistry m_sceneTextureSamplers;

        AZ::Data::Instance<Buffer> m_materialTypeBufferIndicesBuffer;
        bool m_bufferReadIndicesDirty = false;
        bool m_sharedSamplerStatesDirty = false;
        bool m_initialized = false;
    };

} // namespace AZ::RPI
