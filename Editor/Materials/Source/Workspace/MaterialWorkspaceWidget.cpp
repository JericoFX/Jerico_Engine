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
#include <QLabel>
#include <AzCore/IO/FileIO.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace JE::Editor
{
    MaterialWorkspaceWidget::MaterialWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent)
        : MaterialWorkspaceWidget(session, parent, false)
    {
    }

    MaterialWorkspaceWidget::MaterialWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent, bool inspectorOnly)
        : QWidget(parent)
        , m_session(&session)
        , m_inspectorOnly(inspectorOnly)
    {
        auto* layout = new QVBoxLayout(this);
        auto* actions = new QHBoxLayout();
        auto* open = new QPushButton(tr("Open"), this);
        m_save = new QPushButton(tr("Save"), this);
        m_undo = new QPushButton(tr("Undo"), this);
        m_redo = new QPushButton(tr("Redo"), this);
        m_save->setObjectName("MaterialInspectorSave");
        open->setVisible(!m_inspectorOnly);
        m_undo->setVisible(!m_inspectorOnly);
        m_redo->setVisible(!m_inspectorOnly);
        m_status = new QLabel(this);
        m_status->setObjectName("MaterialInspectorStatus");
        m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_status->setWordWrap(true);
        m_status->setVisible(m_inspectorOnly);
        layout->addWidget(m_status);
        actions->addWidget(open);
        actions->addWidget(m_save);
        actions->addWidget(m_undo);
        actions->addWidget(m_redo);
        actions->addStretch();
        layout->addLayout(actions);
        m_documents = new QComboBox(this);
        layout->addWidget(m_documents);
        m_documents->setVisible(!m_inspectorOnly);
        m_inspector = new JE::Editor::Tools::AtomToolsDocumentInspector(MaterialWorkspaceSession::GetToolId(), this);
        m_inspector->setObjectName("MaterialDocumentInspector");
        m_inspector->SetDocumentSettingsPrefix("/JE/Editor/MaterialWorkspace/Inspector");
        if (m_inspectorOnly)
        {
            layout->addWidget(m_inspector, 1);
        }
        else
        {
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
        }

        connect(open, &QPushButton::clicked, this, [this]() { OpenDocumentDialog(); });
        connect(m_save, &QPushButton::clicked, this, [this]() {
            SaveDisplayedDocument();
        });
        connect(m_undo, &QPushButton::clicked, this, [this]() {
            if (!FinishEditing()) { return; }
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);
        });
        connect(m_redo, &QPushButton::clicked, this, [this]() {
            if (!FinishEditing()) { return; }
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);
        });
        connect(m_documents, &QComboBox::currentIndexChanged, this, [this](int index) {
            if (!m_session || index < 0)
            {
                return;
            }
            if (!FinishEditing()) { RefreshDocuments(); return; }
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

    void MaterialWorkspaceWidget::GetAssetInspectorWidget(MaterialWorkspaceSession& session,
        const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry, QWidget* parent,
        QWidget*& widget, bool& selectionAccepted)
    {
        if (!parent || widget || !selectionAccepted) { return; }
        using namespace AzToolsFramework::AssetBrowser;
        auto* view = parent->findChild<MaterialWorkspaceWidget*>("MaterialAssetInspector", Qt::FindDirectChildrenOnly);
        if (view && view->m_session != &session)
        {
            if (view->m_session) { selectionAccepted = false; return; }
            delete view; // Its prior session already made this retained view inert.
            view = nullptr;
        }
        const auto* source = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
        if (const auto* product = azrtti_cast<const ProductAssetBrowserEntry*>(entry))
        {
            source = product->GetAssetType() == azrtti_typeid<JE::Render::RPI::MaterialAsset>()
                ? azrtti_cast<const SourceAssetBrowserEntry*>(product->GetParent()) : nullptr;
        }
        const AZStd::string sourcePath = source ? source->GetFullPath() : AZStd::string{};
        if (!source || !QString::fromUtf8(sourcePath.c_str()).endsWith(".material", Qt::CaseInsensitive))
        {
            if (view && !view->FinishEditing()) { selectionAccepted = false; }
            return;
        }
        if (!view)
        {
            view = new MaterialWorkspaceWidget(session, parent, true);
            view->setObjectName("MaterialAssetInspector");
        }
        if (view->OpenDocument(sourcePath).IsNull())
        {
            selectionAccepted = false;
            return;
        }
        widget = view;
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
        if (m_inspectorOnly || m_previewViewport || !isVisible() || m_displayedDocumentId.IsNull() || !m_session || !JE::Render::RPI::RPISystemInterface::Get() ||
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
        if (m_inspectorOnly || !m_previewHost)
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
        if (!FinishEditing()) { return; }
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
        if (!m_session || !FinishEditing()) { return AZ::Uuid::CreateNull(); }
        const auto id = m_session->OpenDocument(sourcePath);
        if (m_inspectorOnly && !id.IsNull() && !m_session->IsGraphDocument(id) && m_session->SelectDocument(id))
        {
            m_displayedDocumentId = id;
            m_inspector->SetDocumentId(id);
            RefreshActions();
        }
        return id;
    }

    bool MaterialWorkspaceWidget::FinishEditing()
    {
        if (auto* focus = QApplication::focusWidget(); focus && (focus == this || isAncestorOf(focus)))
        {
            focus->clearFocus();
        }
        if (!m_displayedDocumentId.IsNull())
        {
            bool committed = false;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                committed, m_displayedDocumentId, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
            if (!committed && m_inspectorOnly)
            {
                m_status->setToolTip(tr("The current edit could not be committed. Selection was preserved."));
                return false;
            }
        }
        return true;
    }

    void MaterialWorkspaceWidget::closeEvent(QCloseEvent* event)
    {
        if (!FinishEditing()) { event->ignore(); return; }
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
            const bool open = !m_inspectorOnly && key->matches(QKeySequence::Open);
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
                        if (!FinishEditing()) { return true; }
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
                    else
                    {
                        SaveDisplayedDocument();
                    }
                }
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void MaterialWorkspaceWidget::RefreshDocuments(const AZ::Uuid& excludedDocumentId)
    {
        if (m_inspectorOnly)
        {
            if (m_displayedDocumentId == excludedDocumentId) { m_displayedDocumentId = AZ::Uuid::CreateNull(); }
            m_inspector->SetDocumentId(m_displayedDocumentId);
            RefreshActions();
            return;
        }
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
        bool modified = false;
        AZStd::string path;
        if (m_session && !m_displayedDocumentId.IsNull())
        {
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                modified, m_displayedDocumentId, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                path, m_displayedDocumentId, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
        }
        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        const bool editable = path.empty() || (JE::Editor::Tools::IsDocumentPathEditable(path) &&
            (!fileIO || !fileIO->IsReadOnly(path.c_str())));
        if (m_inspectorOnly) { m_inspector->setEnabled(editable); }
        m_save->setEnabled(m_session && !m_displayedDocumentId.IsNull() && (!m_inspectorOnly || (modified && editable)));
        m_status->setText(path.empty() ? tr("No material selected") : QFileInfo(QString::fromUtf8(path.c_str())).fileName() +
            (modified ? " *" : "") + (editable ? "" : tr(" (read only)")));
        m_status->setToolTip(QString::fromUtf8(path.c_str()));
        AzToolsFramework::UndoSystem::UndoStack* stack = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            stack, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);
        m_undo->setEnabled(stack && stack->CanUndo());
        m_redo->setEnabled(stack && stack->CanRedo());
    }

    bool MaterialWorkspaceWidget::SaveDisplayedDocument()
    {
        if (!m_session || !FinishEditing() || !m_session->SelectDocument(m_displayedDocumentId)) { return false; }
        if (m_inspectorOnly)
        {
            RefreshActions();
            if (!m_save->isEnabled()) { return false; }
        }
        const bool saved = m_session->SaveActiveDocument();
        RefreshActions();
        if (!saved) { m_status->setToolTip(tr("Saving failed. The document remains modified.")); }
        return saved;
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
        if (m_inspectorOnly || (m_session && m_session->IsGraphDocument(id)))
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
