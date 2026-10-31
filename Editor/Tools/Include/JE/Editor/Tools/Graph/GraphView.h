/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Tools/Graph/GraphViewSettings.h>
#include <JE/Editor/Tools/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Connections/ConnectionBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/AssetEditorBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/EditorTypes.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/ConstructPresetDialog/ConstructPresetDialog.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <QToolButton>
#include <QWidget>

namespace JE::Editor::Tools::GraphCanvas
{
    class AssetEditorToolbar;
    class GraphCanvasMimeEvent;
    class MiniMapDockWidget;
    class ConstructPresetDialog;
    class AssetEditorToolbar;
    class NodePaletteDockWidget;
    class BookmarkDockWidget;
    class SceneContextMenu;
    class EditorContextMenu;

    namespace Styling
    {
        class StyleSheet;
    }
} // namespace GraphCanvas

namespace JE::Editor::Tools
{
    //! GraphView manages displaying and interacting with a single Graph Canvas graph. Graph Canvas has other classes like the
    //! AssetEditorMainWindow that manage everything related to multiple graphs but it is too heavyweight and incompatible with the
    //! AtomToolsFramework application, main window, and document system. This class focuses on a single graph that might typically be
    //! represented or wrapped by a document class. It might be possible to reimplement AssetEditorMainWindow in terms of this to
    //! consolidate code and features.
    class GraphView
        : public QWidget
        , private JE::Editor::Tools::AtomToolsMainMenuRequestBus::Handler
        , private JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler
        , private JE::Editor::Tools::GraphCanvas::AssetEditorRequestBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphView, AZ::SystemAllocator);

        GraphView(
            const AZ::Crc32& toolId,
            const JE::Editor::Tools::GraphCanvas::GraphId& activeGraphId,
            GraphViewSettingsPtr graphViewSettingsPtr,
            QWidget* parent = nullptr);
        ~GraphView();

        // Assign the graph ID that will be rendered and edited within the view. Any connected buses will be disconnected from the old ID
        // before connecting to the new one.
        // @param activeGraphId ID of the graph that will be managed by this view. If the ID is invalid then the view will be cleared.
        // @param notify Send a notification that the active graph has changed.
        void SetActiveGraphId(const JE::Editor::Tools::GraphCanvas::GraphId& activeGraphId, bool notify);

    protected:
        void CreateActions();

        // AtomToolsFramework::AtomToolsMainMenuRequestBus::Handler overrides...
        void CreateMenus(QMenuBar* menuBar) override;
        void UpdateMenus(QMenuBar* menuBar) override;
        AZ::s32 GetMainMenuPriority() const override;

        // GraphCanvas::AssetEditorRequestBus::Handler overrides...
        AZ::EntityId CreateNewGraph() override;
        bool ContainsGraph(const JE::Editor::Tools::GraphCanvas::GraphId& graphId) const override;
        bool CloseGraph(const JE::Editor::Tools::GraphCanvas::GraphId& graphId) override;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint) override;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ShowNodeContextMenu(
            const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ShowCommentContextMenu(
            const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ShowNodeGroupContextMenu(
            const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ShowCollapsedNodeGroupContextMenu(
            const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ShowBookmarkContextMenu(
            const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ShowConnectionContextMenu(
            const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ShowSlotContextMenu(
            const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        JE::Editor::Tools::GraphCanvas::Endpoint CreateNodeForProposal(
            const AZ::EntityId& connectionId,
            const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint,
            const QPointF& scenePoint,
            const QPoint& screenPoint) override;
        void OnWrapperNodeActionWidgetClicked(
            const AZ::EntityId& wrapperNode,
            const QRect& actionWidgetBoundingRect,
            const QPointF& scenePoint,
            const QPoint& screenPoint) override;

        // GraphCanvas::SceneNotificationBus::Handler overrides...
        void OnSelectionChanged() override;

        JE::Editor::Tools::GraphCanvas::Endpoint HandleProposedConnection(
            const JE::Editor::Tools::GraphCanvas::GraphId& graphId,
            const JE::Editor::Tools::GraphCanvas::ConnectionId& connectionId,
            const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint,
            const JE::Editor::Tools::GraphCanvas::NodeId& proposedNode,
            const QPoint& screenPoint);

        JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction HandleContextMenu(
            JE::Editor::Tools::GraphCanvas::EditorContextMenu& editorContextMenu,
            const AZ::EntityId& memberId,
            const QPoint& screenPoint,
            const QPointF& scenePoint) const;

        void AlignSelected(const JE::Editor::Tools::GraphCanvas::AlignConfig& alignConfig);
        void OpenPresetsEditor();

        const AZ::Crc32 m_toolId;
        JE::Editor::Tools::GraphCanvas::GraphId m_activeGraphId;

        GraphViewSettingsPtr m_graphViewSettingsPtr;
        AzQtComponents::WindowDecorationWrapper* m_presetWrapper = {};
        JE::Editor::Tools::GraphCanvas::ConstructPresetDialog* m_presetEditor = {};
        JE::Editor::Tools::GraphCanvas::GraphCanvasGraphicsView* m_graphicsView = {};
        JE::Editor::Tools::GraphCanvas::AssetEditorToolbar* m_editorToolbar = {};
        JE::Editor::Tools::GraphCanvas::SceneContextMenu* m_sceneContextMenu = {};
        JE::Editor::Tools::GraphCanvas::EditorContextMenu* m_createNodeProposalContextMenu = {};

        QAction* m_actionCut = {};
        QAction* m_actionCopy = {};
        QAction* m_actionPaste = {};
        QAction* m_actionDelete = {};
        QAction* m_actionDuplicate = {};

        QAction* m_actionRemoveUnusedNodes = {};
        QAction* m_actionRemoveUnusedElements = {};

        QAction* m_actionSelectAll = {};
        QAction* m_actionSelectNone = {};
        QAction* m_actionSelectInputs = {};
        QAction* m_actionSelectOutputs = {};
        QAction* m_actionSelectConnected = {};
        QAction* m_actionSelectEnable = {};
        QAction* m_actionSelectDisable = {};

        QAction* m_actionScreenShot = {};

        QAction* m_actionAlignTop = {};
        QAction* m_actionAlignBottom = {};
        QAction* m_actionAlignLeft = {};
        QAction* m_actionAlignRight = {};

        QAction* m_actionPresetEditor = {};
        QAction* m_actionShowEntireGraph = {};
        QAction* m_actionZoomIn = {};
        QAction* m_actionZoomOut = {};
        QAction* m_actionZoomSelection = {};
        QAction* m_actionGotoStartOfChain = {};
        QAction* m_actionGotoEndOfChain = {};

        QToolButton* m_takeScreenshot = {};
    };
} // namespace JE::Editor::Tools
