/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Draw/EditorStateMeshDrawPacket.h>

#include <JE/Render/RPI/Reflect/Model/ModelLodIndex.h>
#include <JE/Render/Scene/Mesh/MeshHandleStateBus.h>
#include <AzCore/Instance/Instance.h>
#include <AzCore/std/containers/vector.h>

namespace JE::Editor::Render::Features
{
    //! Representation of a focused entity's Atom mesh (if any).
    //! @note It is not an error for an entity to not have any Atom mesh.
    class DrawableMeshEntity
        : private JE::Render::Scene::MeshHandleStateNotificationBus::Handler
    {
    public:
        DrawableMeshEntity(AZ::EntityId entityId, AZ::Data::Instance<JE::Render::RPI::Material> maskMaterial, AZ::Name drawList);
        ~DrawableMeshEntity();

        //! Returns true if this entity can be drawn, otherwise false.
        bool CanDraw() const;

        //! Draws the entity's Atom mesh.
        void Draw();

    private:

        //! Retrieves the levol of detail index for this entity's Atom mesh.
        JE::Render::RPI::ModelLodIndex GetModelLodIndex(const JE::Render::RPI::ViewPtr view, AZ::Data::Instance<JE::Render::RPI::Model> model) const;

        // MeshHandleStateNotificationBus overrides ...
        void OnMeshHandleSet(const JE::Render::Features::MeshFeatureProcessorInterface::MeshHandle* meshHandle) override;

        //! Builds the entity's drawable mesh data from scratch, overwriting any existing data.
        void CreateOrUpdateMeshDrawPackets(
            const JE::Render::Features::MeshFeatureProcessorInterface* featureProcessor,
            const JE::Render::RPI::ModelLodIndex modelLodIndex,
            AZ::Data::Instance<JE::Render::RPI::Model> model);

        //! Clears the entity's mesh draw packets and other draw state.
        void ClearDrawData();

        // Builds the mesh draw packets for the Atom mesh.
        void BuildMeshDrawPackets(
            const AZ::Data::Asset<JE::Render::RPI::ModelAsset> modelAsset, AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> meshObjectSrg);

        // Creates the mask shader resource group for the Atom mesh.
        AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> CreateMaskShaderResourceGroup(
            const JE::Render::Features::MeshFeatureProcessorInterface* featureProcessor) const;

        AZ::EntityId m_entityId;
        const JE::Render::Features::MeshFeatureProcessorInterface::MeshHandle* m_meshHandle = nullptr;
        AZ::Data::Instance<JE::Render::RPI::Material> m_maskMaterial = nullptr;
        AZ::Name m_drawList;
        JE::Render::RPI::ModelLodIndex m_modelLodIndex = JE::Render::RPI::ModelLodIndex::Null;
        AZStd::vector<EditorStateMeshDrawPacket> m_meshDrawPackets;
    };
} // namespace JE::Editor::Render::Features
