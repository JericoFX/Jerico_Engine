/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/Graph/GraphView.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <AzCore/IO/FileIO.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Connections/ConnectionBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/MimeDataHandlerBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/ViewBus.h>
#include <JE/Editor/Tools/GraphCanvas/GraphCanvasBus.h>
#include <JE/Editor/Tools/GraphCanvas/Types/ConstructPresets.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/AssetEditorToolbar/AssetEditorToolbar.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/GeneralMenuActions/GeneralMenuActions.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/BookmarkContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/CollapsedNodeGroupContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/CommentContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/ConnectionContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeGroupContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SceneContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SlotContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QVBoxLayout>
#include <QWindow>

namespace JE::Editor::Tools
{
    GraphView::GraphView(
        const AZ::Crc32& toolId, const JE::Editor::Tools::GraphCanvas::GraphId& activeGraphId, GraphViewSettingsPtr graphViewSettingsPtr, QWidget* parent)
        : QWidget(parent)
        , m_toolId(toolId)
        , m_graphViewSettingsPtr(graphViewSettingsPtr)
    {
        setLayout(new QVBoxLayout());

        m_editorToolbar = aznew JE::Editor::Tools::GraphCanvas::AssetEditorToolbar(m_toolId);
        m_editorToolbar->setParent(this);
        layout()->addWidget(m_editorToolbar);

        // Screenshot
        m_takeScreenshot = new QToolButton(m_editorToolbar);
        m_takeScreenshot->setToolTip(tr("Captures a full resolution screenshot of the entire graph or selected nodes into the clipboard"));
        m_takeScreenshot->setIcon(QIcon(":/Icons/screenshot.png"));
        m_takeScreenshot->setEnabled(false);
        connect(m_takeScreenshot, &QToolButton::clicked, this, [this](){
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::ScreenshotSelection);
        });
        m_editorToolbar->AddCustomAction(m_takeScreenshot);

        m_graphicsView = new JE::Editor::Tools::GraphCanvas::GraphCanvasGraphicsView(this, false);
        m_graphicsView->SetEditorId(m_toolId);
        layout()->addWidget(m_graphicsView);

        m_presetEditor = aznew JE::Editor::Tools::GraphCanvas::ConstructPresetDialog(this);
        m_presetEditor->SetEditorId(m_toolId);

        m_presetWrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons, this);
        m_presetWrapper->setGuest(m_presetEditor);
        m_presetWrapper->hide();

        // Add a node palette for creating new nodes to the default scene context menu,
        // which is what is displayed when right-clicking on an empty space in the graph
        JE::Editor::Tools::GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = m_graphViewSettingsPtr->m_nodeMimeType.c_str();
        nodePaletteConfig.m_isInContextMenu = true;
        nodePaletteConfig.m_saveIdentifier = m_graphViewSettingsPtr->m_nodeSaveIdentifier;
        nodePaletteConfig.m_rootTreeItem = m_graphViewSettingsPtr->m_createNodeTreeItemsFn(m_toolId);
        m_sceneContextMenu = aznew JE::Editor::Tools::GraphCanvas::SceneContextMenu(m_toolId, this);
        m_sceneContextMenu->AddNodePaletteMenuAction(nodePaletteConfig);

        // Setup the context menu with node palette for proposing a new node
        // when dropping a connection in an empty space in the graph
        nodePaletteConfig.m_rootTreeItem = m_graphViewSettingsPtr->m_createNodeTreeItemsFn(m_toolId);
        m_createNodeProposalContextMenu = aznew JE::Editor::Tools::GraphCanvas::EditorContextMenu(m_toolId, this);
        m_createNodeProposalContextMenu->AddNodePaletteMenuAction(nodePaletteConfig);

        // Set up style sheet to fix highlighting in node palettes
        AzQtComponents::StyleManager::setStyleSheet(
            const_cast<JE::Editor::Tools::GraphCanvas::NodePaletteWidget*>(m_sceneContextMenu->GetNodePalette()),
            QStringLiteral(":/GraphView/GraphView.qss"));

        AzQtComponents::StyleManager::setStyleSheet(
            const_cast<JE::Editor::Tools::GraphCanvas::NodePaletteWidget*>(m_createNodeProposalContextMenu->GetNodePalette()),
            QStringLiteral(":/GraphView/GraphView.qss"));

        CreateActions();

        SetActiveGraphId(activeGraphId, true);
    }

    GraphView::~GraphView()
    {
        SetActiveGraphId(JE::Editor::Tools::GraphCanvas::GraphId(), false);
        delete m_presetEditor;
    }

    void GraphView::SetActiveGraphId(const JE::Editor::Tools::GraphCanvas::GraphId& activeGraphId, bool notify)
    {
        // Disconnect from any previously connecting buses.
        // We are enforcing that only one graph is active and connected at any given time.
        AtomToolsMainMenuRequestBus::Handler::BusDisconnect();
        JE::Editor::Tools::GraphCanvas::AssetEditorRequestBus::Handler::BusDisconnect();
        JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        // Update the value of the active graph ID and only reconnect the buses if it's valid.
        m_activeGraphId = activeGraphId;

        // Valid or not, update the graphics view to reference the new ID
        m_graphicsView->SetScene(m_activeGraphId);

        if (m_activeGraphId.IsValid())
        {
            AtomToolsMainMenuRequestBus::Handler::BusConnect(m_toolId);
            JE::Editor::Tools::GraphCanvas::AssetEditorRequestBus::Handler::BusConnect(m_toolId);
            JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_activeGraphId);

            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::SetMimeType, m_graphViewSettingsPtr->m_nodeMimeType.c_str());
        }

        if (notify)
        {
            // Notify any observers connected to the asset editor buses that the active graph has changed.
            // We are only managing one graph at a time, not using the asset editor buses, but this will update any other system that is.
            JE::Editor::Tools::GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &JE::Editor::Tools::GraphCanvas::AssetEditorNotifications::PreOnActiveGraphChanged);
            JE::Editor::Tools::GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &JE::Editor::Tools::GraphCanvas::AssetEditorNotifications::OnActiveGraphChanged, m_activeGraphId);
            JE::Editor::Tools::GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &JE::Editor::Tools::GraphCanvas::AssetEditorNotifications::PostOnActiveGraphChanged);
        }

        // Update main window menus with commands from this view.
        AtomToolsMainWindowRequestBus::Event(m_toolId, &AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, true);
    }


    void GraphView::CreateActions()
    {
        auto makeAction =
            [&](const QString& menuName, const QString& name, const AZStd::function<void()>& fn, const QKeySequence& shortcut) -> QAction*
        {
            QAction* action = new QAction(name, this);
            action->setShortcut(shortcut);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            action->setProperty("menuName", menuName);
            QObject::connect(action, &QAction::triggered, this, fn);
            addAction(action);
            return action;
        };
        auto makeSeperator =
            [&](const QString& menuName) -> QAction*
        {
            QAction* action = new QAction(this);
            action->setSeparator(true);
            action->setProperty("menuName", menuName);
            addAction(action);
            return action;
        };

        makeSeperator("menuEdit");
        m_actionCut = makeAction("menuEdit", tr("Cut"), [this](){
            JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::CutSelection);
        }, QKeySequence::Cut);
        m_actionCopy = makeAction("menuEdit", tr("Copy"), [this](){
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::CopySelection);
        }, QKeySequence::Copy);
        m_actionPaste = makeAction("menuEdit", tr("Paste"), [this](){
            JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::Paste);
        }, QKeySequence::Paste);
        m_actionDuplicate = makeAction("menuEdit", tr("Duplicate"), [this](){
            JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::DuplicateSelection);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::Key_D));
        m_actionDelete = makeAction("menuEdit", tr("Delete"), [this](){
            JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::DeleteSelection);
        }, QKeySequence::Delete);

        makeSeperator("menuEdit");
        m_actionRemoveUnusedNodes = makeAction("menuEdit", tr("Remove Unused Nodes"), [this](){
            JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::RemoveUnusedNodes);
        }, {});
        m_actionRemoveUnusedElements = makeAction("menuEdit", tr("Remove Unused Elements"), [this](){
            JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::RemoveUnusedElements);
        }, {});

        makeSeperator("menuEdit");
        m_actionSelectAll = makeAction("menuEdit", tr("Select All"), [this](){
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::SelectAll);
        }, QKeySequence::SelectAll);
        m_actionSelectInputs = makeAction("menuEdit", tr("Select Inputs"), [this](){
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::SelectAllRelative, JE::Editor::Tools::GraphCanvas::ConnectionType::CT_Input);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::Key_Left));
        m_actionSelectOutputs = makeAction("menuEdit", tr("Select Outputs"), [this](){
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::SelectAllRelative, JE::Editor::Tools::GraphCanvas::ConnectionType::CT_Output);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::Key_Right));
        m_actionSelectConnected = makeAction("menuEdit", tr("Select Connected"), [this](){
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::SelectConnectedNodes);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::Key_Up));
        m_actionSelectNone = makeAction("menuEdit", tr("Clear Selection"), [this](){
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::ClearSelection);
        }, QKeySequence::Deselect);
        m_actionSelectEnable = makeAction("menuEdit", tr("Enable Selection"), [this](){
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::EnableSelection);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::Key_K, 0x0 | Qt::CTRL | Qt::Key_U));
        m_actionSelectDisable = makeAction("menuEdit", tr("Disable Selection"), [this](){
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::DisableSelection);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::Key_K, 0x0 | Qt::CTRL | Qt::Key_C));

        makeSeperator("menuEdit");
        m_actionScreenShot = makeAction("menuEdit", tr("Screenshot"), [this](){
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::ScreenshotSelection);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::SHIFT | Qt::Key_P));

        makeSeperator("menuEdit");
        m_actionAlignTop = makeAction("menuEdit", tr("Align Top"), [this](){
            JE::Editor::Tools::GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = JE::Editor::Tools::GraphCanvas::GraphUtils::HorizontalAlignment::None;
            alignConfig.m_verAlign = JE::Editor::Tools::GraphCanvas::GraphUtils::VerticalAlignment::Top;
            JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::EventResult(
                alignConfig.m_alignTime, m_toolId, &JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::Events::GetAlignmentTime);
            AlignSelected(alignConfig);
        }, {});
        m_actionAlignBottom = makeAction("menuEdit", tr("Align Bottom"), [this](){
            JE::Editor::Tools::GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = JE::Editor::Tools::GraphCanvas::GraphUtils::HorizontalAlignment::None;
            alignConfig.m_verAlign = JE::Editor::Tools::GraphCanvas::GraphUtils::VerticalAlignment::Bottom;
            JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::EventResult(
                alignConfig.m_alignTime, m_toolId, &JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::Events::GetAlignmentTime);
            AlignSelected(alignConfig);
        }, {});
        m_actionAlignLeft = makeAction("menuEdit", tr("Align Left"), [this](){
            JE::Editor::Tools::GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = JE::Editor::Tools::GraphCanvas::GraphUtils::HorizontalAlignment::Left;
            alignConfig.m_verAlign = JE::Editor::Tools::GraphCanvas::GraphUtils::VerticalAlignment::None;
            JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::EventResult(
                alignConfig.m_alignTime, m_toolId, &JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::Events::GetAlignmentTime);
            AlignSelected(alignConfig);
        }, {});
        m_actionAlignRight = makeAction("menuEdit", tr("Align Right"), [this](){
            JE::Editor::Tools::GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = JE::Editor::Tools::GraphCanvas::GraphUtils::HorizontalAlignment::Right;
            alignConfig.m_verAlign = JE::Editor::Tools::GraphCanvas::GraphUtils::VerticalAlignment::None;
            JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::EventResult(
                alignConfig.m_alignTime, m_toolId, &JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::Events::GetAlignmentTime);
            AlignSelected(alignConfig);
        }, {});

        makeSeperator("menuView");
        m_actionPresetEditor = makeAction("menuView", tr("Presets Editor"), [this](){ OpenPresetsEditor(); }, {});

        makeSeperator("menuView");
        m_actionShowEntireGraph = makeAction("menuView", tr("Show Entire Graph"), [this](){
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::ShowEntireGraph);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::SHIFT | Qt::Key_Down));
        m_actionZoomIn = makeAction("menuView", tr("Zoom In"), [this](){
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::ZoomIn);
        }, QKeySequence::ZoomIn);
        m_actionZoomOut = makeAction("menuView", tr("Zoom Out"), [this](){
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::ZoomOut);
        }, QKeySequence::ZoomOut);
        m_actionZoomSelection = makeAction("menuView", tr("Zoom Selection"), [this](){
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::CenterOnSelection);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::SHIFT | Qt::Key_Up));

        makeSeperator("menuView");
        m_actionGotoStartOfChain = makeAction("menuView", tr("Goto Start Of Chain"), [this](){
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::CenterOnStartOfChain);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::SHIFT | Qt::Key_Left));
        m_actionGotoEndOfChain = makeAction("menuView", tr("Goto End Of Chain"), [this](){
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::CenterOnEndOfChain);
        }, QKeySequence(0x0 | Qt::CTRL | Qt::SHIFT | Qt::Key_Right));
    }

    void GraphView::CreateMenus(QMenuBar* menuBar)
    {
        for (auto action : actions())
        {
            if (auto menuName = action->property("menuName"); menuName.isValid())
            {
                if (auto menu = menuBar->findChild<QMenu*>(menuName.toString()))
                {
                    menu->addAction(action);
                }
            }
        }
    }

    void GraphView::UpdateMenus([[maybe_unused]] QMenuBar* menuBar)
    {
        const bool hasGraph = m_activeGraphId.IsValid();

        bool hasSelection = false;
        bool hasCopiableSelection = false;
        if (hasGraph)
        {
            AzToolsFramework::EntityIdList selectedItems;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(selectedItems, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetSelectedItems);
            hasSelection = !selectedItems.empty();

            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(
                hasCopiableSelection, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::HasCopiableSelection);
        }

        // Enable the Paste action if the clipboard (if any) has a mime type that we support
        AZStd::string copyMimeType;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(copyMimeType, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetCopyMimeType);
        const bool canPaste = hasGraph && !copyMimeType.empty() && QApplication::clipboard()->mimeData()->hasFormat(copyMimeType.c_str());

        m_actionCut->setEnabled(hasCopiableSelection);
        m_actionCopy->setEnabled(hasCopiableSelection);
        m_actionPaste->setEnabled(canPaste);
        m_actionDelete->setEnabled(hasSelection);
        m_actionDuplicate->setEnabled(hasCopiableSelection);

        m_actionRemoveUnusedNodes->setEnabled(hasGraph);
        m_actionRemoveUnusedElements->setEnabled(hasGraph);

        m_actionSelectAll->setEnabled(hasGraph);
        m_actionSelectNone->setEnabled(hasSelection);
        m_actionSelectInputs->setEnabled(hasGraph);
        m_actionSelectOutputs->setEnabled(hasGraph);
        m_actionSelectConnected->setEnabled(hasGraph);
        m_actionSelectEnable->setEnabled(hasGraph);
        m_actionSelectDisable->setEnabled(hasGraph);

        m_actionScreenShot->setEnabled(hasGraph);

        m_actionAlignTop->setEnabled(hasSelection);
        m_actionAlignBottom->setEnabled(hasSelection);
        m_actionAlignLeft->setEnabled(hasSelection);
        m_actionAlignRight->setEnabled(hasSelection);

        m_actionPresetEditor->setEnabled(hasGraph);
        m_actionShowEntireGraph->setEnabled(hasGraph);
        m_actionZoomIn->setEnabled(hasGraph);
        m_actionZoomOut->setEnabled(hasGraph);
        m_actionZoomSelection->setEnabled(hasSelection);
        m_actionGotoStartOfChain->setEnabled(hasGraph);
        m_actionGotoEndOfChain->setEnabled(hasGraph);

        m_takeScreenshot->setEnabled(hasGraph);
    }

    AZ::s32 GraphView::GetMainMenuPriority() const
    {
        // Return a priority that will place menus for the view below menus for the main window
        return 1;
    }

    AZ::EntityId GraphView::CreateNewGraph()
    {
        return m_activeGraphId;
    }

    bool GraphView::ContainsGraph([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::GraphId& graphId) const
    {
        return m_activeGraphId == graphId;
    }

    bool GraphView::CloseGraph([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::GraphId& graphId)
    {
        return false;
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint)
    {
        m_sceneContextMenu->ResetSourceSlotFilter();

        // We pass an invalid EntityId here since this is for the scene, there is no member to specify.
        return HandleContextMenu(*m_sceneContextMenu, AZ::EntityId(), screenPoint, scenePoint);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowNodeContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        JE::Editor::Tools::GraphCanvas::NodeContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowCommentContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        JE::Editor::Tools::GraphCanvas::CommentContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowNodeGroupContextMenu(
        const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        JE::Editor::Tools::GraphCanvas::NodeGroupContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, groupId, screenPoint, scenePoint);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowCollapsedNodeGroupContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        JE::Editor::Tools::GraphCanvas::CollapsedNodeGroupContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowBookmarkContextMenu(
        const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        JE::Editor::Tools::GraphCanvas::BookmarkContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, bookmarkId, screenPoint, scenePoint);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowConnectionContextMenu(
        const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        JE::Editor::Tools::GraphCanvas::ConnectionContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, connectionId, screenPoint, scenePoint);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowSlotContextMenu(
        const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        JE::Editor::Tools::GraphCanvas::SlotContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, slotId, screenPoint, scenePoint);
    }

    JE::Editor::Tools::GraphCanvas::Endpoint GraphView::CreateNodeForProposal(
        const AZ::EntityId& connectionId, const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        JE::Editor::Tools::GraphCanvas::Endpoint retVal;

        m_createNodeProposalContextMenu->FilterForSourceSlot(m_activeGraphId, endpoint.GetSlotId());
        m_createNodeProposalContextMenu->RefreshActions(m_activeGraphId, connectionId);

        m_createNodeProposalContextMenu->exec(screenPoint);

        JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_createNodeProposalContextMenu->GetNodePalette()->GetContextMenuEvent();
        if (mimeEvent)
        {
            JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);

            AZ::Vector2 dropPos(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
            if (mimeEvent->ExecuteEvent(dropPos, dropPos, m_activeGraphId))
            {
                JE::Editor::Tools::GraphCanvas::NodeId nodeId = mimeEvent->GetCreatedNodeId();
                if (nodeId.IsValid())
                {
                    JE::Editor::Tools::GraphCanvas::VisualRequestBus::Event(nodeId, &JE::Editor::Tools::GraphCanvas::VisualRequests::SetVisible, false);
                    retVal = HandleProposedConnection(m_activeGraphId, connectionId, endpoint, nodeId, screenPoint);
                }

                if (retVal.IsValid())
                {
                    JE::Editor::Tools::GraphCanvas::GraphUtils::CreateOpportunisticConnectionsBetween(endpoint, retVal);
                    JE::Editor::Tools::GraphCanvas::VisualRequestBus::Event(nodeId, &JE::Editor::Tools::GraphCanvas::VisualRequests::SetVisible, true);

                    AZ::Vector2 position;
                    JE::Editor::Tools::GraphCanvas::GeometryRequestBus::EventResult(position, retVal.GetNodeId(), &JE::Editor::Tools::GraphCanvas::GeometryRequests::GetPosition);

                    QPointF connectionPoint;
                    JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(
                        connectionPoint, retVal.GetSlotId(), &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetConnectionPoint);

                    qreal verticalOffset = connectionPoint.y() - position.GetY();
                    position.SetY(aznumeric_cast<float>(scenePoint.y() - verticalOffset));

                    qreal horizontalOffset = connectionPoint.x() - position.GetX();
                    position.SetX(aznumeric_cast<float>(scenePoint.x() - horizontalOffset));

                    JE::Editor::Tools::GraphCanvas::GeometryRequestBus::Event(retVal.GetNodeId(), &JE::Editor::Tools::GraphCanvas::GeometryRequests::SetPosition, position);

                    JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneNotifications::PostCreationEvent);
                }
                else
                {
                    JE::Editor::Tools::GraphCanvas::GraphUtils::DeleteOutermostNode(m_activeGraphId, nodeId);
                }
            }
        }

        return retVal;
    }

    void GraphView::OnWrapperNodeActionWidgetClicked(
        [[maybe_unused]] const AZ::EntityId& wrapperNode,
        [[maybe_unused]] const QRect& actionWidgetBoundingRect,
        [[maybe_unused]] const QPointF& scenePoint,
        [[maybe_unused]] const QPoint& screenPoint)
    {
    }

    void GraphView::OnSelectionChanged()
    {
        AtomToolsMainWindowRequestBus::Event(m_toolId, &AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, false);
    }

    JE::Editor::Tools::GraphCanvas::Endpoint GraphView::HandleProposedConnection(
        [[maybe_unused]] const JE::Editor::Tools::GraphCanvas::GraphId& graphId,
        [[maybe_unused]] const JE::Editor::Tools::GraphCanvas::ConnectionId& connectionId,
        const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint,
        const JE::Editor::Tools::GraphCanvas::NodeId& proposedNode,
        const QPoint& screenPoint)
    {
        JE::Editor::Tools::GraphCanvas::Endpoint retVal;

        JE::Editor::Tools::GraphCanvas::ConnectionType connectionType = JE::Editor::Tools::GraphCanvas::ConnectionType::CT_Invalid;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(connectionType, endpoint.GetSlotId(), &JE::Editor::Tools::GraphCanvas::SlotRequests::GetConnectionType);

        JE::Editor::Tools::GraphCanvas::NodeId currentTarget = proposedNode;

        while (!retVal.IsValid() && currentTarget.IsValid())
        {
            AZStd::vector<AZ::EntityId> targetSlotIds;
            JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(targetSlotIds, currentTarget, &JE::Editor::Tools::GraphCanvas::NodeRequests::GetSlotIds);

            // Find the list of endpoints on the created node that could create a valid connection
            // with the specified slot
            AZStd::list<JE::Editor::Tools::GraphCanvas::Endpoint> endpoints;
            for (const auto& targetSlotId : targetSlotIds)
            {
                JE::Editor::Tools::GraphCanvas::Endpoint proposedEndpoint(currentTarget, targetSlotId);

                bool canCreate = false;
                JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(canCreate, endpoint.GetSlotId(), &JE::Editor::Tools::GraphCanvas::SlotRequests::CanCreateConnectionTo, proposedEndpoint);

                if (canCreate)
                {
                    JE::Editor::Tools::GraphCanvas::SlotGroup slotGroup = JE::Editor::Tools::GraphCanvas::SlotGroups::Invalid;
                    JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(slotGroup, targetSlotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetSlotGroup);

                    bool isVisible = slotGroup != JE::Editor::Tools::GraphCanvas::SlotGroups::Invalid;
                    JE::Editor::Tools::GraphCanvas::SlotLayoutRequestBus::EventResult(isVisible, currentTarget, &JE::Editor::Tools::GraphCanvas::SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

                    if (isVisible)
                    {
                        endpoints.push_back(proposedEndpoint);
                    }
                }
            }

            if (!endpoints.empty())
            {
                // If there is exactly one match, then we can just use that endpoint.
                if (endpoints.size() == 1)
                {
                    retVal = endpoints.front();
                }
                // Otherwise, since there are multiple possible matches, we need to display a simple menu for the
                // user to select which slot they want they want to be connected to the proposed endpoint.
                else
                {
                    QMenu menu;

                    for (JE::Editor::Tools::GraphCanvas::Endpoint proposedEndpoint : endpoints)
                    {
                        menu.addAction(aznew JE::Editor::Tools::GraphCanvas::EndpointSelectionAction(proposedEndpoint));
                    }

                    QAction* result = menu.exec(screenPoint);

                    if (result)
                    {
                        auto* selectedEnpointAction = static_cast<JE::Editor::Tools::GraphCanvas::EndpointSelectionAction*>(result);
                        retVal = selectedEnpointAction->GetEndpoint();
                    }
                    else
                    {
                        retVal.Clear();
                    }
                }

                if (retVal.IsValid())
                {
                    // Double safety check. This should be guaranteed by the previous checks. But just extra safety.
                    bool canCreateConnection = false;
                    JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(canCreateConnection, endpoint.GetSlotId(), &JE::Editor::Tools::GraphCanvas::SlotRequests::CanCreateConnectionTo, retVal);

                    if (!canCreateConnection)
                    {
                        retVal.Clear();
                    }
                }
            }
            else
            {
                retVal.Clear();
            }

            if (!retVal.IsValid())
            {
                bool isWrapped = false;
                JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(isWrapped, currentTarget, &JE::Editor::Tools::GraphCanvas::NodeRequests::IsWrapped);

                if (isWrapped)
                {
                    JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(currentTarget, currentTarget, &JE::Editor::Tools::GraphCanvas::NodeRequests::GetWrappingNode);
                }
                else
                {
                    currentTarget.SetInvalid();
                }
            }
        }

        return retVal;
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction GraphView::HandleContextMenu(
        JE::Editor::Tools::GraphCanvas::EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const
    {
        AZ::Vector2 sceneVector(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));

        editorContextMenu.RefreshActions(m_activeGraphId, memberId);

        QAction* result = editorContextMenu.exec(screenPoint);

        if (auto contextMenuAction = qobject_cast<JE::Editor::Tools::GraphCanvas::ContextMenuAction*>(result))
        {
            JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            return contextMenuAction->TriggerAction(m_activeGraphId, sceneVector);
        }

        if (editorContextMenu.GetNodePalette())
        {
            // Handle creating node from any node palette embedded in an GraphCanvas::EditorContextMenu.
            JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent* mimeEvent = editorContextMenu.GetNodePalette()->GetContextMenuEvent();
            if (mimeEvent)
            {
                JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);

                AZ::Vector2 dropPos(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
                if (mimeEvent->ExecuteEvent(dropPos, dropPos, m_activeGraphId))
                {
                    JE::Editor::Tools::GraphCanvas::NodeId nodeId = mimeEvent->GetCreatedNodeId();
                    if (nodeId.IsValid())
                    {
                        JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::ClearSelection);
                        JE::Editor::Tools::GraphCanvas::VisualRequestBus::Event(nodeId, &JE::Editor::Tools::GraphCanvas::VisualRequests::SetVisible, true);
                        JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::Event(nodeId, &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::SetSelected, true);
                        JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneNotifications::PostCreationEvent);
                    }
                }
            }
        }

        return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }

    void GraphView::AlignSelected(const JE::Editor::Tools::GraphCanvas::AlignConfig& alignConfig)
    {
        JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);

        AZStd::vector<JE::Editor::Tools::GraphCanvas::NodeId> selectedNodes;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetSelectedNodes);
        JE::Editor::Tools::GraphCanvas::GraphUtils::AlignNodes(selectedNodes, alignConfig);
    }

    void GraphView::OpenPresetsEditor()
    {
        if (m_presetEditor && m_presetWrapper)
        {
            QSize boundingBox = size();
            QPointF newPosition =
                mapToGlobal(QPoint(aznumeric_cast<int>(boundingBox.width() * 0.5f), aznumeric_cast<int>(boundingBox.height() * 0.5f)));

            m_presetEditor->show();

            m_presetWrapper->show();
            m_presetWrapper->raise();
            m_presetWrapper->activateWindow();

            QRect geometry = m_presetWrapper->geometry();
            QSize originalSize = geometry.size();

            newPosition.setX(newPosition.x() - geometry.width() * 0.5f);
            newPosition.setY(newPosition.y() - geometry.height() * 0.5f);

            geometry.setTopLeft(newPosition.toPoint());

            geometry.setWidth(originalSize.width());
            geometry.setHeight(originalSize.height());

            m_presetWrapper->setGeometry(geometry);
        }
    }
} // namespace JE::Editor::Tools

