/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// GraphModel
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>

// Landscape Canvas
#include <Editor/Core/Core.h>
#include <Editor/Nodes/BaseNode.h>
#include <Editor/Menus/SceneContextMenuActions.h>

namespace LandscapeCanvasEditor
{
    FindSelectedNodesAction::FindSelectedNodesAction(QObject* parent)
        : GraphCanvas::ContextMenuAction("", parent)
    {
        UpdateActionState();

        QString tooltip = QObject::tr("Select the corresponding node(s) in the graph based on the Vegetation Entities that are selected in the Editor");
        setToolTip(tooltip);
        setStatusTip(tooltip);
    }

    GraphCanvas::ActionGroupId FindSelectedNodesAction::GetActionGroupId() const
    {
        return AZ_CRC_CE("SceneActionGroup");
    }

    void FindSelectedNodesAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetId);

        UpdateActionState();
    }

    GraphCanvas::ContextMenuAction::SceneReaction FindSelectedNodesAction::TriggerAction(const GraphCanvas::GraphId& graphId, [[maybe_unused]] const AZ::Vector2& scenePos)
    {
        // Find the selected Entities in the Editor
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        // Retrieve all the nodes in our scene
        JE::Editor::Tools::GraphModel::NodePtrList nodeList;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeList, graphId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodes);

        // Find the nodes in our scene that correspond to the Entities
        JE::Editor::Tools::GraphModel::NodePtrList nodesToSelect;
        for (const auto& node : nodeList)
        {
            auto baseNodePtr = static_cast<LandscapeCanvas::BaseNode*>(node.get());
            auto it = AZStd::find(selectedEntities.begin(), selectedEntities.end(), baseNodePtr->GetVegetationEntityId());
            if (it != selectedEntities.end())
            {
                nodesToSelect.push_back(node);
            }
        }

        if (nodesToSelect.empty())
        {
            QString warningMessage = selectedEntities.size() > 1 ? QObject::tr("The selected Entities are not present in the graph") : QObject::tr("The selected Entity is not present in the graph");
            AZ_Warning("LandscapeCanvas", false, warningMessage.toUtf8().constData());
        }
        else
        {
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::Event(graphId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::ClearSelection);
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::Event(graphId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::SetSelected, nodesToSelect, true);
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::Event(graphId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::CenterOnNodes, nodesToSelect);
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }

    void FindSelectedNodesAction::UpdateActionState()
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        setText(selectedEntities.size() > 1 ? QObject::tr("Find Selected Entities in Graph") : QObject::tr("Find Selected Entity in Graph"));

        setEnabled(!selectedEntities.empty());
    }
}
