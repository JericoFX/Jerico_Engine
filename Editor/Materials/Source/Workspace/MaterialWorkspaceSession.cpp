/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "MaterialWorkspaceSession.h"
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystem.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
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
        AtomToolsFramework::GraphDocumentNotificationBus::Handler::BusConnect(GetToolId());
    }

    MaterialWorkspaceSession::~MaterialWorkspaceSession()
    {
        SetMainWindow(nullptr);
        // Notify inspectors before their document-backed property objects are released.
        m_documentSystem->CloseAllDocuments();
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::GraphDocumentNotificationBus::Handler::BusDisconnect();
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

    const AtomToolsFramework::DocumentTypeInfoVector& MaterialWorkspaceSession::GetGraphDocumentTypes() const
    {
        return m_graphDocumentTypes;
    }

    void MaterialWorkspaceSession::RegisterGraphDocumentTypes(const AtomToolsFramework::DocumentTypeInfoVector& types)
    {
        for (const auto& type : types)
        {
            const bool registered = AZStd::any_of(m_graphDocumentTypes.begin(), m_graphDocumentTypes.end(),
                [&type](const auto& existing) { return existing.m_documentTypeName == type.m_documentTypeName; });
            if (!registered)
            {
                m_documentSystem->RegisterDocumentType(type);
                m_graphDocumentTypes.push_back(type);
            }
        }
    }

    MaterialGraphEnvironment& MaterialWorkspaceSession::EnableGraphDocuments()
    {
        if (!m_graphEnvironment)
        {
            m_graphEnvironment = AZStd::make_unique<MaterialGraphEnvironment>(GetToolId());
            RegisterGraphDocumentTypes(m_graphEnvironment->BuildDocumentTypes(MaterialGraphEnvironment::HistoryOwner::Editor));
        }
        return *m_graphEnvironment;
    }

    bool MaterialWorkspaceSession::IsGraphDocument(const AZ::Uuid& documentId) const
    {
        AtomToolsFramework::DocumentTypeInfo type;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            type, documentId, &AtomToolsFramework::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        return AZStd::any_of(m_graphDocumentTypes.begin(), m_graphDocumentTypes.end(),
            [&type](const auto& existing) { return existing.m_documentTypeName == type.m_documentTypeName; });
    }

    AZ::Uuid MaterialWorkspaceSession::OpenDocument(const AZStd::string& sourcePath)
    {
        return m_documentSystem->OpenDocument(sourcePath);
    }

    AZ::Uuid MaterialWorkspaceSession::CreateDocument(const AZStd::string& sourcePath, const AZStd::string& targetPath)
    {
        // Creation selects a stable destination before any edits can enter the editor's global history.
        AZStd::string source = sourcePath;
        AZStd::string target = targetPath;
        if (!AZ::IO::PathView(target).IsAbsolute() || !AtomToolsFramework::ValidateDocumentPath(target) ||
            AZ::IO::SystemFile::Exists(target.c_str()) ||
            (!source.empty() && !AtomToolsFramework::ValidateDocumentPath(source)))
        {
            return AZ::Uuid::CreateNull();
        }
        const AZ::IO::Path targetFile(target);
        AZStd::string temporaryName = ".je-create-" + AZ::Uuid::CreateRandom().ToString<AZStd::string>();
        temporaryName += targetFile.Extension().Native();
        const auto temporary = (AZ::IO::Path(targetFile.ParentPath()) / temporaryName).Native();
        AZ::IO::SystemFile reservation;
        if (!reservation.Open(temporary.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE_NEW))
        {
            return AZ::Uuid::CreateNull();
        }
        reservation.Close();

        const auto id = m_documentSystem->CreateDocumentFromFileType(source.empty() ? target : source);
        bool success = !id.IsNull();
        bool published = false;
        if (success && !source.empty())
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                success, id, &AtomToolsFramework::AtomToolsDocumentRequests::Open, source);
        }
        if (success)
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                success, id, &AtomToolsFramework::AtomToolsDocumentRequests::SaveAsChild, temporary);
        }
        if (success)
        {
            published = AZ::IO::SystemFile::Rename(temporary.c_str(), target.c_str(), false);
            success = published;
        }
        if (success)
        {
            // No modal dialog or event loop between temporary save and final open: queued graph compilation sees only the final path.
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                success, id, &AtomToolsFramework::AtomToolsDocumentRequests::Open, target);
        }
        if (!success)
        {
            if (!id.IsNull())
            {
                AtomToolsFramework::AtomToolsDocumentRequestBus::Event(id, &AtomToolsFramework::AtomToolsDocumentRequests::Close);
                m_documentSystem->DestroyDocument(id);
            }
            AZ::IO::SystemFile::Delete(temporary.c_str());
            // A published file remains recoverable if reopening fails; never remove the final destination here.
            return AZ::Uuid::CreateNull();
        }
        m_documentSystem->AddRecentFilePath(target);
        return id;
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

    MaterialWorkspaceSession::CompileState MaterialWorkspaceSession::GetCompileState(const AZ::Uuid& documentId) const
    {
        const auto state = m_compileStates.find(documentId);
        return state == m_compileStates.end() ? CompileState::NotCompiled : state->second;
    }

    bool MaterialWorkspaceSession::RequestCompile(const AZ::Uuid& documentId)
    {
        if (AZStd::find(m_documentIds.begin(), m_documentIds.end(), documentId) == m_documentIds.end() || !IsGraphDocument(documentId))
        {
            return false;
        }
        bool started = false;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(started, documentId, &AtomToolsFramework::GraphDocumentRequests::CompileGraph);
        if (started)
        {
            // Job acceptance precedes its main-thread Started notification; it is not compilation success.
            SetCompileState(documentId, CompileState::Compiling);
        }
        return started;
    }

    void MaterialWorkspaceSession::SetCompileState(const AZ::Uuid& documentId, CompileState state)
    {
        // Jobs can finish after their document is destroyed. Do not recreate state for an unowned id.
        if (AZStd::find(m_documentIds.begin(), m_documentIds.end(), documentId) != m_documentIds.end() && IsGraphDocument(documentId))
        {
            m_compileStates[documentId] = state;
            Q_EMIT CompileStateChanged(documentId);
        }
    }

    void MaterialWorkspaceSession::OnDocumentModified(const AZ::Uuid& documentId)
    {
        const auto state = GetCompileState(documentId);
        const bool compiling = state == CompileState::Compiling || state == CompileState::CompilingModified;
        SetCompileState(documentId, compiling ? CompileState::CompilingModified : CompileState::Modified);
    }

    void MaterialWorkspaceSession::OnCompileGraphStarted(const AZ::Uuid& documentId)
    {
        if (GetCompileState(documentId) != CompileState::CompilingModified)
        {
            SetCompileState(documentId, CompileState::Compiling);
        }
    }

    void MaterialWorkspaceSession::OnCompileGraphCompleted(const AZ::Uuid& documentId)
    {
        SetCompileState(documentId, GetCompileState(documentId) == CompileState::CompilingModified
            ? CompileState::Modified : CompileState::Completed);
    }

    void MaterialWorkspaceSession::OnCompileGraphFailed(const AZ::Uuid& documentId)
    {
        SetCompileState(documentId, CompileState::Failed);
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
        m_compileStates.erase(documentId);
        OnDocumentClosed(documentId);
    }
}
