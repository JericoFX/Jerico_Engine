/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "MaterialGraphWorkspaceWidget.h"
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>
#include <JE/Editor/Materials/Viewport/MaterialPreviewViewportContent.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportScene.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportInputController.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportWidget.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportToolBar.h>
#include <JE/Render/RPI/Public/RPISystemInterface.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzFramework/Entity/EntityContext.h>
#include <JE/Render/RPI/Public/ViewportContextBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponentRequestBus.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentInspector.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentRequestBus.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentSystemRequestBus.h>
#include <JE/Editor/Tools/Document/CreateDocumentDialog.h>
#include <JE/Editor/Tools/SettingsDialog/SettingsDialog.h>
#include <JE/Editor/Tools/Graph/GraphDocumentRequestBus.h>
#include <JE/Editor/Tools/Graph/GraphDocumentView.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/algorithm.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/FlowLayout.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileInfo>
#include <QHideEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSplitter>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace JE::Editor
{
    MaterialGraphWorkspaceWidget::MaterialGraphWorkspaceWidget(MaterialWorkspaceSession& session, QWidget* parent)
        : QWidget(parent), m_session(&session)
    {
        auto& environment = session.EnableGraphDocuments();
        const auto& settings = environment.GetViewSettings();
        auto* layout = new QVBoxLayout(this);
        auto* actions = new FlowLayout(0);
        auto* create = new QPushButton(tr("New"), this);
        auto* open = new QPushButton(tr("Open"), this);
        m_save = new QPushButton(tr("Save"), this);
        m_compile = new QPushButton(tr("Compile"), this);
        m_compile->setObjectName(QStringLiteral("CompileGraph"));
        m_openMaterial = new QPushButton(tr("Open Generated Material"), this);
        m_openMaterial->setObjectName(QStringLiteral("OpenGeneratedMaterial"));
        m_undo = new QPushButton(tr("Undo"), this);
        m_redo = new QPushButton(tr("Redo"), this);
        m_settings = new QPushButton(tr("Settings..."), this);
        m_settings->setObjectName(QStringLiteral("MaterialGraphSettings"));
        m_settings->setAccessibleName(tr("Material Graph Settings"));
        m_settings->setToolTip(tr("Configure material graph compilation and node editing."));
        for (auto* button : {create, open, m_save, m_compile, m_openMaterial, m_undo, m_redo, m_settings})
        {
            actions->addWidget(button);
        }
        layout->addLayout(actions);
        m_documents = new QComboBox(this);
        m_documents->setObjectName(QStringLiteral("GraphDocuments"));
        m_documents->setToolTip(tr("Right-click to close an open document."));
        m_documents->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_documents, &QComboBox::customContextMenuRequested, this, [this](const QPoint& position)
        {
            QMenu menu(this);
            menu.setObjectName(QStringLiteral("CloseGraphDocumentMenu"));
            menu.addSection(tr("Close Document"));
            for (int index = 0; index < m_documents->count(); ++index)
            {
                const auto id = AZ::Uuid::CreateString(m_documents->itemData(index).toString().toUtf8().constData());
                auto* action = menu.addAction(tr("Close %1").arg(m_documents->itemText(index)));
                action->setData(m_documents->itemData(index));
                action->setToolTip(m_documents->itemData(index, Qt::ToolTipRole).toString());
                connect(action, &QAction::triggered, this, [this, id]() { CloseDocument(id); });
            }
            menu.exec(m_documents->mapToGlobal(position));
        });
        layout->addWidget(m_documents);
        m_compileStatus = new QLabel(this);
        m_compileStatus->setObjectName(QStringLiteral("GraphCompileStatus"));
        layout->addWidget(m_compileStatus);

        auto* columns = new QSplitter(this);
        m_palette = aznew JE::Editor::Tools::GraphCanvas::NodePaletteWidget(columns);
        m_palette->setObjectName(QStringLiteral("GraphNodePalette"));
        JE::Editor::Tools::GraphCanvas::NodePaletteConfig config;
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
        auto* bookmarks = aznew JE::Editor::Tools::GraphCanvas::BookmarkDockWidget(session.GetToolId(), navigation);
        bookmarks->layout()->setVerticalSizeConstraint(QLayout::SetNoConstraint);
        bookmarks->setMinimumHeight(0);
        navigation->addTab(bookmarks, tr("Bookmarks"));
        auto* miniMap = aznew JE::Editor::Tools::GraphCanvas::MiniMapGraphicsView(
            session.GetToolId(), false, JE::Editor::Tools::GraphCanvas::GraphId(), navigation);
        static_cast<JE::Editor::Tools::GraphCanvas::AssetEditorNotificationBus::Handler*>(miniMap)->BusConnect(session.GetToolId());
        miniMap->setMinimumSize(0, 0);
        miniMap->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        navigation->addTab(miniMap, tr("MiniMap"));
        auto navigationSizePolicy = navigation->sizePolicy();
        navigationSizePolicy.setVerticalPolicy(QSizePolicy::Ignored);
        navigation->setSizePolicy(navigationSizePolicy);
        graphColumn->addWidget(navigation);
        graphColumn->setStretchFactor(0, 1);
        graphColumn->setStretchFactor(1, 1);
        columns->addWidget(graphColumn);
        m_propertiesColumn = new QSplitter(Qt::Vertical, columns);
        m_propertiesColumn->setObjectName(QStringLiteral("MaterialGraphPropertiesSplitter"));
        m_inspector = new JE::Editor::Tools::AtomToolsDocumentInspector(session.GetToolId(), m_propertiesColumn);
        m_inspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/DocumentInspector");
        m_propertiesColumn->addWidget(m_inspector);
        m_materialPreviewHost = new QWidget(m_propertiesColumn);
        m_materialPreviewHost->setObjectName(QStringLiteral("MaterialGraphPreviewHost"));
        m_materialPreviewHost->hide();
        m_materialPreviewLayout = new QVBoxLayout(m_materialPreviewHost);
        m_materialPreviewLayout->setContentsMargins(0, 0, 0, 0);
        m_propertiesColumn->addWidget(m_materialPreviewHost);
        m_propertiesColumn->setStretchFactor(0, 1);
        m_propertiesColumn->setStretchFactor(1, 1);
        columns->addWidget(m_propertiesColumn);
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
        connect(m_settings, &QPushButton::clicked, this, &MaterialGraphWorkspaceWidget::ShowMaterialGraphSettings);
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
        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler::BusConnect(session.GetToolId());
        connect(&session, &MaterialWorkspaceSession::DocumentSelected, this, [this](const AZ::Uuid& id)
        {
            if (m_session && m_session->IsGraphDocument(id) && id != m_displayedDocumentId)
            {
                RefreshDocuments();
            }
        });
        connect(&session, &MaterialWorkspaceSession::CompileStateChanged, this, [this](const AZ::Uuid& id)
        {
            if (id == m_displayedDocumentId)
            {
                RefreshActions();
            }
        });
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        connect(&session, &MaterialWorkspaceSession::AboutToClose, this,
            &MaterialGraphWorkspaceWidget::ReleaseMaterialPreview, Qt::DirectConnection);
        qApp->installEventFilter(this);
        RefreshDocuments();
        // Set the initial proportions after creating the views; later selection keeps the user's splitter adjustments.
        columns->setSizes({280, 1000, 360});
        const int initialRegionSize = qMax(viewHost->minimumSizeHint().height(), navigation->minimumSizeHint().height());
        graphColumn->setSizes({initialRegionSize, initialRegionSize});
    }

    MaterialGraphWorkspaceWidget::~MaterialGraphWorkspaceWidget()
    {
        m_previewClosed = true;
        ReleaseMaterialPreview();
        qApp->removeEventFilter(this);
        FinishEditing();
        m_inspector->SetDocumentId(AZ::Uuid::CreateNull());
        if (m_graphView)
        {
            m_graphView->SetActiveGraphId(JE::Editor::Tools::GraphCanvas::GraphId(), true);
        }
        delete m_graphView;
        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
    }

    void MaterialGraphWorkspaceWidget::ShowMaterialGraphSettings()
    {
        if (!m_session) { return; }
        JE::Editor::Tools::DocumentTypeInfo type;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(type, m_displayedDocumentId,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        if (type.m_documentTypeName == MaterialGraphEnvironment::PassGraphTypeName ||
            type.m_documentTypeName == MaterialGraphEnvironment::PassNodeConfigTypeName) { return; }

        const QPointer<MaterialWorkspaceSession> session = m_session;
        const auto viewSettings = session->EnableGraphDocuments().GetViewSettings();
        // Keep the legacy live-edit registry bindings without writing absent material-option defaults.
        const auto group = JE::Editor::Tools::CreateSettingsPropertyGroup(
            tr("Material Canvas Settings").toUtf8().constData(),
            tr("Material Canvas Settings").toUtf8().constData(),
            {
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/Atom/MaterialCanvas/ForceDeleteGeneratedFiles",
                    tr("Delete Files On Compile").toUtf8().constData(),
                    tr("This option forces files previously generated from the current graph to be deleted before creating new ones.").toUtf8().constData(),
                    false),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/Atom/MaterialCanvas/ForceClearAssetFingerprints",
                    tr("Clear Asset Fingerprints On Compile").toUtf8().constData(),
                    tr("This option forces the AP to reprocess generated files even if no differences were detected since last generated. This "
                       "guarantees that notifications are sent for assets like materials that may not be changed even if their dependent "
                       "material types or shaders are. This notifies other systems to reload materials after shader changes. "
                       "Enabling this setting may affect the time it takes for the viewport to reflect shader and material changes.").toUtf8().constData(),
                    false),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen",
                    tr("Enable Compile On Open").toUtf8().constData(),
                    tr("If enabled, shaders and materials will automatically be generated whenever a material graph is opened.").toUtf8().constData(),
                    true),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave",
                    tr("Enable Compile On Save").toUtf8().constData(),
                    tr("If enabled, shaders and materials will automatically be generated whenever a material graph is saved.").toUtf8().constData(),
                    true),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit",
                    tr("Enable Compile On Edit").toUtf8().constData(),
                    tr("If enabled, shaders and materials will automatically be generated whenever a material graph is edited.").toUtf8().constData(),
                    true),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphStarted",
                    tr("Clear Viewport Material When Compiling Starts").toUtf8().constData(),
                    tr("Clear the viewport model's material whenever compiling shaders and materials starts.").toUtf8().constData(),
                    true),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphFailed",
                    tr("Clear Viewport Material When Compiling Fails").toUtf8().constData(),
                    tr("Clear the viewport model's material whenever compiling shaders and materials fails.").toUtf8().constData(),
                    true),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/AtomToolsFramework/GraphCompiler/EnableLogging",
                    tr("Enable Compiler Logging").toUtf8().constData(),
                    tr("Toggle verbose logging for material graph generation.").toUtf8().constData(),
                    false),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/AtomToolsFramework/DynamicNode/EnablePropertyEditingOnNodeUI",
                    tr("Enable Property Editing On Nodes").toUtf8().constData(),
                    tr("Toggle settings to display properties and allow them to be added directly on graph nodes.").toUtf8().constData(),
                    true),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/AtomToolsFramework/GraphCompiler/QueueGraphCompileIntervalMs",
                    tr("Queue Graph Compile Interval Ms").toUtf8().constData(),
                    tr("The delay (in milliseconds) before the graph is recompiled after changes.").toUtf8().constData(),
                    aznumeric_cast<AZ::s64>(500), aznumeric_cast<AZ::s64>(0), aznumeric_cast<AZ::s64>(1000)),
                JE::Editor::Tools::CreateSettingsPropertyValue(
                    "/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart",
                    tr("Create Default Document On First Workspace Open").toUtf8().constData(),
                    tr("Create an untitled material graph on the first explicit Graph Workspace opening of a new session, "
                       "when no document is already present. Changes apply to the next session.").toUtf8().constData(),
                    true)
            });

        constexpr const char* fasterShaderBuildsKey = "/O3DE/Preferences/JE/Materials/EnableFasterShaderBuilds";
        auto fasterShaderBuilds = JE::Editor::Tools::CreateSettingsPropertyValue(
            fasterShaderBuildsKey,
            tr("Faster Shader Builds").toUtf8().constData(),
            tr("Build fewer shader variants to reduce compilation time. "
               "Applies the next time this Editor starts Asset Processor for this project.").toUtf8().constData(),
            false);
        auto fasterShaderBuildsConfig = fasterShaderBuilds.GetConfig();
        const auto changeRequested = fasterShaderBuildsConfig.m_dataChangeCallback;
        fasterShaderBuildsConfig.m_dataChangeCallback = [fasterShaderBuildsKey, changeRequested](const AZStd::any& value) -> AZ::u32
        {
            if (JE::Editor::Tools::GetSettingsValue<bool>(fasterShaderBuildsKey, false) == AZStd::any_cast<bool>(value))
            {
                return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
            }
            const auto refresh = changeRequested(value);
            AzToolsFramework::EditorSettingsAPIBus::Broadcast(
                &AzToolsFramework::EditorSettingsAPIRequests::SaveSettingsRegistryFile);
            return refresh;
        };
        fasterShaderBuilds.SetConfig(fasterShaderBuildsConfig);
        group->m_properties.push_back(AZStd::move(fasterShaderBuilds));

        // The local group outlives the inspector even if the Workspace is destroyed during exec().
        QPointer<JE::Editor::Tools::SettingsDialog> dialog = new JE::Editor::Tools::SettingsDialog(this);
        dialog->setObjectName(QStringLiteral("MaterialGraphSettingsDialog"));
        dialog->setWindowTitle(tr("Material Graph Settings"));
        dialog->GetInspector()->AddGroupsBegin();
        dialog->GetInspector()->AddGroup(group->m_name, group->m_displayName, group->m_description,
            new JE::Editor::Tools::InspectorPropertyGroupWidget(group.get(), group.get(),
                azrtti_typeid<JE::Editor::Tools::DynamicPropertyGroup>()));
        dialog->GetInspector()->AddGroup(
            tr("Graph View Settings").toUtf8().constData(),
            tr("Graph View Settings").toUtf8().constData(),
            tr("Configuration settings for the graph view interaction, animation, and other behavior.").toUtf8().constData(),
            new JE::Editor::Tools::InspectorPropertyGroupWidget(
                viewSettings.get(), viewSettings.get(), viewSettings->RTTI_Type()));
        dialog->GetInspector()->AddGroupsEnd();
        connect(session.data(), &MaterialWorkspaceSession::AboutToClose, dialog.data(), &QDialog::reject);
        // Preserve the initial size through automatic window decoration, then allow normal resizing.
        dialog->setFixedSize(800, 400);
        dialog->show();
        dialog->setMinimumSize(0, 0);
        dialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        dialog->exec();
        if (session)
        {
            JE::Editor::Tools::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", viewSettings);
        }
        if (dialog) { delete dialog.data(); }
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
                JE::Editor::Tools::CreateDocumentDialog dialog(type, QString::fromUtf8(folder.c_str()), this);
                if (dialog.exec() == QDialog::Accepted)
                {
                    const AZStd::string source = JE::Editor::Tools::GetPathWithoutAlias(dialog.m_sourcePath.toUtf8().constData());
                    AZStd::string target = dialog.m_targetPath.toUtf8().constData();
                    if (JE::Editor::Tools::ValidateDocumentPath(target))
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
        JE::Editor::Tools::DocumentTypeInfo::DocumentExtensionInfoVector extensions;
        for (const auto& type : m_session->GetGraphDocumentTypes())
        {
            extensions.insert(extensions.end(), type.m_supportedExtensionsToOpen.begin(), type.m_supportedExtensionsToOpen.end());
        }
        const auto paths = JE::Editor::Tools::GetOpenFilePathsFromDialog({}, extensions, "Graph Documents", false);
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
            JE::Editor::Tools::AtomToolsDocumentRequestBus::Event(m_displayedDocumentId, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        }
    }

    void MaterialGraphWorkspaceWidget::closeEvent(QCloseEvent* event)
    {
        FinishEditing();
        m_previewClosed = true;
        RefreshMaterialPreview();
        RefreshActions();
        event->accept(); // Documents and global history belong to the session, not the pane.
    }

    void MaterialGraphWorkspaceWidget::showEvent(QShowEvent* event)
    {
        m_previewClosed = false;
        RefreshActions();
        QWidget::showEvent(event);
        RefreshMaterialPreview();
    }

    void MaterialGraphWorkspaceWidget::hideEvent(QHideEvent* event)
    {
        if (m_materialPreviewContent)
        {
            m_materialPreviewContent->SetGraphDocumentId(AZ::Uuid::CreateNull());
        }
        QWidget::hideEvent(event);
    }

    void MaterialGraphWorkspaceWidget::CloseDocument(const AZ::Uuid& id)
    {
        if (!m_session || !m_session->IsGraphDocument(id))
        {
            return;
        }
        bool ended = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            ended, id, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        if (!ended)
        {
            return;
        }
        bool open = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            open, id, &JE::Editor::Tools::AtomToolsDocumentRequests::IsOpen);
        if (!open)
        {
            return;
        }
        bool modified = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            modified, id, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
        if (modified)
        {
            AZStd::string path;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                path, id, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
            const auto decision = QMessageBox::question(this, tr("Material Changes"),
                tr("Save changes to %1 before closing this document?").arg(QString::fromUtf8(path.c_str())),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
            if ((decision != QMessageBox::Save && decision != QMessageBox::Discard) || !m_session)
            {
                return;
            }
            if (decision == QMessageBox::Save)
            {
                bool saved = false;
                JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(
                    saved, m_session->GetToolId(), &JE::Editor::Tools::AtomToolsDocumentSystemRequests::SaveDocument, id);
                if (!saved)
                {
                    return;
                }
            }
        }
        if (m_session)
        {
            bool closed = false;
            JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(
                closed, m_session->GetToolId(), &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CloseDocument, id);
        }
    }

    void MaterialGraphWorkspaceWidget::InitializeMaterialPreviewSplit()
    {
        if (m_materialPreviewSplitInitialized || !m_propertiesColumn || !m_materialPreviewHost ||
            !m_propertiesColumn->isVisible() || !m_materialPreviewHost->isVisible())
        {
            return;
        }
        const int available = m_propertiesColumn->height() - m_propertiesColumn->handleWidth();
        if (available > 1)
        {
            m_materialPreviewSplitInitialized = true;
            // One initial request; QSplitter still applies the children's real minimum/maximum constraints.
            m_propertiesColumn->setSizes({available / 2, available - available / 2});
        }
    }

    void MaterialGraphWorkspaceWidget::CreateMaterialPreview()
    {
        if (m_materialPreviewViewport || !isVisible() || m_previewClosed || !m_session || m_displayedDocumentId.IsNull() ||
            !JE::Render::RPI::RPISystemInterface::Get() || !AZ::RHI::RHISystemInterface::Get() ||
            !AZ::Interface<JE::Render::RPI::ViewportContextRequestsInterface>::Get())
        {
            return;
        }
        m_session->EnsureMaterialPreviewSettings();
        const auto toolId = m_session->GetToolId();
        auto* toolbar = new JE::Editor::Tools::EntityPreviewViewportToolBar(toolId, m_materialPreviewHost);
        m_materialPreviewViewport = new JE::Editor::Tools::EntityPreviewViewportWidget(toolId, m_materialPreviewHost, true);
        m_materialPreviewViewport->setObjectName(QStringLiteral("MaterialGraphPreviewViewport"));
        auto entityContext = AZStd::make_shared<AzFramework::EntityContext>();
        entityContext->InitContext();
        auto scene = AZStd::make_shared<JE::Editor::Tools::EntityPreviewViewportScene>(
            toolId, m_materialPreviewViewport, entityContext, "MaterialGraphWorkspacePreview", "passes/mainrenderpipeline.azasset", true);
        m_materialPreviewContent = AZStd::make_shared<MaterialPreviewViewportContent>(
            toolId, m_materialPreviewViewport, entityContext, false);
        auto controller = AZStd::make_shared<JE::Editor::Tools::EntityPreviewViewportInputController>(
            toolId, m_materialPreviewViewport, m_materialPreviewContent);
        m_materialPreviewViewport->Init(entityContext, scene, m_materialPreviewContent, controller);
        m_materialPreviewLayout->addWidget(toolbar);
        m_materialPreviewLayout->addWidget(m_materialPreviewViewport, 1);
    }

    void MaterialGraphWorkspaceWidget::RefreshMaterialPreview()
    {
        if (!m_materialPreviewHost)
        {
            return;
        }
        JE::Editor::Tools::DocumentTypeInfo type;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            type, m_displayedDocumentId, &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        if (!isVisible() || m_previewClosed || !m_session || type.m_documentTypeName != "Material Graph")
        {
            m_materialPreviewHost->hide();
            if (m_materialPreviewContent)
            {
                m_materialPreviewContent->SetGraphDocumentId(AZ::Uuid::CreateNull());
            }
            return;
        }
        CreateMaterialPreview();
        if (m_materialPreviewContent)
        {
            m_materialPreviewContent->SetGraphDocumentId(m_displayedDocumentId);
            m_materialPreviewHost->show();
        }
    }

    void MaterialGraphWorkspaceWidget::ReleaseMaterialPreview()
    {
        if (m_materialPreviewContent)
        {
            m_materialPreviewContent->SetGraphDocumentId(AZ::Uuid::CreateNull());
        }
        m_materialPreviewContent.reset();
        delete m_materialPreviewHost;
        m_materialPreviewHost = nullptr;
        m_materialPreviewLayout = nullptr;
        m_materialPreviewViewport = nullptr;
    }

    bool MaterialGraphWorkspaceWidget::eventFilter(QObject* watched, QEvent* event)
    {
        if (!m_materialPreviewSplitInitialized && !m_materialPreviewSplitPending &&
            ((watched == m_materialPreviewHost && event->type() == QEvent::Show) ||
                (watched == m_propertiesColumn && (event->type() == QEvent::Resize || event->type() == QEvent::LayoutRequest))))
        {
            m_materialPreviewSplitPending = true;
            QTimer::singleShot(0, this, [this]()
            {
                m_materialPreviewSplitPending = false;
                InitializeMaterialPreviewSplit();
            });
        }
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
                    if (undo || redo) { setFocus(Qt::ShortcutFocusReason); }
                }
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void MaterialGraphWorkspaceWidget::SelectDocument(const AZ::Uuid& id)
    {
        const bool alreadyDisplayed = m_displayedDocumentId == id;
        if (!m_session || (!id.IsNull() && !m_session->SelectDocument(id)))
        {
            return;
        }
        // A synchronous session notification may already have refreshed this pane.
        if (!alreadyDisplayed && m_displayedDocumentId == id) { return; }
        if (m_displayedDocumentId == id)
        {
            if (m_graphView)
            {
                JE::Editor::Tools::GraphCanvas::GraphId graphId;
                JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graphId, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
                m_graphView->SetActiveGraphId(graphId, true);
            }
            RefreshMaterialPreview();
            return;
        }
        if (m_graphView)
        {
            m_graphView->SetActiveGraphId(JE::Editor::Tools::GraphCanvas::GraphId(), true);
        }
        delete m_graphView;
        m_graphView = nullptr;
        m_previewClosed = false;
        m_displayedDocumentId = id;
        JE::Editor::Tools::DocumentTypeInfo type;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            type, id, &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        const bool pass = type.m_documentTypeName == MaterialGraphEnvironment::PassGraphTypeName ||
            type.m_documentTypeName == MaterialGraphEnvironment::PassNodeConfigTypeName;
        const auto kind = pass ? MaterialGraphEnvironment::GraphKind::RenderPass : MaterialGraphEnvironment::GraphKind::Material;
        auto& environment = m_session->EnableGraphDocuments();
        environment.SetActiveGraphKind(kind);
        m_palette->ResetModel(environment.CreateNodePaletteTree(kind));
        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graphId, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
        if (graphId.IsValid())
        {
            m_graphView = aznew JE::Editor::Tools::GraphDocumentView(
                m_session->GetToolId(), id, m_session->EnableGraphDocuments().GetViewSettings(), this);
            m_viewLayout->addWidget(m_graphView);
        }
        m_propertiesMessage->setVisible(!graphId.IsValid());
        m_inspector->SetDocumentId(id);
        RefreshMaterialPreview();
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
                JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(open, id, &JE::Editor::Tools::AtomToolsDocumentRequests::IsOpen);
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
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(path, id, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(modified, id, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
            QString label = QFileInfo(QString::fromUtf8(path.c_str())).fileName();
            if (path.empty())
            {
                JE::Editor::Tools::DocumentTypeInfo type;
                JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(type, id,
                    &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
                label = QString::fromUtf8(type.m_documentTypeName.c_str());
            }
            m_documents->setItemText(index, label + (modified ? " *" : ""));
            m_documents->setItemData(index, QString::fromUtf8(path.c_str()), Qt::ToolTipRole);
        }
    }

    void MaterialGraphWorkspaceWidget::RefreshActions()
    {
        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graphId, m_displayedDocumentId, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
        bool queued = false;
        AZStd::vector<AZStd::string> outputs;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(queued, m_displayedDocumentId, &JE::Editor::Tools::GraphDocumentRequests::IsCompileGraphQueued);
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(outputs, m_displayedDocumentId, &JE::Editor::Tools::GraphDocumentRequests::GetGeneratedFilePaths);
        using CompileState = MaterialWorkspaceSession::CompileState;
        const auto state = m_session ? m_session->GetCompileState(m_displayedDocumentId) : CompileState::NotCompiled;
        const bool compiling = state == CompileState::Compiling || state == CompileState::CompilingModified;
        const bool failed = state == CompileState::Failed;
        const bool modified = state == CompileState::Modified;
        const bool hasMaterial = AZStd::any_of(outputs.begin(), outputs.end(), [](const auto& path) { return path.ends_with(".material"); });
        JE::Editor::Tools::DocumentTypeInfo type;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(type, m_displayedDocumentId,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        const bool pass = type.m_documentTypeName == MaterialGraphEnvironment::PassGraphTypeName ||
            type.m_documentTypeName == MaterialGraphEnvironment::PassNodeConfigTypeName;
        m_compile->setVisible(!pass);
        m_openMaterial->setVisible(!pass);
        m_settings->setVisible(!pass);
        m_settings->setEnabled(m_session && !pass);
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
        JE::Editor::Tools::DocumentTypeInfo type;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(type, m_displayedDocumentId,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
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
        JE::Editor::Tools::DocumentTypeInfo type;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(type, m_displayedDocumentId,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        if (type.m_documentTypeName != "Material Graph") { return; }
        AZStd::vector<AZStd::string> paths;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(paths, m_displayedDocumentId, &JE::Editor::Tools::GraphDocumentRequests::GetGeneratedFilePaths);
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
