/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Materials/Base.h>

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <JE/Render/Materials/MaterialComponentBus.h>
#include <JE/Render/Materials/MaterialComponentConstants.h>
#include <JE/Editor/Materials/EditorMaterialComponentSlot.h>
#include <JE/Render/Materials/MaterialComponent.h>

namespace JE
{
    namespace Editor
    {
        //! In-editor material component for displaying and editing material assignments.
        class JE_EDITOR_MATERIALS_API EditorMaterialComponent final
            : public AZ::Render::EditorRenderComponentAdapter<JE::Render::MaterialComponentController, JE::Render::MaterialComponent, JE::Render::MaterialComponentConfig>
            , public JE::Render::MaterialComponentNotificationBus::Handler
        {
        public:
            friend class JsonEditorMaterialComponentSerializer;

            using BaseClass = AZ::Render::EditorRenderComponentAdapter<JE::Render::MaterialComponentController, JE::Render::MaterialComponent, JE::Render::MaterialComponentConfig>;
            AZ_EDITOR_COMPONENT(EditorMaterialComponent, JE::Render::EditorMaterialComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);
            static bool ConvertVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            EditorMaterialComponent() = default;
            EditorMaterialComponent(const JE::Render::MaterialComponentConfig& config);

        private:
            // BaseClass overrides ...
            void Activate() override;
            void Deactivate() override;

            void AddContextMenuActions(QMenu* menu) override;

            //! Called when you want to change the game asset through code (like when creating components based on assets).
            void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override;

            AZ::u32 OnConfigurationChanged() override;

            //! MaterialComponentNotificationBus::Handler overrides...
            void OnMaterialSlotLayoutChanged() override;
            void OnMaterialsCreated(const JE::Render::MaterialAssignmentMap& materials) override;

            // AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler overrides
            void OnEntityVisibilityChanged(bool visibility) override;

            // Regenerates the editor component material slots based on the material and
            // LOD mapping from the model or other consumer of materials.
            // If any corresponding material assignments are found in the component
            // controller configuration then those values will be assigned to the editor component slots.
            void UpdateMaterialSlots();

            // Opens the source material export dialog and updates editor material slots based on
            // selected actions
            AZ::u32 OpenMaterialExporterFromRPE();
            AZ::u32 OpenMaterialExporter(const AzToolsFramework::EntityIdSet& entityIdsToEdit);

            AZ::u32 OnLodsToggled();

            // Get the visibility of the LOD material slots based on the enable flag
            AZ::Crc32 GetLodVisibility() const;

            AZStd::string GetLabelForLod(int lodIndex) const;

            EditorMaterialComponentSlot m_defaultMaterialSlot;
            EditorMaterialComponentSlotContainer m_materialSlots;
            EditorMaterialComponentSlotsByLodContainer m_materialSlotsByLod;
            bool m_materialSlotsByLodEnabled = false;

            static const char* GenerateMaterialsButtonText;
            static const char* GenerateMaterialsToolTipText;
            static const char* ResetMaterialsButtonText;
            static const char* ResetMaterialsToolTipText;
        };
    } // namespace Editor
} // namespace JE
