/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "MaterialWorkspaceWidget.h"
#include <JE/Editor/Materials/Viewport/MaterialPreviewViewportContent.h>
#include <JE/Editor/Materials/Document/MaterialDocumentRequestBus.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportWidget.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportToolBar.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <JE/Render/RPI/Public/RPISystemInterface.h>
#include <JE/Render/RPI/Public/ViewportContextBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <QShowEvent>
#include <QSplitter>

#include <JE/Editor/Tools/Document/AtomToolsDocumentInspector.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentRequestBus.h>
#include <JE/Editor/Tools/Util/Util.h>
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
        m_inspector = new JE::Editor::Tools::AtomToolsDocumentInspector(MaterialWorkspaceSession::GetToolId(), this);
        m_inspector->SetDocumentSettingsPrefix("/JE/Editor/MaterialWorkspace/Inspector");
        auto* splitter = new QSplitter(Qt::Horizontal, this);
        m_previewHost = new QWidget(splitter);
        m_previewHost->hide();
        m_previewLayout = new QVBoxLayout(m_previewHost);
        m_previewLayout->setContentsMargins(0, 0, 0, 0);
        splitter->addWidget(m_previewHost);
        splitter->addWidget(m_inspector);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 1);
        layout->addWidget(splitter, 1);

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
                RefreshPreview();
            }
            RefreshActions();
        });
        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler::BusConnect(MaterialWorkspaceSession::GetToolId());
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        connect(&session, &MaterialWorkspaceSession::AboutToClose, this,
            &MaterialWorkspaceWidget::OnSessionClosing, Qt::DirectConnection);
        qApp->installEventFilter(this);
        RefreshDocuments();
    }

    MaterialWorkspaceWidget::~MaterialWorkspaceWidget()
    {
        OnSessionClosing();
    }

    void MaterialWorkspaceWidget::ReleasePreview()
    {
        if (m_previewContent)
        {
            m_previewContent->SetDocumentId(AZ::Uuid::CreateNull());
        }
        m_previewContent.reset();
        delete m_previewHost;
        m_previewHost = nullptr;
        m_previewLayout = nullptr;
        m_previewViewport = nullptr;
    }

    void MaterialWorkspaceWidget::OnSessionClosing()
    {
        FinishEditing();
        m_inspector->SetDocumentId(AZ::Uuid::CreateNull());
        ReleasePreview();
        m_displayedDocumentId = AZ::Uuid::CreateNull();
        m_session = nullptr;
        m_documents->clear();
        setEnabled(false);
        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        qApp->removeEventFilter(this);
    }

    void MaterialWorkspaceWidget::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        RefreshPreview();
    }

    void MaterialWorkspaceWidget::CreatePreview()
    {
        if (m_previewViewport || !isVisible() || m_displayedDocumentId.IsNull() || !m_session || !JE::Render::RPI::RPISystemInterface::Get() ||
            !AZ::RHI::RHISystemInterface::Get() ||
            !AZ::Interface<JE::Render::RPI::ViewportContextRequestsInterface>::Get())
        {
            return;
        }
        m_session->EnsureMaterialPreviewSettings();
        const auto toolId = MaterialWorkspaceSession::GetToolId();
        auto* toolbar = new JE::Editor::Tools::EntityPreviewViewportToolBar(toolId, m_previewHost);
        m_previewViewport = new JE::Editor::Tools::EntityPreviewViewportWidget(toolId, m_previewHost, true);
        auto entityContext = AZStd::make_shared<AzFramework::EntityContext>();
        entityContext->InitContext();
        auto scene = AZStd::make_shared<JE::Editor::Tools::EntityPreviewViewportScene>(
            toolId, m_previewViewport, entityContext, "MaterialWorkspacePreview", "passes/mainrenderpipeline.azasset", true);
        m_previewContent = AZStd::make_shared<MaterialPreviewViewportContent>(toolId, m_previewViewport, entityContext, false);
        auto controller = AZStd::make_shared<JE::Editor::Tools::EntityPreviewViewportInputController>(
            toolId, m_previewViewport, m_previewContent);
        m_previewViewport->Init(entityContext, scene, m_previewContent, controller);
        m_previewLayout->addWidget(toolbar);
        m_previewLayout->addWidget(m_previewViewport, 1);
    }

    void MaterialWorkspaceWidget::RefreshPreview()
    {
        if (!m_previewHost)
        {
            return;
        }
        AZ::Data::Instance<JE::Render::RPI::Material> material;
        if (isVisible() && !m_displayedDocumentId.IsNull())
        {
            MaterialDocumentRequestBus::EventResult(
                material, m_displayedDocumentId, &MaterialDocumentRequests::GetInstance);
        }
        if (!material)
        {
            m_previewHost->hide();
            if (m_previewContent)
            {
                m_previewContent->SetDocumentId(AZ::Uuid::CreateNull());
            }
            return;
        }
        CreatePreview();
        if (m_previewContent)
        {
            m_previewContent->SetDocumentId(m_displayedDocumentId);
            m_previewHost->show();
        }
    }

    void MaterialWorkspaceWidget::OpenDocumentDialog()
    {
        if (!m_session)
        {
            return;
        }
        FinishEditing();
        const auto& type = m_session->GetDocumentType();
        const auto paths = JE::Editor::Tools::GetOpenFilePathsFromDialog(
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
            JE::Editor::Tools::AtomToolsDocumentRequestBus::Event(
                m_displayedDocumentId, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
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
                JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                    open, id, &JE::Editor::Tools::AtomToolsDocumentRequests::IsOpen);
                if (!open)
                {
                    continue;
                }
                AZStd::string path;
                bool modified = false;
                JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                    path, id, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
                JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                    modified, id, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
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
        RefreshPreview();
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
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                path, id, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                modified, id, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
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
            RefreshPreview();
        }
    }
    void MaterialWorkspaceWidget::OnDocumentDestroyed(const AZ::Uuid& id) { RefreshDocuments(id); }
    void MaterialWorkspaceWidget::OnDocumentModified(const AZ::Uuid& id)
    {
        RefreshDocumentLabel(id);
        if (id == m_displayedDocumentId)
        {
            RefreshPreview();
        }
        RefreshActions();
    }
    void MaterialWorkspaceWidget::OnDocumentSaved(const AZ::Uuid&) { RefreshDocuments(); }
    void MaterialWorkspaceWidget::AfterUndoRedo() { RefreshPreview(); RefreshActions(); }
    void MaterialWorkspaceWidget::OnEndUndo(const char*, bool) { RefreshPreview(); RefreshActions(); }
}
