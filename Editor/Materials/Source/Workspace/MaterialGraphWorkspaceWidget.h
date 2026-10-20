/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once
#include "MaterialWorkspaceSession.h"
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <QWidget>

class QComboBox;
namespace GraphCanvas { class NodePaletteWidget; }
class QLabel;
class QPushButton;
class QVBoxLayout;
namespace JE::Editor::Tools
{
    class AtomToolsDocumentInspector;
    class GraphDocumentView;
}

namespace JE::Editor
{
    class JE_EDITOR_MATERIALS_API MaterialGraphWorkspaceWidget final
        : public QWidget
        , private JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
    {
        Q_OBJECT
    public:
        static constexpr const char* PaneName = "Material Graph Workspace";
        explicit MaterialGraphWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent = nullptr);
        ~MaterialGraphWorkspaceWidget() override;
        AZ::Uuid OpenDocument(const AZStd::string& sourcePath);

    private:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void closeEvent(QCloseEvent* event) override;
        void NewDocumentDialog();
        void OpenDocumentDialog();
        void FinishEditing();
        void RefreshDocuments(const AZ::Uuid& excludedDocumentId = AZ::Uuid::CreateNull());
        void RefreshDocumentLabel(const AZ::Uuid& id);
        void SelectDocument(const AZ::Uuid& id);
        void RefreshActions();
        void CompileDocument();
        void OpenGeneratedMaterial();
        void OnDocumentOpened(const AZ::Uuid& id) override;
        void OnDocumentClosed(const AZ::Uuid& id) override;
        void OnDocumentDestroyed(const AZ::Uuid& id) override;
        void OnDocumentModified(const AZ::Uuid& id) override;
        void OnDocumentSaved(const AZ::Uuid& id) override;
        void AfterUndoRedo() override;
        void OnEndUndo(const char*, bool) override;

        QPointer<MaterialWorkspaceSession> m_session;
        JE::Editor::Tools::AtomToolsDocumentInspector* m_inspector = nullptr;
        JE::Editor::Tools::GraphDocumentView* m_graphView = nullptr;
        QVBoxLayout* m_viewLayout = nullptr;
        QWidget* m_propertiesMessage = nullptr;
        QComboBox* m_documents = nullptr;
        GraphCanvas::NodePaletteWidget* m_palette = nullptr;
        QLabel* m_compileStatus = nullptr;
        QPushButton* m_save = nullptr;
        QPushButton* m_compile = nullptr;
        QPushButton* m_openMaterial = nullptr;
        QPushButton* m_undo = nullptr;
        QPushButton* m_redo = nullptr;
        AZ::Uuid m_displayedDocumentId = AZ::Uuid::CreateNull();
    };
}
