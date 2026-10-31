/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>

// GraphCanvas
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>

// GraphModel
#include <JE/Editor/Tools/GraphModel/Integration/EditorMainWindow.h>
#include <JE/Editor/Tools/GraphModel/Model/Graph.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    EditorMainWindow::EditorMainWindow(JE::Editor::Tools::GraphCanvas::AssetEditorWindowConfig* config, QWidget* parent)
        : JE::Editor::Tools::GraphCanvas::AssetEditorMainWindow(config, parent)
    {
    }

    EditorMainWindow::~EditorMainWindow()
    {
        GraphControllerNotificationBus::MultiHandler::BusDisconnect();
    }

    JE::Editor::Tools::GraphModel::GraphPtr EditorMainWindow::GetGraphById(JE::Editor::Tools::GraphCanvas::GraphId graphId) const
    {
        auto it = m_graphs.find(graphId);
        if (it != m_graphs.end())
        {
            return it->second;
        }

        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::GraphId EditorMainWindow::GetGraphId(JE::Editor::Tools::GraphModel::GraphPtr graph) const
    {
        auto it = AZStd::find_if(m_graphs.begin(), m_graphs.end(),
            [graph](decltype(m_graphs)::const_reference pair)
        {
            return graph == pair.second;
        });
        if (it != m_graphs.end())
        {
            return it->first;
        }

        return JE::Editor::Tools::GraphCanvas::GraphId();
    }

    void EditorMainWindow::OnEditorOpened(JE::Editor::Tools::GraphCanvas::EditorDockWidget* dockWidget)
    {
        JE::Editor::Tools::GraphCanvas::AssetEditorMainWindow::OnEditorOpened(dockWidget);

        JE::Editor::Tools::GraphCanvas::GraphId graphId = dockWidget->GetGraphId();

        // Create the new graph.
        JE::Editor::Tools::GraphModel::GraphPtr graph = AZStd::make_shared<JE::Editor::Tools::GraphModel::Graph>(GetGraphContext());
        m_graphs[graphId] = graph;

        // Create the controller for the new graph.
        JE::Editor::Tools::GraphModel::Integration::GraphManagerRequestBus::Broadcast(&JE::Editor::Tools::GraphModel::Integration::GraphManagerRequests::CreateGraphController, graphId, graph);

        // Listen for GraphController notifications on the new graph.
        JE::Editor::Tools::GraphModel::Integration::GraphControllerNotificationBus::MultiHandler::BusConnect(graphId);
    }

    void EditorMainWindow::OnEditorClosing(JE::Editor::Tools::GraphCanvas::EditorDockWidget* dockWidget)
    {
        JE::Editor::Tools::GraphCanvas::AssetEditorMainWindow::OnEditorClosing(dockWidget);

        JE::Editor::Tools::GraphCanvas::GraphId graphId = dockWidget->GetGraphId();

        // Stop listening for GraphController notifications for this graph.
        JE::Editor::Tools::GraphModel::Integration::GraphControllerNotificationBus::MultiHandler::BusDisconnect(graphId);

        // Remove the controller for this graph.
        JE::Editor::Tools::GraphModel::Integration::GraphManagerRequestBus::Broadcast(&JE::Editor::Tools::GraphModel::Integration::GraphManagerRequests::DeleteGraphController, graphId);

        // Delete the graph that was created.
        m_graphs.erase(graphId);
    }

    void EditorMainWindow::OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        // Find the GraphModel::NodePtr whose action widget was clicked.
        JE::Editor::Tools::GraphCanvas::GraphId graphId = GetActiveGraphCanvasGraphId();
        JE::Editor::Tools::GraphModel::NodePtr node;
        GraphControllerRequestBus::EventResult(node, graphId, &GraphControllerRequests::GetNodeById, wrapperNode);
        AZ_Assert(node, "Unable to find NodePtr for the given NodeId");

        // Invoke the handler so that the client can handle this event if necessary.
        HandleWrapperNodeActionWidgetClicked(node, actionWidgetBoundingRect, scenePoint, screenPoint);
    }
}
