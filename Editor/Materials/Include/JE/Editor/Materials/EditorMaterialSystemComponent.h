/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Materials/Base.h>
#if !defined(AZ_RELEASE_BUILD)
#include <AzCore/Console/IConsole.h>
#endif

#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>
#include <JE/Render/RPI/Reflect/Model/ModelAsset.h>
#include <JE/Render/RPI/Reflect/System/AnyAsset.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponentNotificationBus.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponentRequestBus.h>
#include <JE/Render/Materials/MaterialComponentBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <JE/Editor/Materials/MaterialBrowserInteractions.h>
#include <QPixmap>


namespace JE
{
    namespace Editor
    {
        class MaterialWorkspaceSession;

        //! System component that owns the native material workspace session.
        class JE_EDITOR_MATERIALS_API EditorMaterialSystemComponent final
            : public AZ::Component
            , public AZ::EntitySystemBus::Handler
            , public EditorMaterialSystemComponentNotificationBus::Handler
            , public EditorMaterialSystemComponentRequestBus::Handler
            , public JE::Render::MaterialComponentNotificationBus::Router
            , public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
            , private AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler
            , public AzToolsFramework::EditorEvents::Bus::Handler
            , public AzToolsFramework::ToolsApplicationNotificationBus::Handler
            , public AZ::SystemTickBus::Handler
            , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(EditorMaterialSystemComponent, "{96652157-DA0B-420F-B49C-0207C585144C}");

            EditorMaterialSystemComponent();
            ~EditorMaterialSystemComponent() override;

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Init() override;
            void Activate() override;
            void Deactivate() override;

        private:
#if !defined(AZ_RELEASE_BUILD)
            void JeOpenMaterialGraph(const AZ::ConsoleCommandContainer& arguments);
            AZ_CONSOLEFUNC(EditorMaterialSystemComponent, JeOpenMaterialGraph, AZ::ConsoleFunctorFlags::DontReplicate,
                "Editor diagnostic: <absolute .materialgraph source path>, one token without whitespace; paired double quotes allowed. "
                "Open through the native workspace; does not confirm compilation or preview.");
            void JeCompileGraphAndSelect(const AZ::ConsoleCommandContainer& arguments);
            AZ_CONSOLEFUNC(EditorMaterialSystemComponent, JeCompileGraphAndSelect, AZ::ConsoleFunctorFlags::DontReplicate,
                "Editor diagnostic: <A document UUID> <B document UUID>. Compile selected A and select already-open B without pumping events.");
#endif
            //! EditorMaterialSystemComponentRequestBus::Handler overrides...
            void OpenMaterialEditor(const AZStd::string& sourcePath) override;
            void OpenMaterialCanvas(const AZStd::string& sourcePath) override;
            void OpenMaterialInspector(
                const AZ::EntityId& primaryEntityId,
                const AzToolsFramework::EntityIdSet& entityIdsToEdit,
                const JE::Render::MaterialAssignmentId& materialAssignmentId) override;
            void RenderMaterialPreview(const AZ::EntityId& entityId, const JE::Render::MaterialAssignmentId& materialAssignmentId) override;
            QPixmap GetRenderedMaterialPreview(
                const AZ::EntityId& entityId, const JE::Render::MaterialAssignmentId& materialAssignmentId) const override;

            // AZ::EntitySystemBus::Handler overrides...
            void OnEntityDeactivated(const AZ::EntityId& entityId) override;

            //! AZ::SystemTickBus::Handler interface overrides...
            void OnSystemTick() override;

            //! EditorMaterialSystemComponentNotificationBus::Handler overrides...
            void OnRenderMaterialPreviewRendered(
                const AZ::EntityId& entityId, const JE::Render::MaterialAssignmentId& materialAssignmentId, const QPixmap& pixmap) override;

            //! MaterialComponentNotificationBus::Router overrides...
            void OnMaterialSlotLayoutChanged() override;

            //! AssetBrowserInteractionNotificationBus::Handler overrides...
            AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
            void GetAssetInspectorWidget(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry,
                QWidget* parent, QWidget*& widget, bool& selectionAccepted) override;

            // AztoolsFramework::EditorEvents::Bus::Handler overrides...
            void NotifyRegisterViews() override;
            void NotifyMainWindowInitialized(QMainWindow* mainWindow) override;

            // AzToolsFramework::ToolsApplicationNotificationBus::Handler overrides...
            void AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList&) override;

            // AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler...
            void OnActionRegistrationHook() override;
            void OnMenuBindingHook() override;

            void PurgePreviews();

            AZStd::unique_ptr<MaterialWorkspaceSession> m_workspaceSession;
            bool m_workspacePaneRegistered = false;
            bool m_graphWorkspacePaneRegistered = false;
            AZStd::unique_ptr<MaterialBrowserInteractions> m_materialBrowserInteractions;
            AZStd::unordered_set<AZStd::pair<AZ::EntityId, JE::Render::MaterialAssignmentId>> m_materialPreviewRequests;
            AZStd::unordered_map<AZ::EntityId, AZStd::unordered_map<JE::Render::MaterialAssignmentId, QPixmap>> m_materialPreviews;
            AZ::Data::Asset<JE::Render::RPI::ModelAsset> m_materialPreviewModelAsset;
            AZ::Data::Asset<JE::Render::RPI::AnyAsset> m_materialPreviewLightingPresetAsset;
            static constexpr const size_t MaterialPreviewLimit = 100;
            static constexpr const int MaterialPreviewResolution = 128;
        };
    } // namespace Editor
} // namespace JE
