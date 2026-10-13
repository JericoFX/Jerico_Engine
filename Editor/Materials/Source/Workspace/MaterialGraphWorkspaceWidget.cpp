/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "MaterialGraphWorkspaceWidget.h"
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentInspector.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/Graph/GraphDocumentView.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/algorithm.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

namespace JE::Editor
{
    MaterialGraphWorkspaceWidget::MaterialGraphWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent)
        : QWidget(parent), m_session(&session)
    {
        auto& environment = session.EnableGraphDocuments();
        const auto& settings = environment.GetViewSettings();
        auto* layout = new QVBoxLayout(this);
        auto* actions = new QHBoxLayout();
        auto* create = new QPushButton(tr("New"), this);
        auto* open = new QPushButton(tr("Open"), this);
        m_save = new QPushButton(tr("Save"), this);
        m_compile = new QPushButton(tr("Compile"), this);
        m_compile->setObjectName(QStringLiteral("CompileGraph"));
        m_openMaterial = new QPushButton(tr("Open Generated Material"), this);
        m_openMaterial->setObjectName(QStringLiteral("OpenGeneratedMaterial"));
        m_undo = new QPushButton(tr("Undo"), this);
        m_redo = new QPushButton(tr("Redo"), this);
        for (auto* button : {create, open, m_save, m_compile, m_openMaterial, m_undo, m_redo})
        {
            actions->addWidget(button);
        }
        layout->addLayout(actions);
        m_documents = new QComboBox(this);
        layout->addWidget(m_documents);
        m_compileStatus = new QLabel(this);
        m_compileStatus->setObjectName(QStringLiteral("GraphCompileStatus"));
        layout->addWidget(m_compileStatus);

        auto* columns = new QSplitter(this);
        m_palette = aznew GraphCanvas::NodePaletteWidget(columns);
        m_palette->setObjectName(QStringLiteral("GraphNodePalette"));
        GraphCanvas::NodePaletteConfig config;
        config.m_rootTreeItem = settings->m_createNodeTreeItemsFn(session.GetToolId());
        config.m_editorId = session.GetToolId();
        config.m_mimeType = settings->m_nodeMimeType.c_str();
        config.m_saveIdentifier = settings->m_nodeSaveIdentifier;
        m_palette->SetupNodePalette(config);
        AzQtComponents::StyleManager::setStyleSheet(m_palette, QStringLiteral(":/GraphView/GraphView.qss"));
        columns->addWidget(m_palette);
        auto* graphColumn = new QSplitter(Qt::Vertical, columns);
        auto* viewHost = new QWidget(graphColumn);
        m_viewLayout = new QVBoxLayout(viewHost);
        m_viewLayout->setContentsMargins(0, 0, 0, 0);
        m_propertiesMessage = new QLabel(tr("Open a material or pass graph, node configuration, or shader."), viewHost);
        m_viewLayout->addWidget(m_propertiesMessage);
        graphColumn->addWidget(viewHost);
        auto* navigation = new QTabWidget(graphColumn);
        navigation->addTab(aznew GraphCanvas::BookmarkDockWidget(session.GetToolId(), navigation), tr("Bookmarks"));
        navigation->addTab(aznew GraphCanvas::MiniMapGraphicsView(session.GetToolId(), false, GraphCanvas::GraphId(), navigation), tr("MiniMap"));
        graphColumn->addWidget(navigation);
        graphColumn->setStretchFactor(0, 1);
        graphColumn->setStretchFactor(1, 1);
        columns->addWidget(graphColumn);
        m_inspector = new AtomToolsFramework::AtomToolsDocumentInspector(session.GetToolId(), columns);
        m_inspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/DocumentInspector");
        columns->addWidget(m_inspector);
        columns->setStretchFactor(0, 1);
        columns->setStretchFactor(1, 1);
        columns->setStretchFactor(2, 1);
        layout->addWidget(columns, 1);

        connect(create, &QPushButton::clicked, this, [this]() { NewDocumentDialog(); });
        connect(open, &QPushButton::clicked, this, [this]() { OpenDocumentDialog(); });
        connect(m_save, &QPushButton::clicked, this, [this]()
        {
            FinishEditing();
            if (m_session && m_session->SelectDocument(m_displayedDocumentId))
            {
                m_session->SaveActiveDocument();
            }
        });
        connect(m_compile, &QPushButton::clicked, this, [this]() { CompileDocument(); });
        connect(m_openMaterial, &QPushButton::clicked, this, [this]() { OpenGeneratedMaterial(); });
        connect(m_undo, &QPushButton::clicked, this, [this]()
        {
            FinishEditing();
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed);
        });
        connect(m_redo, &QPushButton::clicked, this, [this]()
        {
            FinishEditing();
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed);
        });
        connect(m_documents, &QComboBox::currentIndexChanged, this, [this](int index)
        {
            if (index >= 0)
            {
                FinishEditing();
                SelectDocument(AZ::Uuid::CreateString(m_documents->itemData(index).toString().toUtf8().constData()));
            }
        });
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(session.GetToolId());
        connect(&session, &MaterialWorkspaceSession::CompileStateChanged, this, [this](const AZ::Uuid& id)
        {
            if (id == m_displayedDocumentId) { RefreshActions(); }
        });
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        qApp->installEventFilter(this);
        RefreshDocuments();
        // Set the initial proportions after creating the views; later selection keeps the user's splitter adjustments.
        columns->setSizes({280, 1000, 360});
        graphColumn->setSizes({800, 200});
    }

    MaterialGraphWorkspaceWidget::~MaterialGraphWorkspaceWidget()
    {
        qApp->removeEventFilter(this);
        FinishEditing();
        m_inspector->SetDocumentId(AZ::Uuid::CreateNull());
        if (m_graphView)
        {
            m_graphView->SetActiveGraphId(GraphCanvas::GraphId(), true);
        }
        delete m_graphView;
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
    }

    void MaterialGraphWorkspaceWidget::NewDocumentDialog()
    {
        if (!m_session)
        {
            return;
        }
        FinishEditing();
        QStringList names;
        for (const auto& type : m_session->GetGraphDocumentTypes())
        {
            if (!type.m_supportedExtensionsToCreate.empty())
            {
                names.push_back(QString::fromUtf8(type.m_documentTypeName.c_str()));
            }
        }
        bool accepted = false;
        const auto name = QInputDialog::getItem(this, tr("New Graph Document"), tr("Document Type"), names, 0, false, &accepted);
        if (!accepted)
        {
            return;
        }
        for (const auto& type : m_session->GetGraphDocumentTypes())
        {
            if (name == QString::fromUtf8(type.m_documentTypeName.c_str()))
            {
                const AZ::IO::FixedMaxPath folder = AZ::IO::FixedMaxPath(AZ::Utils::GetProjectPath()) / "Assets";
                AtomToolsFramework::CreateDocumentDialog dialog(type, QString::fromUtf8(folder.c_str()), this);
                if (dialog.exec() == QDialog::Accepted)
                {
                    const AZStd::string source = AtomToolsFramework::GetPathWithoutAlias(dialog.m_sourcePath.toUtf8().constData());
                    AZStd::string target = dialog.m_targetPath.toUtf8().constData();
                    if (AtomToolsFramework::ValidateDocumentPath(target))
                    {
                        const auto id = m_session->CreateDocument(source, target);
                        if (id.IsNull())
                        {
                            QMessageBox::critical(this, tr("Create Graph Document"), tr("The document could not be created. Choose a new destination."));
                        }
                    }
                }
                return;
            }
        }
    }

    void MaterialGraphWorkspaceWidget::OpenDocumentDialog()
    {
        if (!m_session)
        {
            return;
        }
        AtomToolsFramework::DocumentTypeInfo::DocumentExtensionInfoVector extensions;
        for (const auto& type : m_session->GetGraphDocumentTypes())
        {
            extensions.insert(extensions.end(), type.m_supportedExtensionsToOpen.begin(), type.m_supportedExtensionsToOpen.end());
        }
        const auto paths = AtomToolsFramework::GetOpenFilePathsFromDialog({}, extensions, "Graph Documents", false);
        if (!paths.empty())
        {
            OpenDocument(paths.front());
        }
    }

    AZ::Uuid MaterialGraphWorkspaceWidget::OpenDocument(const AZStd::string& sourcePath)
    {
        FinishEditing();
        return m_session ? m_session->OpenDocument(sourcePath) : AZ::Uuid::CreateNull();
    }

    void MaterialGraphWorkspaceWidget::FinishEditing()
    {
        if (auto* focus = QApplication::focusWidget(); focus && (focus == this || isAncestorOf(focus)))
        {
            focus->clearFocus();
        }
        if (!m_displayedDocumentId.IsNull())
        {
            AtomToolsFramework::AtomToolsDocumentRequestBus::Event(m_displayedDocumentId, &AtomToolsFramework::AtomToolsDocumentRequests::EndEdit);
        }
    }

    void MaterialGraphWorkspaceWidget::closeEvent(QCloseEvent* event)
    {
        FinishEditing();
        event->accept(); // Documents and global history belong to the session, not the pane.
    }

    bool MaterialGraphWorkspaceWidget::eventFilter(QObject* watched, QEvent* event)
    {
        auto* widget = qobject_cast<QWidget*>(watched);
        if (widget && (widget == this || isAncestorOf(widget)) && event->type() == QEvent::FocusIn &&
            m_session && m_session->GetActiveDocumentId() != m_displayedDocumentId)
        {
            SelectDocument(m_displayedDocumentId);
        }
        if (widget && (widget == this || isAncestorOf(widget)) &&
            (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress))
        {
            auto* key = static_cast<QKeyEvent*>(event);
            const bool create = key->matches(QKeySequence::New);
            const bool open = key->matches(QKeySequence::Open);
            const bool save = key->matches(QKeySequence::Save);
            const bool undo = key->matches(QKeySequence::Undo);
            const bool redo = key->matches(QKeySequence::Redo);
            if (create || open || save || undo || redo)
            {
                key->accept();
                if (event->type() == QEvent::KeyPress)
                {
                    FinishEditing();
                    if (create) { NewDocumentDialog(); }
                    else if (open) { OpenDocumentDialog(); }
                    else if (undo) { AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::UndoPressed); }
                    else if (redo) { AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::RedoPressed); }
                    else if (m_session && m_session->SelectDocument(m_displayedDocumentId)) { m_session->SaveActiveDocument(); }
                }
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void MaterialGraphWorkspaceWidget::SelectDocument(const AZ::Uuid& id)
    {
        if (!m_session || (!id.IsNull() && !m_session->SelectDocument(id)))
        {
            return;
        }
        if (m_displayedDocumentId == id)
        {
            if (m_graphView)
            {
                GraphCanvas::GraphId graphId;
                AtomToolsFramework::GraphDocumentRequestBus::EventResult(graphId, id, &AtomToolsFramework::GraphDocumentRequests::GetGraphId);
                m_graphView->SetActiveGraphId(graphId, true);
            }
            return;
        }
        if (m_graphView)
        {
            m_graphView->SetActiveGraphId(GraphCanvas::GraphId(), true);
        }
        delete m_graphView;
        m_graphView = nullptr;
        m_displayedDocumentId = id;
        AtomToolsFramework::DocumentTypeInfo type;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            type, id, &AtomToolsFramework::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        const bool pass = type.m_documentTypeName == MaterialGraphEnvironment::PassGraphTypeName ||
            type.m_documentTypeName == MaterialGraphEnvironment::PassNodeConfigTypeName;
        const auto kind = pass ? MaterialGraphEnvironment::GraphKind::RenderPass : MaterialGraphEnvironment::GraphKind::Material;
        auto& environment = m_session->EnableGraphDocuments();
        environment.SetActiveGraphKind(kind);
        m_palette->ResetModel(environment.CreateNodePaletteTree(kind));
        GraphCanvas::GraphId graphId;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(graphId, id, &AtomToolsFramework::GraphDocumentRequests::GetGraphId);
        if (graphId.IsValid())
        {
            m_graphView = aznew AtomToolsFramework::GraphDocumentView(
                m_session->GetToolId(), id, m_session->EnableGraphDocuments().GetViewSettings(), this);
            m_viewLayout->addWidget(m_graphView);
        }
        m_propertiesMessage->setVisible(!graphId.IsValid());
        m_inspector->SetDocumentId(id);
        RefreshActions();
    }

    void MaterialGraphWorkspaceWidget::RefreshDocuments(const AZ::Uuid& excludedDocumentId)
    {
        const QSignalBlocker blocker(m_documents);
        m_documents->clear();
        auto selected = AZ::Uuid::CreateNull();
        if (m_session)
        {
            for (const auto& id : m_session->GetDocumentIds())
            {
                if (id == excludedDocumentId || !m_session->IsGraphDocument(id)) { continue; }
                bool open = false;
                AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(open, id, &AtomToolsFramework::AtomToolsDocumentRequests::IsOpen);
                if (!open) { continue; }
                m_documents->addItem(QString(), QString::fromUtf8(id.ToString<AZStd::string>().c_str()));
                RefreshDocumentLabel(id);
                if (id == m_session->GetActiveDocumentId() || (selected.IsNull() && id == m_displayedDocumentId))
                {
                    selected = id;
                    m_documents->setCurrentIndex(m_documents->count() - 1);
                }
            }
        }
        if (selected.IsNull() && m_documents->count() > 0)
        {
            m_documents->setCurrentIndex(0);
            selected = AZ::Uuid::CreateString(m_documents->itemData(0).toString().toUtf8().constData());
        }
        SelectDocument(selected);
        RefreshActions();
    }

    void MaterialGraphWorkspaceWidget::RefreshDocumentLabel(const AZ::Uuid& id)
    {
        const int index = m_documents->findData(QString::fromUtf8(id.ToString<AZStd::string>().c_str()));
        if (index >= 0)
        {
            AZStd::string path;
            bool modified = false;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(path, id, &AtomToolsFramework::AtomToolsDocumentRequests::GetAbsolutePath);
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(modified, id, &AtomToolsFramework::AtomToolsDocumentRequests::IsModified);
            m_documents->setItemText(index, QFileInfo(QString::fromUtf8(path.c_str())).fileName() + (modified ? " *" : ""));
            m_documents->setItemData(index, QString::fromUtf8(path.c_str()), Qt::ToolTipRole);
        }
    }

    void MaterialGraphWorkspaceWidget::RefreshActions()
    {
        GraphCanvas::GraphId graphId;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(graphId, m_displayedDocumentId, &AtomToolsFramework::GraphDocumentRequests::GetGraphId);
        bool queued = false;
        AZStd::vector<AZStd::string> outputs;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(queued, m_displayedDocumentId, &AtomToolsFramework::GraphDocumentRequests::IsCompileGraphQueued);
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(outputs, m_displayedDocumentId, &AtomToolsFramework::GraphDocumentRequests::GetGeneratedFilePaths);
        using CompileState = MaterialWorkspaceSession::CompileState;
        const auto state = m_session ? m_session->GetCompileState(m_displayedDocumentId) : CompileState::NotCompiled;
        const bool compiling = state == CompileState::Compiling || state == CompileState::CompilingModified;
        const bool failed = state == CompileState::Failed;
        const bool modified = state == CompileState::Modified;
        const bool hasMaterial = AZStd::any_of(outputs.begin(), outputs.end(), [](const auto& path) { return path.ends_with(".material"); });
        AtomToolsFramework::DocumentTypeInfo type;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(type, m_displayedDocumentId,
            &AtomToolsFramework::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        const bool pass = type.m_documentTypeName == MaterialGraphEnvironment::PassGraphTypeName ||
            type.m_documentTypeName == MaterialGraphEnvironment::PassNodeConfigTypeName;
        m_compile->setVisible(!pass);
        m_openMaterial->setVisible(!pass);
        m_save->setEnabled(!m_displayedDocumentId.IsNull());
        m_compile->setEnabled(!pass && graphId.IsValid() && !compiling && !queued);
        m_openMaterial->setEnabled(!pass && graphId.IsValid() && hasMaterial && !compiling && !queued && !failed && !modified);
        m_compileStatus->setVisible(pass || graphId.IsValid());
        m_compileStatus->setText(pass ? tr("Pass Graph editing is available. Pass generation and pipeline application are not implemented.")
            : compiling ? tr("Compiling material graph...") : queued ? tr("Material graph compilation queued...")
            : failed ? tr("Material graph compilation failed. See the Editor log for details.")
            : modified ? tr("Material graph changed. Compile to update its outputs.")
            : state == CompileState::Completed ? tr("Material graph compilation completed.")
            : tr("Compile the material graph to generate its outputs."));
        AzToolsFramework::UndoSystem::UndoStack* stack = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(stack, &AzToolsFramework::ToolsApplicationRequests::GetUndoStack);
        m_undo->setEnabled(stack && stack->CanUndo());
        m_redo->setEnabled(stack && stack->CanRedo());
    }

    void MaterialGraphWorkspaceWidget::CompileDocument()
    {
        FinishEditing();
        AtomToolsFramework::DocumentTypeInfo type;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(type, m_displayedDocumentId,
            &AtomToolsFramework::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        if (type.m_documentTypeName != "Material Graph") { return; }
        const bool started = m_session && m_session->RequestCompile(m_displayedDocumentId);
        if (!started)
        {
            QMessageBox::critical(this, tr("Compile Material Graph"), tr("Material graph compilation could not be started. See the Editor log for details."));
        }
        RefreshActions();
    }

    void MaterialGraphWorkspaceWidget::OpenGeneratedMaterial()
    {
        AtomToolsFramework::DocumentTypeInfo type;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(type, m_displayedDocumentId,
            &AtomToolsFramework::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        if (type.m_documentTypeName != "Material Graph") { return; }
        AZStd::vector<AZStd::string> paths;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(paths, m_displayedDocumentId, &AtomToolsFramework::GraphDocumentRequests::GetGeneratedFilePaths);
        QStringList materials;
        for (const auto& path : paths)
        {
            if (path.ends_with(".material")) { materials.push_back(QString::fromUtf8(path.c_str())); }
        }
        if (materials.empty())
        {
            QMessageBox::information(this, tr("Generated Material"), tr("Compile a graph with a material output first."));
            return;
        }
        bool accepted = true;
        const QString path = materials.size() == 1 ? materials.front()
            : QInputDialog::getItem(this, tr("Generated Material"), tr("Material"), materials, 0, false, &accepted);
        if (accepted)
        {
            EditorMaterialSystemComponentRequestBus::Broadcast(&EditorMaterialSystemComponentRequests::OpenMaterialEditor,
                AZStd::string(path.toUtf8().constData()));
        }
    }

    void MaterialGraphWorkspaceWidget::OnDocumentOpened(const AZ::Uuid& id)
    {
        if (m_session && m_session->IsGraphDocument(id))
        {
            m_session->SelectDocument(id);
            RefreshDocuments();
        }
    }
    void MaterialGraphWorkspaceWidget::OnDocumentClosed(const AZ::Uuid& id)
    {
        if (m_displayedDocumentId == id) { SelectDocument(AZ::Uuid::CreateNull()); }
    }
    void MaterialGraphWorkspaceWidget::OnDocumentDestroyed(const AZ::Uuid& id) { RefreshDocuments(id); }
    void MaterialGraphWorkspaceWidget::OnDocumentModified(const AZ::Uuid& id)
    {
        RefreshDocumentLabel(id);
        RefreshActions();
    }
    void MaterialGraphWorkspaceWidget::OnDocumentSaved(const AZ::Uuid& id) { RefreshDocumentLabel(id); }
    void MaterialGraphWorkspaceWidget::AfterUndoRedo() { RefreshActions(); }
    void MaterialGraphWorkspaceWidget::OnEndUndo(const char*, bool) { RefreshActions(); }
}
