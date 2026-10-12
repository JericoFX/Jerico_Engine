/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Materials/Base.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <JE/Render/Materials/MaterialAssignment.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshHandleStateBus.h>
#include <AtomToolsFramework/PreviewRenderer/PreviewContent.h>

namespace JE
{
    namespace Editor
    {
        //! Creates a simple scene used for most previews and thumbnails
        class JE_EDITOR_MATERIALS_API SharedPreviewContent final : public AtomToolsFramework::PreviewContent, public AZ::Render::MeshHandleStateNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(SharedPreviewContent, AZ::SystemAllocator);

            SharedPreviewContent(
                AZ::RPI::ScenePtr scene,
                AZ::RPI::ViewPtr view,
                AZ::Uuid entityContextId,
                const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset,
                const AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset,
                const AZ::Data::Asset<AZ::RPI::AnyAsset>& lightingPresetAsset,
                const JE::Render::MaterialPropertyOverrideMap& materialPropertyOverrides);

            ~SharedPreviewContent() override;

            void Load() override;
            bool IsReady() const override;
            bool IsError() const override;
            void ReportErrors() override;
            void Update() override;
            bool IsReadyToRender() override;

        private:
            void UpdateModel();
            void UpdateLighting();
            void UpdateCamera();

            ///////////////////////////////////////////////////////////////
            //AZ::Render::MeshHandleStateNotificationBus::Handler overrides
            void OnMeshHandleSet(const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* meshHandle) override;
            ///////////////////////////////////////////////////////////////

            // Called by @m_meshUpdatedHandler
            void OnMeshDrawPacketUpdated(
                const AZ::Render::ModelDataInstanceInterface& meshHandleIface,
                uint32_t lodIndex, uint32_t meshIndex, const AZ::RPI::MeshDrawPacket& meshDrawPacket);

            static constexpr float AspectRatio = 1.0f;
            static constexpr float NearDist = 0.001f;
            static constexpr float FarDist = 100.0f;
            static constexpr float FieldOfView = AZ::Constants::HalfPi;
            static constexpr float CameraRotationAngle = AZ::Constants::QuarterPi / 3.0f;

            AZ::RPI::ScenePtr m_scene;
            AZ::RPI::ViewPtr m_view;
            AZ::Uuid m_entityContextId;
            AZ::Entity* m_modelEntity = {};

            AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;
            AZ::Data::Asset<AZ::RPI::AnyAsset> m_lightingPresetAsset;
            JE::Render::MaterialPropertyOverrideMap m_materialPropertyOverrides;

            // We need @m_meshHandle to be able to register for MeshDrawPacketUpdatedEvent(s).
            // These events are the signals we need to have assurance that the scene is fully available
            // on GPU and we are ready to render and generate the Thumbnail.
            const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* m_meshHandle = nullptr;
            AZ::Render::ModelDataInstanceInterface::MeshDrawPacketUpdatedEvent::Handler m_meshUpdatedHandler;
            uint32_t m_meshDrawPacketUpdateCount = 0;
        };
    } // namespace Editor
} // namespace JE
