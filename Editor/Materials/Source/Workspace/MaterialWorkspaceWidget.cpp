/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "MaterialWorkspaceWidget.h"

#include <AtomToolsFramework/Document/AtomToolsDocumentInspector.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace JE::Editor
{
    MaterialWorkspaceWidget::MaterialWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent)
        : QWidget(parent)
        , m_session(&session)
    {
        auto* layout = new QVBoxLayout(this);
        auto* actions = new QHBoxLayout();
        auto* open = new QPushButton(tr("Open"), this);
        m_save = new QPushButton(tr("Save"), this);
        m_undo = new QPushButton(tr("Undo"), this);
        m_redo = new QPushButton(tr("Redo"), this);
        actions->addWidget(open);
        actions->addWidget(m_save);
        actions->addWidget(m_undo);
        actions->addWidget(m_redo);
        actions->addStretch();
        layout->addLayout(actions);
        m_documents = new QComboBox(this);
        layout->addWidget(m_documents);
        m_inspector = new AtomToolsFramework::AtomToolsDocumentInspector(MaterialWorkspaceSession::GetToolId(), this);
        m_inspector->SetDocumentSettingsPrefix("/JE/Editor/MaterialWorkspace/Inspector");
        layout->addWidget(m_inspector);

        connect(open, &QPushButton::clicked, this, [this]() { OpenDocumentDialog(); });
        connect(m_save, &QPushButton::clicked, this, [this]() {
            FinishEditing();
            if (m_session && m_session->SelectDocument(m_displayedDocumentId))
            {
                m_session->SaveActiveDocument();
            }
        });
        connect(m_undo, &QPushButton::clicked, this, [this]() {
            FinishEditing();
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);
        });
        connect(m_redo, &QPushButton::clicked, this, [this]() {
            FinishEditing();
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);
        });
        connect(m_documents, &QComboBox::currentIndexChanged, this, [this](int index) {
            if (!m_session || index < 0)
            {
                return;
            }
            FinishEditing();
            const auto id = AZ::Uuid::CreateString(m_documents->itemData(index).toString().toUtf8().constData());
            if (m_session->SelectDocument(id))
            {
                m_displayedDocumentId = id;
                m_inspector->SetDocumentId(id);
            }
            RefreshActions();
        });
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(MaterialWorkspaceSession::GetToolId());
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        qApp->installEventFilter(this);
        RefreshDocuments();
    }

    MaterialWorkspaceWidget::~MaterialWorkspaceWidget()
    {
        qApp->removeEventFilter(this);
        FinishEditing();
        m_inspector->SetDocumentId(AZ::Uuid::CreateNull());
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
    }

    void MaterialWorkspaceWidget::OpenDocumentDialog()
    {
        if (!m_session)
        {
            return;
        }
        FinishEditing();
        const auto& type = m_session->GetDocumentType();
        const auto paths = AtomToolsFramework::GetOpenFilePathsFromDialog(
            {}, type.m_supportedExtensionsToOpen, type.m_documentTypeName, false);
        if (!paths.empty())
        {
            OpenDocument(paths.front());
        }
    }

    AZ::Uuid MaterialWorkspaceWidget::OpenDocument(const AZStd::string& sourcePath)
    {
        FinishEditing();
        return m_session ? m_session->OpenDocument(sourcePath) : AZ::Uuid::CreateNull();
    }

    void MaterialWorkspaceWidget::FinishEditing()
    {
        if (auto* focus = QApplication::focusWidget(); focus && (focus == this || isAncestorOf(focus)))
        {
            focus->clearFocus();
        }
        if (!m_displayedDocumentId.IsNull())
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::Event(
                m_displayedDocumentId, &AtomToolsFramework::AtomToolsDocumentRequests::EndEdit);
        }
    }

    void MaterialWorkspaceWidget::closeEvent(QCloseEvent* event)
    {
        FinishEditing();
        // Only the view closes. The session retains the document and its global undo references.
        event->accept();
    }

    bool MaterialWorkspaceWidget::eventFilter(QObject* watched, QEvent* event)
    {
        auto* widget = qobject_cast<QWidget*>(watched);
        if (widget && (widget == this || isAncestorOf(widget)) &&
            (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress))
        {
            auto* key = static_cast<QKeyEvent*>(event);
            const bool open = key->matches(QKeySequence::Open);
            const bool save = key->matches(QKeySequence::Save);
            const bool undo = key->matches(QKeySequence::Undo);
            const bool redo = key->matches(QKeySequence::Redo);
            if (open || save || undo || redo)
            {
                key->accept();
                if (event->type() == QEvent::KeyPress)
                {
                    if (open)
                    {
                        OpenDocumentDialog();
                    }
                    else if (undo || redo)
                    {
                        FinishEditing();
                        if (undo)
                        {
                            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                                &AzToolsFramework::ToolsApplicationRequests::UndoPressed);
                        }
                        else
                        {
                            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                                &AzToolsFramework::ToolsApplicationRequests::RedoPressed);
                        }
                    }
                    else if (m_session && m_session->SelectDocument(m_displayedDocumentId))
                    {
                        FinishEditing();
                        m_session->SaveActiveDocument();
                    }
                }
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void MaterialWorkspaceWidget::RefreshDocuments(const AZ::Uuid& excludedDocumentId)
    {
        const QSignalBlocker blocker(m_documents);
        m_documents->clear();
        const auto previousId = m_displayedDocumentId;
        m_displayedDocumentId = AZ::Uuid::CreateNull();
        if (m_session)
        {
            for (const auto& id : m_session->GetDocumentIds())
            {
                if (id == excludedDocumentId || m_session->IsGraphDocument(id))
                {
                    continue;
                }
                bool open = false;
                AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                    open, id, &AtomToolsFramework::AtomToolsDocumentRequests::IsOpen);
                if (!open)
                {
                    continue;
                }
                AZStd::string path;
                bool modified = false;
                AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                    path, id, &AtomToolsFramework::AtomToolsDocumentRequests::GetAbsolutePath);
                AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                    modified, id, &AtomToolsFramework::AtomToolsDocumentRequests::IsModified);
                auto label = QFileInfo(QString::fromUtf8(path.c_str())).fileName();
                if (modified)
                {
                    label += " *";
                }
                m_documents->addItem(label, QString::fromUtf8(id.ToString<AZStd::string>().c_str()));
                m_documents->setItemData(m_documents->count() - 1, QString::fromUtf8(path.c_str()), Qt::ToolTipRole);
                if (id == m_session->GetActiveDocumentId() ||
                    (m_session->IsGraphDocument(m_session->GetActiveDocumentId()) && id == previousId))
                {
                    m_documents->setCurrentIndex(m_documents->count() - 1);
                    m_displayedDocumentId = id;
                }
            }
        }
        if (m_displayedDocumentId.IsNull() && m_documents->count() > 0)
        {
            m_documents->setCurrentIndex(0);
            m_displayedDocumentId = AZ::Uuid::CreateString(m_documents->itemData(0).toString().toUtf8().constData());
        }
        m_inspector->SetDocumentId(m_displayedDocumentId);
        RefreshActions();
    }

    void MaterialWorkspaceWidget::RefreshActions()
    {
        m_save->setEnabled(m_session && !m_displayedDocumentId.IsNull());
        AzToolsFramework::UndoSystem::UndoStack* stack = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            stack, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);
        m_undo->setEnabled(stack && stack->CanUndo());
        m_redo->setEnabled(stack && stack->CanRedo());
    }

    void MaterialWorkspaceWidget::RefreshDocumentLabel(const AZ::Uuid& id)
    {
        const int index = m_documents->findData(QString::fromUtf8(id.ToString<AZStd::string>().c_str()));
        if (index >= 0)
        {
            AZStd::string path;
            bool modified = false;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                path, id, &AtomToolsFramework::AtomToolsDocumentRequests::GetAbsolutePath);
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                modified, id, &AtomToolsFramework::AtomToolsDocumentRequests::IsModified);
            m_documents->setItemText(index, QFileInfo(QString::fromUtf8(path.c_str())).fileName() + (modified ? " *" : ""));
        }
    }

    void MaterialWorkspaceWidget::OnDocumentOpened(const AZ::Uuid& id)
    {
        if (m_session && m_session->IsGraphDocument(id))
        {
            return;
        }
        if (m_session)
        {
            m_session->SelectDocument(id);
        }
        RefreshDocuments();
    }
    void MaterialWorkspaceWidget::OnDocumentClosed(const AZ::Uuid& id)
    {
        if (m_displayedDocumentId == id)
        {
            m_displayedDocumentId = AZ::Uuid::CreateNull();
            m_inspector->SetDocumentId(m_displayedDocumentId);
        }
    }
    void MaterialWorkspaceWidget::OnDocumentDestroyed(const AZ::Uuid& id) { RefreshDocuments(id); }
    void MaterialWorkspaceWidget::OnDocumentModified(const AZ::Uuid& id)
    {
        RefreshDocumentLabel(id);
        RefreshActions();
    }
    void MaterialWorkspaceWidget::OnDocumentSaved(const AZ::Uuid&) { RefreshDocuments(); }
    void MaterialWorkspaceWidget::AfterUndoRedo() { RefreshActions(); }
    void MaterialWorkspaceWidget::OnEndUndo(const char*, bool) { RefreshActions(); }
}
