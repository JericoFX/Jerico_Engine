/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once

#include "MaterialWorkspaceSession.h"
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <QWidget>

namespace AzToolsFramework::AssetBrowser { class AssetBrowserEntry; }
class QLabel;
class QComboBox;
class QPushButton;
class QVBoxLayout;
namespace JE::Editor::Tools
{
    class AtomToolsDocumentInspector;
    class EntityPreviewViewportWidget;
}

namespace JE::Editor
{
    class MaterialPreviewViewportContent;
    class JE_EDITOR_MATERIALS_API MaterialWorkspaceWidget final
        : public QWidget
        , private JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
    {
        Q_OBJECT
    public:
        static constexpr const char* PaneName = "Material Workspace";
        explicit MaterialWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent = nullptr);
        MaterialWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent, bool inspectorOnly);
        ~MaterialWorkspaceWidget() override;
        static void GetAssetInspectorWidget(MaterialWorkspaceSession& session,
            const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry, QWidget* parent,
            QWidget*& widget, bool& selectionAccepted);
        void OpenDocumentDialog();
        AZ::Uuid OpenDocument(const AZStd::string& sourcePath);
        bool FinishEditing();

    private:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void closeEvent(QCloseEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void CreatePreview();
        void RefreshPreview();
        void ReleasePreview();
        void OnSessionClosing();
        void RefreshDocuments(const AZ::Uuid& excludedDocumentId = AZ::Uuid::CreateNull());
        void RefreshActions();
        bool SaveDisplayedDocument();
        void RefreshDocumentLabel(const AZ::Uuid& documentId);
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnDocumentSaved(const AZ::Uuid& documentId) override;
        void AfterUndoRedo() override;
        void OnEndUndo(const char* label, bool changed) override;

        QPointer<MaterialWorkspaceSession> m_session;
        JE::Editor::Tools::AtomToolsDocumentInspector* m_inspector = nullptr;
        QComboBox* m_documents = nullptr;
        QWidget* m_previewHost = nullptr;
        QVBoxLayout* m_previewLayout = nullptr;
        JE::Editor::Tools::EntityPreviewViewportWidget* m_previewViewport = nullptr;
        AZStd::shared_ptr<MaterialPreviewViewportContent> m_previewContent;
        QPushButton* m_save = nullptr;
        QPushButton* m_undo = nullptr;
        QPushButton* m_redo = nullptr;
        AZ::Uuid m_displayedDocumentId = AZ::Uuid::CreateNull();
        QLabel* m_status = nullptr;
        bool m_inspectorOnly = false;
    };
}
