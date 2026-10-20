/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Shader/Shader.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Public/Model/ModelLod.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/DrawPacketBuilder.h>

#include <AzCore/Math/Obb.h>

namespace JE::Render::RPI
{
    class Scene;
}

namespace JE::Editor::Render::Features
{
    //! Holds and manages an RHI DrawPacket for a specific mesh, and the resources that are needed to build and maintain it.
    //! @note This class is based on the meshDrawPacket class and could be paired down even further to leave only the pertinent
    //! parts of the interface and implementation.
    class EditorStateMeshDrawPacket
    {
    public:
        using ShaderList = AZStd::vector<AZ::Data::Instance<JE::Render::RPI::Shader>>;

        EditorStateMeshDrawPacket() = default;
        EditorStateMeshDrawPacket(
            JE::Render::RPI::ModelLod& modelLod,
            size_t modelLodMeshIndex,
            AZ::Data::Instance<JE::Render::RPI::Material> materialOverride,
            AZ::Name drawList,
            AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> objectSrg,
            const JE::Render::RPI::MaterialModelUvOverrideMap& materialModelUvMap = {});

        AZ_DEFAULT_COPY(EditorStateMeshDrawPacket);
        AZ_DEFAULT_MOVE(EditorStateMeshDrawPacket);

        bool Update(const JE::Render::RPI::Scene& parentScene, bool forceUpdate = false);

        const AZ::RHI::DrawPacket* GetRHIDrawPacket() const;

        void SetStencilRef(uint8_t stencilRef) { m_stencilRef = stencilRef; }
        void SetSortKey(AZ::RHI::DrawItemSortKey sortKey) { m_sortKey = sortKey; };
        bool SetShaderOption(const AZ::Name& shaderOptionName, JE::Render::RPI::ShaderOptionValue value);

        AZ::Data::Instance<JE::Render::RPI::Material> GetMaterial();

    private:
        bool DoUpdate(const JE::Render::RPI::Scene& parentScene);

        JE::Render::RPI::ConstPtr<AZ::RHI::DrawPacket> m_drawPacket;

        // Note, many of the following items are held locally in the EditorStateMeshDrawPacket solely to keep them resident in memory as long as they are needed
        // for the m_drawPacket. RHI::DrawPacket uses raw pointers only, but we use smart pointers here to hold on to the data.

        // Maintains references to the shader instances to keep their PSO caches resident (see Shader::Shutdown())
        ShaderList m_activeShaders;

        // The model that contains the mesh being represented by the DrawPacket
        AZ::Data::Instance<JE::Render::RPI::ModelLod> m_modelLod;

        // The index of the mesh within m_modelLod that is represented by the DrawPacket
        size_t m_modelLodMeshIndex;

        // The per-object shader resource group
        AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_objectSrg;

        // We hold ConstPtr<RHI::ShaderResourceGroup> instead of Instance<RPI::ShaderResourceGroup> because the Material class
        // does not allow public access to its Instance<RPI::ShaderResourceGroup>.
        JE::Render::RPI::ConstPtr<AZ::RHI::ShaderResourceGroup> m_materialSrg;

        AZStd::fixed_vector<AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup>, AZ::RHI::DrawPacketBuilder::DrawItemCountMax> m_perDrawSrgs;

        // A reference to the material, used to rebuild the DrawPacket if needed
        AZ::Data::Instance<JE::Render::RPI::Material> m_material;

        // Tracks whether the Material has change since the DrawPacket was last built
        JE::Render::RPI::Material::ChangeId m_materialChangeId = JE::Render::RPI::Material::DEFAULT_CHANGE_ID;

        // Set the sort key for the draw packet
        AZ::RHI::DrawItemSortKey m_sortKey = 0;

        // Set the stencil value for this draw packet
        uint8_t m_stencilRef = 0;

        //! A map matches the index of UV names of this material to the custom names from the model.
        JE::Render::RPI::MaterialModelUvOverrideMap m_materialModelUvMap;

        //! List of shader options set for this specific draw packet
        typedef AZStd::pair<AZ::Name, JE::Render::RPI::ShaderOptionValue> ShaderOptionPair;
        typedef AZStd::vector<ShaderOptionPair> ShaderOptionVector;
        ShaderOptionVector m_shaderOptions;
        AZ::RHI::DrawListTag m_drawListTag;
    };
} // namespace JE::Editor::Render::Features
