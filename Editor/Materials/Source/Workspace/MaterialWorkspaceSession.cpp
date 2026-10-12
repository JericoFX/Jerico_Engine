/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "MaterialWorkspaceSession.h"

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystem.h>
#include <AzCore/std/algorithm.h>
#include <QCloseEvent>
#include <QMessageBox>
#include <QWidget>

namespace JE::Editor
{
    MaterialWorkspaceSession::MaterialWorkspaceSession(const AtomToolsFramework::DocumentTypeInfo& documentType)
        : m_documentType(documentType)
        , m_documentSystem(AZStd::make_unique<AtomToolsFramework::AtomToolsDocumentSystem>(GetToolId()))
    {
        m_documentSystem->RegisterDocumentType(m_documentType);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(GetToolId());
    }

    MaterialWorkspaceSession::~MaterialWorkspaceSession()
    {
        SetMainWindow(nullptr);
        // Notify inspectors before their document-backed property objects are released.
        m_documentSystem->CloseAllDocuments();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        m_documentSystem.reset();
    }

    const AZStd::vector<AZ::Uuid>& MaterialWorkspaceSession::GetDocumentIds() const
    {
        return m_documentIds;
    }

    const AZ::Uuid& MaterialWorkspaceSession::GetActiveDocumentId() const
    {
        return m_activeDocumentId;
    }

    const AtomToolsFramework::DocumentTypeInfo& MaterialWorkspaceSession::GetDocumentType() const
    {
        return m_documentType;
    }

    AZ::Uuid MaterialWorkspaceSession::OpenDocument(const AZStd::string& sourcePath)
    {
        return m_documentSystem->OpenDocument(sourcePath);
    }

    bool MaterialWorkspaceSession::SelectDocument(const AZ::Uuid& documentId)
    {
        if (!m_documentSystem->IsDocumentOpen(documentId))
        {
            return false;
        }
        m_activeDocumentId = documentId;
        return true;
    }

    bool MaterialWorkspaceSession::SaveActiveDocument()
    {
        return !m_activeDocumentId.IsNull() && m_documentSystem->SaveDocument(m_activeDocumentId);
    }

    void MaterialWorkspaceSession::SetMainWindow(QWidget* mainWindow)
    {
        if (m_mainWindow)
        {
            m_mainWindow->removeEventFilter(this);
        }
        m_mainWindow = mainWindow;
        if (m_mainWindow)
        {
            m_mainWindow->installEventFilter(this);
        }
    }

    bool MaterialWorkspaceSession::ConfirmApplicationClose(QWidget* parent)
    {
        const auto documentIds = m_documentIds;
        for (const auto& documentId : documentIds)
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                documentId, &AtomToolsFramework::AtomToolsDocumentRequests::EndEdit);
            bool modified = false;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                modified, documentId, &AtomToolsFramework::AtomToolsDocumentRequests::IsModified);
            if (!modified)
            {
                continue;
            }
            AZStd::string path;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                path, documentId, &AtomToolsFramework::AtomToolsDocumentRequests::GetAbsolutePath);
            const auto decision = QMessageBox::question(parent, tr("Material Changes"),
                tr("Save changes to %1 before closing the Editor?").arg(QString::fromUtf8(path.c_str())),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
            if (decision == QMessageBox::Cancel)
            {
                return false;
            }
            if (decision == QMessageBox::Save && !m_documentSystem->SaveDocument(documentId))
            {
                return false;
            }
            // Discard only authorizes this close attempt. Another pane can still cancel shutdown.
            // Keep the document and undo history unchanged until the session actually ends.
        }
        return true;
    }

    bool MaterialWorkspaceSession::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == m_mainWindow && event->type() == QEvent::Close && !m_confirmingClose)
        {
            m_confirmingClose = true;
            const bool accepted = ConfirmApplicationClose(m_mainWindow);
            m_confirmingClose = false;
            if (!accepted)
            {
                static_cast<QCloseEvent*>(event)->ignore();
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }

    void MaterialWorkspaceSession::OnDocumentCreated(const AZ::Uuid& documentId)
    {
        m_documentIds.push_back(documentId);
    }

    void MaterialWorkspaceSession::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        m_activeDocumentId = documentId;
    }

    void MaterialWorkspaceSession::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        if (m_activeDocumentId == documentId)
        {
            m_activeDocumentId = AZ::Uuid::CreateNull();
        }
    }

    void MaterialWorkspaceSession::OnDocumentDestroyed(const AZ::Uuid& documentId)
    {
        m_documentIds.erase(AZStd::remove(m_documentIds.begin(), m_documentIds.end(), documentId), m_documentIds.end());
        OnDocumentClosed(documentId);
    }
}
