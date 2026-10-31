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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QTableView>
#include <QPushButton>
#include <QItemSelectionModel>
#include <QLabel>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/SearchLineEdit.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/AssetEditorBus.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphOutliner/GraphOutlinerTableModel.h>

namespace JE::Editor::Tools::GraphCanvas
{
    class GraphOutlinerDockWidget
        : public AzQtComponents::StyledDockWidget
        , public JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler
        , public AssetEditorNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphOutlinerDockWidget, AZ::SystemAllocator);

        GraphOutlinerDockWidget(EditorId editorId, QWidget* parent = nullptr);
        ~GraphOutlinerDockWidget();

        // AssetEditorNotificationBus
        void OnActiveGraphChanged(const GraphId& graphId) override;
        ////

        // GraphCanvas::SceneNotifications
        void OnSelectionChanged() override;
        ////

    public Q_SLOTS:

        void OnContextMenuRequested(const QPoint& pos);
        void SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        void CreateUI();
        void OnQuickFilterChanged(const QString& text);
        void UpdateFilter();
        void OnDoubleClicked(const QModelIndex& index);
        void ClearFilter();

        QTimer m_filterTimer;
        EditorId m_editorId;
        AZ::EntityId m_activeGraphCanvasGraphId;

        NodeTableSourceModel* m_model;
        NodeTableSortProxyModel* m_proxyModel;

        AzQtComponents::SearchLineEdit* m_quickFilter;
        QTableView* m_nodelistTable;
    };
}
