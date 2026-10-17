/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <JE/Render/Scene/Mesh/MeshComponentConstants.h>
#include <Mesh/MeshComponent.h>
#include <Mesh/EditorMeshStats.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        /**
         * In-editor mesh component.
         * Conducts some additional listening and operations to ensure immediate
         * effects when changing fields in the editor.
         */
        class EditorMeshComponent final
            : public AZ::Render::EditorRenderComponentAdapter<JE::Render::Scene::MeshComponentController, JE::Render::Scene::MeshComponent, JE::Render::Scene::MeshComponentConfig>
            , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
            , private AzFramework::EntityDebugDisplayEventBus::Handler
            , private JE::Render::Scene::MeshComponentNotificationBus::Handler
        {
        public:
            using BaseClass = AZ::Render::EditorRenderComponentAdapter<JE::Render::Scene::MeshComponentController, JE::Render::Scene::MeshComponent, JE::Render::Scene::MeshComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Editor::Render::Scene::EditorMeshComponent, "AZ::Render::EditorMeshComponent", JE::Render::Scene::EditorMeshComponentTypeId, BaseClass, AzToolsFramework::Components::EditorComponentBase)
            AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Editor::Render::Scene::EditorMeshComponent)
            AZ_COMPONENT_BASE(JE::Editor::Render::Scene::EditorMeshComponent)
            AZ_CLASS_ALLOCATOR(JE::Editor::Render::Scene::EditorMeshComponent, AZ::ComponentAllocator);

            static void Reflect(AZ::ReflectContext* context);

            EditorMeshComponent() = default;
            explicit EditorMeshComponent(const JE::Render::Scene::MeshComponentConfig& config);

            // AZ::Component overrides ...
            void Activate() override;
            void Deactivate() override;

            /// Called when you want to change the game asset through code (like when creating components based on assets).
            void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

        private:

            // EditorComponentSelectionRequestsBus overrides ...
            AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
            bool EditorSelectionIntersectRayViewport(const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
            bool SupportsEditorRayIntersect() override;

            // EntityDebugDisplayEventBus overrides ...
            void DisplayEntityViewport(const AzFramework::ViewportInfo&, AzFramework::DebugDisplayRequests&) override;

            // MeshComponentNotificationBus overrides ...
            void OnModelReady(const AZ::Data::Asset<JE::Render::RPI::ModelAsset>& modelAsset, const AZ::Data::Instance<JE::Render::RPI::Model>& model) override;

            // AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler overrides
            void OnEntityVisibilityChanged(bool visibility) override;

            // AzToolsFramework::Components::EditorComponentAdapter overrides
            bool ShouldActivateController() const override;

            AZ::u32 OnConfigurationChanged() override;

            AZ::Crc32 AddEditorMaterialComponent();
            bool HasEditorMaterialComponent() const;
            AZ::u32 GetEditorMaterialComponentVisibility() const;

            // Stats for current mesh asset
            EditorMeshStats m_stats;
        };
    } // namespace Render
} // namespace AZ
