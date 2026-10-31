/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/SettingsDialog/SettingsDialog.h>
#include <AzCore/IO/FileIO.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Window/PassCanvasMainWindow.h>
#include <Window/PassCanvasViewportContent.h>

#include <QMessageBox>

namespace PassCanvas
{
    PassCanvasMainWindow::PassCanvasMainWindow(
        const AZ::Crc32& toolId, JE::Editor::Tools::GraphViewSettingsPtr graphViewSettingsPtr, QWidget* parent)
        : Base(toolId, "PassCanvasMainWindow", parent)
        , m_graphViewSettingsPtr(graphViewSettingsPtr)
        , m_styleManager(toolId, graphViewSettingsPtr->m_styleManagerPath)
    {
        m_assetBrowser->GetSearchWidget()->SetFilterState("", JE::Render::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->GetSearchWidget()->SetFilterState("", JE::Render::RPI::PassAsset::Group, true);

        m_documentInspector = new JE::Editor::Tools::AtomToolsDocumentInspector(m_toolId, this);
        m_documentInspector->SetDocumentSettingsPrefix("/O3DE/Atom/PassCanvas/DocumentInspector");
        AddDockWidget("Inspector", m_documentInspector, Qt::RightDockWidgetArea);

        // Set up the toolbar that controls the viewport settings
        m_toolBar = new JE::Editor::Tools::EntityPreviewViewportToolBar(m_toolId, this);

        // Create the dockable viewport widget that will be shared between all Pass Canvas documents
        m_passViewport = new JE::Editor::Tools::EntityPreviewViewportWidget(m_toolId, this);

        // Initialize the entity context that will be used to create all of the entities displayed in the viewport
        auto entityContext = AZStd::make_shared<AzFramework::EntityContext>();
        entityContext->InitContext();

        // Initialize the atom scene and pipeline that will bind to the viewport window to render entities and presets
        auto viewportScene = AZStd::make_shared<JE::Editor::Tools::EntityPreviewViewportScene>(
            m_toolId, m_passViewport, entityContext, "PassCanvasViewportWidget", "passes/mainrenderpipeline.azasset");

        // Viewport content will instantiate all of the entities that will be displayed and controlled by the viewport
        auto viewportContent = AZStd::make_shared<PassCanvasViewportContent>(m_toolId, m_passViewport, entityContext);

        // The input controller creates and binds input behaviors to control viewport objects
        auto viewportController = AZStd::make_shared<JE::Editor::Tools::EntityPreviewViewportInputController>(m_toolId, m_passViewport, viewportContent);

        // Inject the entity context, scene, content, and controller into the viewport widget
        m_passViewport->Init(entityContext, viewportScene, viewportContent, viewportController);

        // Combine the shared toolbar in viewport into stacked widget that will be docked as a single view
        auto viewPortAndToolbar = new QWidget(this);
        viewPortAndToolbar->setLayout(new QVBoxLayout(viewPortAndToolbar));
        viewPortAndToolbar->layout()->setContentsMargins(0, 0, 0, 0);
        viewPortAndToolbar->layout()->setSpacing(0);
        viewPortAndToolbar->layout()->addWidget(m_toolBar);
        viewPortAndToolbar->layout()->addWidget(m_passViewport);

        AddDockWidget("Viewport", viewPortAndToolbar, Qt::BottomDockWidgetArea);

        m_viewportSettingsInspector = new JE::Editor::Tools::EntityPreviewViewportSettingsInspector(m_toolId, this);
        AddDockWidget("Viewport Settings", m_viewportSettingsInspector, Qt::LeftDockWidgetArea);
        SetDockWidgetVisible("Viewport Settings", false);

        m_bookmarkDockWidget = aznew JE::Editor::Tools::GraphCanvas::BookmarkDockWidget(m_toolId, this);
        AddDockWidget("Bookmarks", m_bookmarkDockWidget, Qt::BottomDockWidgetArea);
        SetDockWidgetVisible("Bookmarks", false);

        AddDockWidget("MiniMap", aznew JE::Editor::Tools::GraphCanvas::MiniMapDockWidget(m_toolId, this), Qt::BottomDockWidgetArea);
        SetDockWidgetVisible("MiniMap", false);

        JE::Editor::Tools::GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_rootTreeItem = m_graphViewSettingsPtr->m_createNodeTreeItemsFn(m_toolId);
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = m_graphViewSettingsPtr->m_nodeMimeType.c_str();
        nodePaletteConfig.m_isInContextMenu = false;
        nodePaletteConfig.m_saveIdentifier = m_graphViewSettingsPtr->m_nodeSaveIdentifier;

        m_nodePalette = aznew JE::Editor::Tools::GraphCanvas::NodePaletteDockWidget(this, "Node Palette", nodePaletteConfig);
        AddDockWidget("Node Palette", m_nodePalette, Qt::LeftDockWidgetArea);

        AZ::IO::FixedMaxPath resolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(resolvedPath, m_graphViewSettingsPtr->m_translationPath.c_str());
        const AZ::IO::FixedMaxPathString translationFilePath = resolvedPath.LexicallyNormal().FixedMaxPathString();
        if (m_translator.load(QLocale::Language::English, translationFilePath.c_str()))
        {
            if (!qApp->installTranslator(&m_translator))
            {
                AZ_Warning("PassCanvas", false, "Error installing translation %s!", translationFilePath.c_str());
            }
        }
        else
        {
            AZ_Warning("PassCanvas", false, "Error loading translation file %s", translationFilePath.c_str());
        }

        // Set up style sheet to fix highlighting in the node palette
        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral(":/GraphView/GraphView.qss"));

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    void PassCanvasMainWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        Base::OnDocumentOpened(documentId);
        m_documentInspector->SetDocumentId(documentId);
    }

    void PassCanvasMainWindow::ResizeViewportRenderTarget(uint32_t width, uint32_t height)
    {
        QSize requestedViewportSize = QSize(width, height) / devicePixelRatioF();
        QSize currentViewportSize = m_passViewport->size();
        QSize offset = requestedViewportSize - currentViewportSize;
        QSize requestedWindowSize = size() + offset;
        resize(requestedWindowSize);

        AZ_Assert(
            m_passViewport->size() == requestedViewportSize,
            "Resizing the window did not give the expected viewport size. Requested %d x %d but got %d x %d.",
            requestedViewportSize.width(), requestedViewportSize.height(), m_passViewport->size().width(),
            m_passViewport->size().height());

        [[maybe_unused]] QSize newDeviceSize = m_passViewport->size();
        AZ_Warning(
            "Pass Canvas",
            static_cast<uint32_t>(newDeviceSize.width()) == width && static_cast<uint32_t>(newDeviceSize.height()) == height,
            "Resizing the window did not give the expected frame size. Requested %d x %d but got %d x %d.", width, height,
            newDeviceSize.width(), newDeviceSize.height());
    }

    void PassCanvasMainWindow::LockViewportRenderTargetSize(uint32_t width, uint32_t height)
    {
        m_passViewport->LockRenderTargetSize(width, height);
    }

    void PassCanvasMainWindow::UnlockViewportRenderTargetSize()
    {
        m_passViewport->UnlockRenderTargetSize();
    }

    void PassCanvasMainWindow::PopulateSettingsInspector(JE::Editor::Tools::InspectorWidget* inspector) const
    {
        m_passCanvasCompileSettingsGroup = JE::Editor::Tools::CreateSettingsPropertyGroup(
            "Pass Canvas Settings",
            "Pass Canvas Settings",
            { JE::Editor::Tools::CreateSettingsPropertyValue(
                  "/O3DE/Atom/PassCanvas/CreateDefaultDocumentOnStart",
                  "Create Untitled Graph Document On Start",
                  "Create a default, untitled graph document when Pass Canvas starts",
                  true) });

        inspector->AddGroup(
            m_passCanvasCompileSettingsGroup->m_name,
            m_passCanvasCompileSettingsGroup->m_displayName,
            m_passCanvasCompileSettingsGroup->m_description,
            new JE::Editor::Tools::InspectorPropertyGroupWidget(
                m_passCanvasCompileSettingsGroup.get(),
                m_passCanvasCompileSettingsGroup.get(),
                azrtti_typeid<JE::Editor::Tools::DynamicPropertyGroup>()));

        inspector->AddGroup(
            "Graph View Settings",
            "Graph View Settings",
            "Configuration settings for the graph view interaction, animation, and other behavior.",
            new JE::Editor::Tools::InspectorPropertyGroupWidget(
                m_graphViewSettingsPtr.get(), m_graphViewSettingsPtr.get(), m_graphViewSettingsPtr->RTTI_Type()));

        Base::PopulateSettingsInspector(inspector);
    }

    void PassCanvasMainWindow::OnSettingsDialogClosed()
    {
        JE::Editor::Tools::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", m_graphViewSettingsPtr);
        Base::OnSettingsDialogClosed();
    }

    AZStd::string PassCanvasMainWindow::GetHelpUrl() const
    {
        return "https://docs.o3de.org/docs/atom-guide/look-dev/tools/";
    }
} // namespace PassCanvas

