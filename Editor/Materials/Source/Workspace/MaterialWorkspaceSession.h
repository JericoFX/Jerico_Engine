/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once

#include <JE/Editor/Materials/Base.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentNotificationBus.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentTypeInfo.h>
#include <JE/Editor/Tools/Graph/GraphDocumentNotificationBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/unordered_map.h>
#include <QObject>
#include <QPointer>

class QWidget;
namespace JE::Editor::Tools
{
    class AtomToolsDocumentSystem;
    class EntityPreviewViewportSettingsSystem;
}


namespace JE::Editor
{
    class MaterialGraphEnvironment;
    //! Documents outlive their panes so closing a pane cannot invalidate editor undo commands.
    class JE_EDITOR_MATERIALS_API MaterialWorkspaceSession final
        : public QObject
        , private JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler
        , private JE::Editor::Tools::GraphDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        enum class CompileState { NotCompiled, Modified, Compiling, CompilingModified, Completed, Failed };
        explicit MaterialWorkspaceSession(const JE::Editor::Tools::DocumentTypeInfo& documentType);
        ~MaterialWorkspaceSession() override;

        static constexpr AZ::Crc32 GetToolId() { return AZ_CRC_CE("JE.Editor.MaterialWorkspace"); }
        const AZStd::vector<AZ::Uuid>& GetDocumentIds() const;
        const AZ::Uuid& GetActiveDocumentId() const;
        const JE::Editor::Tools::DocumentTypeInfo& GetDocumentType() const;
        const JE::Editor::Tools::DocumentTypeInfoVector& GetGraphDocumentTypes() const;
        void RegisterGraphDocumentTypes(const JE::Editor::Tools::DocumentTypeInfoVector& types);
        MaterialGraphEnvironment& EnableGraphDocuments();
        //! Called only for an explicit Graph Workspace request, before its pane is opened.
        //! An explicit path consumes the default decision; the existing view route opens that path.
        void PrepareGraphWorkspace(const AZStd::string& sourcePath);
        void EnsureMaterialPreviewSettings();
        bool IsGraphDocument(const AZ::Uuid& documentId) const;
        AZ::Uuid OpenDocument(const AZStd::string& sourcePath);
        AZ::Uuid CreateDocument(const AZStd::string& sourcePath, const AZStd::string& targetPath);
        bool SelectDocument(const AZ::Uuid& documentId);
        bool SaveActiveDocument();
        void SetMainWindow(QWidget* mainWindow);
        bool ConfirmApplicationClose(QWidget* parent);
        CompileState GetCompileState(const AZ::Uuid& documentId) const;
        bool RequestCompile(const AZ::Uuid& documentId);

    Q_SIGNALS:
        // Views release renderer-backed resources synchronously before documents and settings are destroyed.
        void AboutToClose();
        // Successful explicit selection changes; selecting the active document again does not emit.
        void DocumentSelected(const AZ::Uuid& documentId);
        // Document/compiler notifications are dispatched on the main thread; observers see the updated session state.
        void CompileStateChanged(const AZ::Uuid& documentId);

    private:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void OnDocumentCreated(const AZ::Uuid& documentId) override;
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnCompileGraphStarted(const AZ::Uuid& documentId) override;
        void OnCompileGraphCompleted(const AZ::Uuid& documentId) override;
        void OnCompileGraphFailed(const AZ::Uuid& documentId) override;
        void SetCompileState(const AZ::Uuid& documentId, CompileState state);

        JE::Editor::Tools::DocumentTypeInfo m_documentType;
        JE::Editor::Tools::DocumentTypeInfoVector m_graphDocumentTypes;
        AZStd::unique_ptr<JE::Editor::Tools::EntityPreviewViewportSettingsSystem> m_previewSettings;
        AZStd::unique_ptr<MaterialGraphEnvironment> m_graphEnvironment;
        AZStd::unique_ptr<JE::Editor::Tools::AtomToolsDocumentSystem> m_documentSystem;
        AZStd::vector<AZ::Uuid> m_documentIds;
        AZStd::unordered_map<AZ::Uuid, CompileState> m_compileStates;
        AZ::Uuid m_activeDocumentId = AZ::Uuid::CreateNull();
        QPointer<QWidget> m_mainWindow;
        bool m_confirmingClose = false;
        bool m_graphWorkspaceOpened = false;
    };
}
