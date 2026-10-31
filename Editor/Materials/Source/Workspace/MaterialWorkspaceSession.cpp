/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "MaterialWorkspaceSession.h"
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>

#include <JE/Editor/Tools/Document/AtomToolsDocumentRequestBus.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentSystem.h>
#include <JE/Editor/Tools/Graph/GraphDocumentRequestBus.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <QCloseEvent>
#include <QMessageBox>
#include <QWidget>

namespace JE::Editor
{
    MaterialWorkspaceSession::MaterialWorkspaceSession(const JE::Editor::Tools::DocumentTypeInfo& documentType)
        : m_documentType(documentType)
        , m_documentSystem(AZStd::make_unique<JE::Editor::Tools::AtomToolsDocumentSystem>(GetToolId()))
    {
        m_documentSystem->RegisterDocumentType(m_documentType);
        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler::BusConnect(GetToolId());
        JE::Editor::Tools::GraphDocumentNotificationBus::Handler::BusConnect(GetToolId());
    }

    MaterialWorkspaceSession::~MaterialWorkspaceSession()
    {
        emit AboutToClose();
        SetMainWindow(nullptr);
        // Notify inspectors before their document-backed property objects are released.
        m_documentSystem->CloseAllDocuments();
        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        JE::Editor::Tools::GraphDocumentNotificationBus::Handler::BusDisconnect();
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

    const JE::Editor::Tools::DocumentTypeInfo& MaterialWorkspaceSession::GetDocumentType() const
    {
        return m_documentType;
    }

    const JE::Editor::Tools::DocumentTypeInfoVector& MaterialWorkspaceSession::GetGraphDocumentTypes() const
    {
        return m_graphDocumentTypes;
    }

    void MaterialWorkspaceSession::RegisterGraphDocumentTypes(const JE::Editor::Tools::DocumentTypeInfoVector& types)
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

    void MaterialWorkspaceSession::PrepareGraphWorkspace(const AZStd::string& sourcePath)
    {
        const bool firstOpen = !m_graphWorkspaceOpened;
        m_graphWorkspaceOpened = true;
        if (!firstOpen || !sourcePath.empty() || !m_documentIds.empty() || !m_activeDocumentId.IsNull() ||
            !JE::Editor::Tools::GetSettingsValue("/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart", true))
        {
            return;
        }

        EnableGraphDocuments();
        const AZ::Uuid id = m_documentSystem->CreateDocumentFromTypeName("Material Graph");
        if (!id.IsNull())
        {
            JE::Editor::Tools::AtomToolsDocumentNotificationBus::Event(GetToolId(),
                &JE::Editor::Tools::AtomToolsDocumentNotifications::OnDocumentOpened, id);
        }
    }

    void MaterialWorkspaceSession::EnsureMaterialPreviewSettings()
    {
        if (!JE::Editor::Tools::EntityPreviewViewportSettingsRequestBus::HasHandlers(GetToolId()))
        {
            m_previewSettings = AZStd::make_unique<JE::Editor::Tools::EntityPreviewViewportSettingsSystem>(GetToolId());
        }
    }

    bool MaterialWorkspaceSession::IsGraphDocument(const AZ::Uuid& documentId) const
    {
        JE::Editor::Tools::DocumentTypeInfo type;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            type, documentId, &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
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
        if (!AZ::IO::PathView(target).IsAbsolute() || !JE::Editor::Tools::ValidateDocumentPath(target) ||
            AZ::IO::SystemFile::Exists(target.c_str()) ||
            (!source.empty() && !JE::Editor::Tools::ValidateDocumentPath(source)))
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
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::Open, source);
        }
        if (success)
        {
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::SaveAsChild, temporary);
        }
        if (success)
        {
            published = AZ::IO::SystemFile::Rename(temporary.c_str(), target.c_str(), false);
            success = published;
        }
        if (success)
        {
            // No modal dialog or event loop between temporary save and final open: queued graph compilation sees only the final path.
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::Open, target);
        }
        if (!success)
        {
            if (!id.IsNull())
            {
                JE::Editor::Tools::AtomToolsDocumentRequestBus::Event(id, &JE::Editor::Tools::AtomToolsDocumentRequests::Close);
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
        if (m_activeDocumentId != documentId)
        {
            m_activeDocumentId = documentId;
            emit DocumentSelected(documentId);
        }
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
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(started, documentId, &JE::Editor::Tools::GraphDocumentRequests::CompileGraph);
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
            JE::Editor::Tools::AtomToolsDocumentRequestBus::Event(
                documentId, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
            bool modified = false;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                modified, documentId, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
            if (!modified)
            {
                continue;
            }
            AZStd::string path;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                path, documentId, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
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
