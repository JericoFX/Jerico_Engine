/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QAction>
#include <QTimer>
#include <QLabel>
#include <QItemSelectionModel>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/AssetEditorBus.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/Bookmarks/BookmarkTableModel.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>

class QSortFilterProxyModel;

namespace Ui
{
    class BookmarkDockWidget;
}

namespace JE::Editor::Tools::GraphCanvas
{
    class BookmarkDockWidget
        : public AzQtComponents::StyledDockWidget
        , public BookmarkTableRequestBus::Handler
        , public JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler
        , public AssetEditorNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(BookmarkDockWidget, AZ::SystemAllocator);

        BookmarkDockWidget(EditorId editorId, QWidget* parent = nullptr);
        ~BookmarkDockWidget();

        // AssetEditorNotificationBus
        void OnActiveGraphChanged(const GraphId& graphId) override;
        ////

        // GraphCanvas::GraphCanvasTreeModelRequestBus::Handler
        void ClearSelection() override;
        ////

        // GraphCanvas::SceneNotifications
        void OnSelectionChanged() override;
        ////

    public Q_SLOTS:

        void OnCreateBookmark();
        void OnDeleteBookmark();
        void OnContextMenuRequested(const QPoint &pos);
        void SelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    private:

        void OnQuickFilterChanged(const QString &text);
        void UpdateFilter();
        void ClearFilter();

        AZStd::unique_ptr<Ui::BookmarkDockWidget> m_ui;

        QTimer m_filterTimer;

        EditorId m_editorId;

        AZ::EntityId m_activeGraphCanvasGraphId;
        AZ::EntityId m_remapTarget;

        JE::Editor::Tools::GraphCanvas::BookmarkTableSourceModel*      m_model;
        JE::Editor::Tools::GraphCanvas::BookmarkTableSortProxyModel*   m_proxyModel;
    };
}
