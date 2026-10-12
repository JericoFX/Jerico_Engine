/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once

#include <JE/Editor/Materials/Base.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <QObject>
#include <QPointer>

class QWidget;
namespace AtomToolsFramework
{
    class AtomToolsDocumentSystem;
}

namespace JE::Editor
{
    //! Documents outlive their panes so closing a pane cannot invalidate editor undo commands.
    class JE_EDITOR_MATERIALS_API MaterialWorkspaceSession final
        : public QObject
        , private AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
    public:
        explicit MaterialWorkspaceSession(const AtomToolsFramework::DocumentTypeInfo& documentType);
        ~MaterialWorkspaceSession() override;

        static constexpr AZ::Crc32 GetToolId() { return AZ_CRC_CE("JE.Editor.MaterialWorkspace"); }
        const AZStd::vector<AZ::Uuid>& GetDocumentIds() const;
        const AZ::Uuid& GetActiveDocumentId() const;
        const AtomToolsFramework::DocumentTypeInfo& GetDocumentType() const;
        AZ::Uuid OpenDocument(const AZStd::string& sourcePath);
        bool SelectDocument(const AZ::Uuid& documentId);
        bool SaveActiveDocument();
        void SetMainWindow(QWidget* mainWindow);
        bool ConfirmApplicationClose(QWidget* parent);

    private:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void OnDocumentCreated(const AZ::Uuid& documentId) override;
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;

        AtomToolsFramework::DocumentTypeInfo m_documentType;
        AZStd::unique_ptr<AtomToolsFramework::AtomToolsDocumentSystem> m_documentSystem;
        AZStd::vector<AZ::Uuid> m_documentIds;
        AZ::Uuid m_activeDocumentId = AZ::Uuid::CreateNull();
        QPointer<QWidget> m_mainWindow;
        bool m_confirmingClose = false;
    };
}
