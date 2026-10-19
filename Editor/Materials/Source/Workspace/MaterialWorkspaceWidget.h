/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once

#include "MaterialWorkspaceSession.h"
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <QWidget>

class QComboBox;
class QPushButton;
class QVBoxLayout;
namespace AtomToolsFramework
{
    class AtomToolsDocumentInspector;
    class EntityPreviewViewportWidget;
}

namespace JE::Editor
{
    class MaterialPreviewViewportContent;
    class JE_EDITOR_MATERIALS_API MaterialWorkspaceWidget final
        : public QWidget
        , private AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
    {
        Q_OBJECT
    public:
        static constexpr const char* PaneName = "Material Workspace";
        explicit MaterialWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent = nullptr);
        ~MaterialWorkspaceWidget() override;
        void OpenDocumentDialog();
        AZ::Uuid OpenDocument(const AZStd::string& sourcePath);

    private:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void closeEvent(QCloseEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void CreatePreview();
        void RefreshPreview();
        void ReleasePreview();
        void OnSessionClosing();
        void FinishEditing();
        void RefreshDocuments(const AZ::Uuid& excludedDocumentId = AZ::Uuid::CreateNull());
        void RefreshActions();
        void RefreshDocumentLabel(const AZ::Uuid& documentId);
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnDocumentSaved(const AZ::Uuid& documentId) override;
        void AfterUndoRedo() override;
        void OnEndUndo(const char* label, bool changed) override;

        QPointer<MaterialWorkspaceSession> m_session;
        AtomToolsFramework::AtomToolsDocumentInspector* m_inspector = nullptr;
        QComboBox* m_documents = nullptr;
        QWidget* m_previewHost = nullptr;
        QVBoxLayout* m_previewLayout = nullptr;
        AtomToolsFramework::EntityPreviewViewportWidget* m_previewViewport = nullptr;
        AZStd::shared_ptr<MaterialPreviewViewportContent> m_previewContent;
        QPushButton* m_save = nullptr;
        QPushButton* m_undo = nullptr;
        QPushButton* m_redo = nullptr;
        AZ::Uuid m_displayedDocumentId = AZ::Uuid::CreateNull();
    };
}
