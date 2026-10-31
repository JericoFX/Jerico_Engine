/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QVBoxLayout>

#include <AzCore/UserSettings/UserSettings.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeCreateUtils.h>
#include <Editor/Nodes/NodeUtils.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>
#include <Editor/View/Widgets/VariablePanel/VariableConfigurationWidget.h>

#include <JE/Editor/Tools/GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/NodeDescriptors/FunctionDefinitionNodeDescriptorComponent.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/ViewBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/VisualBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/EditorTypes.h>
#include <JE/Editor/Tools/GraphCanvas/GraphCanvasBus.h>
#include <JE/Editor/Tools/GraphCanvas/Types/Endpoint.h>
#include <JE/Editor/Tools/GraphCanvas/Utils/ConversionUtils.h>
#include <JE/Editor/Tools/GraphCanvas/Utils/NodeNudgingController.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/GraphSerialization.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Grammar/ParsingUtilitiesScriptEventExtension.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

#include <ScriptEvents/ScriptEventsBus.h>

#include "ScriptCanvasContextMenus.h"
#include "Settings.h"

namespace ScriptCanvasEditor
{
    ////////////////////////////
    // EndpointSelectionAction
    ////////////////////////////

    EndpointSelectionAction::EndpointSelectionAction(const JE::Editor::Tools::GraphCanvas::Endpoint& proposedEndpoint)
        : QAction(nullptr)
        , m_endpoint(proposedEndpoint)
    {
        AZStd::string name;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(name, proposedEndpoint.GetSlotId(), &JE::Editor::Tools::GraphCanvas::SlotRequests::GetName);

        AZStd::string tooltip;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(tooltip, proposedEndpoint.GetSlotId(), &JE::Editor::Tools::GraphCanvas::SlotRequests::GetTooltip);

        setText(name.c_str());
        setToolTip(tooltip.c_str());
    }

    const JE::Editor::Tools::GraphCanvas::Endpoint& EndpointSelectionAction::GetEndpoint() const
    {
        return m_endpoint;
    }

    ////////////////////////////////////
    // RemoveUnusedVariablesMenuAction
    ////////////////////////////////////

    RemoveUnusedVariablesMenuAction::RemoveUnusedVariablesMenuAction(QObject* parent)
        : SceneContextMenuAction("Variables", parent)
    {
        setToolTip("Removes all of the unused variables from the active graph");
    }

    void RemoveUnusedVariablesMenuAction::RefreshAction([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::GraphId& graphId, [[maybe_unused]] const AZ::EntityId& targetId)
    {
        setEnabled(true);
    }

    bool RemoveUnusedVariablesMenuAction::IsInSubMenu() const
    {
        return true;
    }

    AZStd::string RemoveUnusedVariablesMenuAction::GetSubMenuPath() const
    {
        return "Remove Unused";
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction RemoveUnusedVariablesMenuAction::TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, [[maybe_unused]] const AZ::Vector2& scenePos)
    {
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::RemoveUnusedNodes);
        return SceneReaction::PostUndo;
    }

    ///////////////////////////////
    // SlotManipulationMenuAction
    ///////////////////////////////

    SlotManipulationMenuAction::SlotManipulationMenuAction(AZStd::string_view actionName, QObject* parent)
        : JE::Editor::Tools::GraphCanvas::ContextMenuAction(actionName, parent)
    {
    }


    ScriptCanvas::Slot* SlotManipulationMenuAction::GetScriptCanvasSlot(const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint) 
    {
        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, endpoint.GetNodeId(), &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        ScriptCanvas::Endpoint scriptCanvasEndpoint;

        {
            AZStd::any* userData = nullptr;

            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(userData, endpoint.GetSlotId(), &JE::Editor::Tools::GraphCanvas::SlotRequests::GetUserData);
            ScriptCanvas::SlotId scriptCanvasSlotId = (userData && userData->is<ScriptCanvas::SlotId>()) ? *AZStd::any_cast<ScriptCanvas::SlotId>(userData) : ScriptCanvas::SlotId();

            userData = nullptr;

            JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(userData, endpoint.GetNodeId(), &JE::Editor::Tools::GraphCanvas::NodeRequests::GetUserData);
            AZ::EntityId scriptCanvasNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

            scriptCanvasEndpoint = ScriptCanvas::Endpoint(scriptCanvasNodeId, scriptCanvasSlotId);
        }

        ScriptCanvas::Slot* scriptCanvasSlot = nullptr;

        ScriptCanvas::GraphRequestBus::EventResult(scriptCanvasSlot, scriptCanvasId, &ScriptCanvas::GraphRequests::FindSlot, scriptCanvasEndpoint);

        return scriptCanvasSlot;
    }

    /////////////////////////////////////////
    // ConvertVariableNodeToReferenceAction
    /////////////////////////////////////////

    ConvertVariableNodeToReferenceAction::ConvertVariableNodeToReferenceAction(QObject* parent)
        : JE::Editor::Tools::GraphCanvas::ContextMenuAction("Convert to References", parent)
    {
    }

    JE::Editor::Tools::GraphCanvas::ActionGroupId ConvertVariableNodeToReferenceAction::GetActionGroupId() const
    {
        return AZ_CRC_CE("VariableConversion");
    }

    void ConvertVariableNodeToReferenceAction::RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        bool hasMultipleSelection = false;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(hasMultipleSelection, graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::HasMultipleSelection);        

        m_targetId = targetId;

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        bool canConvertNode = false;
        EditorGraphRequestBus::EventResult(canConvertNode, scriptCanvasId, &EditorGraphRequests::CanConvertVariableNodeToReference, m_targetId);

        // This item is added only when it's valid
        setEnabled(canConvertNode && !hasMultipleSelection);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ConvertVariableNodeToReferenceAction::TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, [[maybe_unused]] const AZ::Vector2& scenePos)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        bool convertedNode = false;
        EditorGraphRequestBus::EventResult(convertedNode, scriptCanvasId, &EditorGraphRequests::ConvertVariableNodeToReference, m_targetId);

        if (convertedNode)
        {
            return SceneReaction::PostUndo;
        }

        return SceneReaction::Nothing;
    }

    /////////////////////////////////////////
    // ConvertReferenceToVariableNodeAction
    /////////////////////////////////////////

    ConvertReferenceToVariableNodeAction::ConvertReferenceToVariableNodeAction(QObject* parent)
        : SlotManipulationMenuAction("Convert to Variable Node", parent)
    {
    }

    JE::Editor::Tools::GraphCanvas::ActionGroupId ConvertReferenceToVariableNodeAction::GetActionGroupId() const
    {
        return AZ_CRC_CE("VariableConversion");
    }


    void ConvertReferenceToVariableNodeAction::RefreshAction([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_targetId = targetId;

        bool enableAction = false;

        if (JE::Editor::Tools::GraphCanvas::GraphUtils::IsSlot(m_targetId))
        {
            JE::Editor::Tools::GraphCanvas::SlotType slotType = JE::Editor::Tools::GraphCanvas::SlotTypes::Invalid;
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(slotType, m_targetId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetSlotType);

            if (slotType == JE::Editor::Tools::GraphCanvas::SlotTypes::DataSlot)
            {
                JE::Editor::Tools::GraphCanvas::DataSlotRequestBus::EventResult(enableAction, m_targetId, &JE::Editor::Tools::GraphCanvas::DataSlotRequests::CanConvertToValue);

                if (enableAction)
                {
                    JE::Editor::Tools::GraphCanvas::DataSlotType valueType = JE::Editor::Tools::GraphCanvas::DataSlotType::Unknown;
                    JE::Editor::Tools::GraphCanvas::DataSlotRequestBus::EventResult(valueType, m_targetId, &JE::Editor::Tools::GraphCanvas::DataSlotRequests::GetDataSlotType);

                    enableAction = (valueType == JE::Editor::Tools::GraphCanvas::DataSlotType::Reference);

                    if (enableAction)
                    {
                        JE::Editor::Tools::GraphCanvas::Endpoint endpoint;
                        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(endpoint, m_targetId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);

                        ScriptCanvas::Slot* slot = GetScriptCanvasSlot(endpoint);

                        enableAction = slot->GetVariableReference().IsValid();
                    }
                }
            }
        }

        setEnabled(enableAction);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ConvertReferenceToVariableNodeAction::TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        JE::Editor::Tools::GraphCanvas::Endpoint endpoint;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(endpoint, m_targetId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);

        JE::Editor::Tools::GraphCanvas::ConnectionType connectionType = JE::Editor::Tools::GraphCanvas::CT_Invalid;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(connectionType, m_targetId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetConnectionType);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        ScriptCanvas::Slot* scriptCanvasSlot = GetScriptCanvasSlot(endpoint);

        if (scriptCanvasSlot == nullptr
            || !scriptCanvasSlot->IsVariableReference())
        {
            return SceneReaction::Nothing;
        }

        // Store the variable then convert the slot to a value for the next step.
        ScriptCanvas::VariableId variableId = scriptCanvasSlot->GetVariableReference();
        JE::Editor::Tools::GraphCanvas::DataSlotRequestBus::Event(m_targetId, &JE::Editor::Tools::GraphCanvas::DataSlotRequests::ConvertToValue);

        AZ::EntityId createdNodeId;

        if (connectionType == JE::Editor::Tools::GraphCanvas::CT_Input)
        {
            CreateGetVariableNodeMimeEvent createMimeEvent(variableId);

            createdNodeId = createMimeEvent.CreateSplicingNode(graphId);
        }
        else if (connectionType == JE::Editor::Tools::GraphCanvas::CT_Output)
        {
            CreateSetVariableNodeMimeEvent createMimeEvent(variableId);

            createdNodeId = createMimeEvent.CreateSplicingNode(graphId);
        }

        if (!createdNodeId.IsValid())
        {
            return SceneReaction::Nothing;
        }

        JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::AddNode, createdNodeId, scenePos, false);

        JE::Editor::Tools::GraphCanvas::CreateConnectionsBetweenConfig createConnectionBetweenConfig;

        createConnectionBetweenConfig.m_connectionType = JE::Editor::Tools::GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SingleConnection;
        createConnectionBetweenConfig.m_createModelConnections = true;

        JE::Editor::Tools::GraphCanvas::GraphUtils::CreateConnectionsBetween({ endpoint }, createdNodeId, createConnectionBetweenConfig);

        if (!createConnectionBetweenConfig.m_createdConnections.empty())
        {
            JE::Editor::Tools::GraphCanvas::Endpoint otherEndpoint;
            JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, (*createConnectionBetweenConfig.m_createdConnections.begin()), &JE::Editor::Tools::GraphCanvas::ConnectionRequests::FindOtherEndpoint, endpoint);

            if (otherEndpoint.IsValid())
            {
                JE::Editor::Tools::GraphCanvas::GraphUtils::AlignSlotForConnection(otherEndpoint, endpoint);
            }
        }

        AZStd::vector<AZ::EntityId> slotIds;

        JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(slotIds, endpoint.GetNodeId(), &JE::Editor::Tools::GraphCanvas::NodeRequests::GetSlotIds);        

        JE::Editor::Tools::GraphCanvas::ConnectionSpliceConfig spliceConfig;
        spliceConfig.m_allowOpportunisticConnections = false;

        bool connectedExecution = false;
        AZStd::vector< JE::Editor::Tools::GraphCanvas::Endpoint > validInputSlots;

        for (auto slotId : slotIds)
        {
            JE::Editor::Tools::GraphCanvas::SlotRequests* slotRequests = JE::Editor::Tools::GraphCanvas::SlotRequestBus::FindFirstHandler(slotId);

            if (slotRequests == nullptr)
            {
                continue;
            }

            JE::Editor::Tools::GraphCanvas::SlotType slotType = slotRequests->GetSlotType();

            if (slotType == JE::Editor::Tools::GraphCanvas::SlotTypes::ExecutionSlot)
            {
                JE::Editor::Tools::GraphCanvas::ConnectionType testConnectionType = slotRequests->GetConnectionType();

                // We only want to connect to things going in the same direction as we are.
                if (testConnectionType == connectionType)
                {
                    validInputSlots.emplace_back(endpoint.GetNodeId(), slotId);

                    AZStd::vector< AZ::EntityId > connectionIds = slotRequests->GetConnections();

                    for (const AZ::EntityId& connectionId : connectionIds)
                    {
                        if (JE::Editor::Tools::GraphCanvas::GraphUtils::SpliceNodeOntoConnection(createdNodeId, connectionId, spliceConfig))
                        {
                            connectedExecution = true;
                        }
                    }
                }
            }

            if (!connectedExecution)
            {
                JE::Editor::Tools::GraphCanvas::CreateConnectionsBetweenConfig fallbackConnectionConfig;

                fallbackConnectionConfig.m_connectionType = JE::Editor::Tools::GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SinglePass;
                fallbackConnectionConfig.m_createModelConnections = true;

                JE::Editor::Tools::GraphCanvas::GraphUtils::CreateConnectionsBetween(validInputSlots, createdNodeId, fallbackConnectionConfig);
            }
        }

        JE::Editor::Tools::GraphCanvas::NodeNudgingController nudgingController;

        nudgingController.SetGraphId(graphId);
        nudgingController.StartNudging({ createdNodeId });
        nudgingController.FinalizeNudging();

        JE::Editor::Tools::GraphCanvas::AnimatedPulseConfiguration animatedPulseConfig;

        animatedPulseConfig.m_enableGradient = true;
        animatedPulseConfig.m_drawColor = QColor(255, 255, 255);
        animatedPulseConfig.m_durationSec = 0.25f;

        JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::CreatePulseAroundSceneMember, createdNodeId, 4, animatedPulseConfig);

        return SceneReaction::PostUndo;
    }

    //////////////////////////////////
    // ExposeExecutionSlotMenuAction
    //////////////////////////////////

    ExposeSlotMenuAction::ExposeSlotMenuAction(QObject* parent)
        : JE::Editor::Tools::GraphCanvas::SlotContextMenuAction("Expose", parent)
    {

    }

    void ExposeSlotMenuAction::RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(nodeId, targetId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetNode);

        bool canExposeSlot = false;
        EditorGraphRequestBus::EventResult(canExposeSlot, scriptCanvasId, &EditorGraphRequests::CanExposeEndpoint, JE::Editor::Tools::GraphCanvas::Endpoint(nodeId, targetId ));        

        setEnabled(canExposeSlot);
    }

    void ExposeSlotMenuAction::CreateNodeling(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, AZ::EntityId scriptCanvasGraphId, JE::Editor::Tools::GraphCanvas::GraphId slotId, const AZ::Vector2& scenePos, JE::Editor::Tools::GraphCanvas::ConnectionType connectionType)
    {
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(nodeId, slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetNode);

        // Set the connection type for the node opposite of what it actually is because we're interested in the connection type of the node we're 
        // exposing, not the type of the slot we just created
        ScriptCanvas::ConnectionType scriptCanvasConnectionType = (connectionType == JE::Editor::Tools::GraphCanvas::CT_Input) ? ScriptCanvas::ConnectionType::Input : ScriptCanvas::ConnectionType::Output;
        bool isInput = (scriptCanvasConnectionType == ScriptCanvas::ConnectionType::Output);

        NodeIdPair nodePair = ScriptCanvasEditor::Nodes::CreateFunctionDefinitionNode(scriptCanvasGraphId, isInput);

        JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, scenePos, false);

        JE::Editor::Tools::GraphCanvas::Endpoint graphCanvasEndpoint;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(graphCanvasEndpoint, slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);

        // Find the execution "nodeling"
        ScriptCanvas::Nodes::Core::FunctionDefinitionNode* nodeling = ScriptCanvasEditor::Nodes::GetNode<ScriptCanvas::Nodes::Core::FunctionDefinitionNode>(scriptCanvasGraphId, nodePair);

        // Configure the Execution node
        AZStd::string nodeTitle;
        JE::Editor::Tools::GraphCanvas::NodeTitleRequestBus::EventResult(nodeTitle, nodeId, &JE::Editor::Tools::GraphCanvas::NodeTitleRequests::GetTitle);

        AZStd::string name;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(name, slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetName);

        AZStd::string fullTitle = AZStd::string::format("%s : %s", nodeTitle.c_str(), name.c_str());

        nodeling->SetDisplayName(fullTitle);

        // Set the node title, subtitle, tooltip
        JE::Editor::Tools::GraphCanvas::NodeTitleRequestBus::Event(nodePair.m_graphCanvasId, &JE::Editor::Tools::GraphCanvas::NodeTitleRequests::SetTitle, fullTitle);
        JE::Editor::Tools::GraphCanvas::NodeRequestBus::Event(nodePair.m_graphCanvasId, &JE::Editor::Tools::GraphCanvas::NodeRequests::SetTooltip, name);

        ScriptCanvas::SlotDescriptor descriptor;
        descriptor.m_slotType = ScriptCanvas::SlotTypeDescriptor::Execution;
        descriptor.m_connectionType = scriptCanvasConnectionType;

        auto descriptorSlots = nodeling->GetAllSlotsByDescriptor(descriptor);

        // There should only be a single slot
        AZ_Assert(descriptorSlots.size() == 1, "Nodeling should only create one of each execution slot type.");

        const ScriptCanvas::Slot* slot = descriptorSlots.front();

        JE::Editor::Tools::GraphCanvas::SlotId graphCanvasSlotId;
        SlotMappingRequestBus::EventResult(graphCanvasSlotId, nodePair.m_graphCanvasId, &SlotMappingRequests::MapToGraphCanvasId, slot->GetId());

        JE::Editor::Tools::GraphCanvas::Endpoint fixedEndpoint(nodeId, slotId);

        // Automatically connect to the slot that was exposed
        AZ::EntityId connectionId;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(connectionId, graphCanvasSlotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, fixedEndpoint);

        if (connectionId.IsValid())
        {
            JE::Editor::Tools::GraphCanvas::Endpoint executionEndpoint(nodePair.m_graphCanvasId, graphCanvasSlotId);

            JE::Editor::Tools::GraphCanvas::GraphUtils::AlignSlotForConnection(executionEndpoint, fixedEndpoint);
        }
        else
        {
            AZStd::unordered_set<AZ::EntityId> deletionSet = { nodePair.m_graphCanvasId };
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::Delete, deletionSet);
        }
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction ExposeSlotMenuAction::TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        // Go to Execution node and allow it to be renamed
        // Make sure this stuff is restored on serialization

        ScriptCanvas::ScriptCanvasId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasId, graphId);

        const JE::Editor::Tools::GraphCanvas::SlotId& slotId = GetTargetId();

        JE::Editor::Tools::GraphCanvas::ConnectionType connectionType;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetConnectionType);

        JE::Editor::Tools::GraphCanvas::SlotType slotType;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetSlotType);

        // Will create an Execution node and connect it to this slot, nothing if the slot is already connected (the option shouldn't show)
        if (slotType == JE::Editor::Tools::GraphCanvas::SlotTypes::ExecutionSlot && connectionType == JE::Editor::Tools::GraphCanvas::CT_Input)
        {
            AZ::Vector2 spawnPosition = scenePos + AZ::Vector2(-200, 0);
            CreateNodeling(graphId, scriptCanvasGraphId, slotId, spawnPosition, JE::Editor::Tools::GraphCanvas::CT_Output);
        }
        else if (slotType == JE::Editor::Tools::GraphCanvas::SlotTypes::ExecutionSlot && connectionType == JE::Editor::Tools::GraphCanvas::CT_Output)
        {
            AZ::Vector2 spawnPosition = scenePos + AZ::Vector2(200, 0);
            CreateNodeling(graphId, scriptCanvasGraphId, slotId, spawnPosition, JE::Editor::Tools::GraphCanvas::CT_Input);
        }
        else if (slotType == JE::Editor::Tools::GraphCanvas::SlotTypes::DataSlot)
        {
            const AZ::EntityId& slotId2 = GetTargetId();
            const JE::Editor::Tools::GraphCanvas::GraphId& graphId2 = GetGraphId();

            JE::Editor::Tools::GraphCanvas::Endpoint endpoint;
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(endpoint, slotId2, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);

            bool promotedElement = false;
            JE::Editor::Tools::GraphCanvas::GraphModelRequestBus::EventResult(promotedElement, graphId2, &JE::Editor::Tools::GraphCanvas::GraphModelRequests::PromoteToVariableAction, endpoint, false);

            if (promotedElement)
            {
                ScriptCanvas::Endpoint scEndpoint;
                EditorGraphRequestBus::EventResult(scEndpoint, scriptCanvasGraphId, &EditorGraphRequests::ConvertToScriptCanvasEndpoint, endpoint);

                if (scEndpoint.IsValid())
                {
                    ScriptCanvas::Slot* slot = nullptr;
                    ScriptCanvas::GraphRequestBus::EventResult(slot, scriptCanvasGraphId, &ScriptCanvas::GraphRequests::FindSlot, scEndpoint);

                    if (slot && slot->IsVariableReference())
                    {
                        ScriptCanvas::GraphVariable* variable = slot->GetVariable();

                        if (variable)
                        {
                            variable->SetScope(ScriptCanvas::VariableFlags::Scope::Function);
                        }
                    }
                }
            }
        }

        return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
    }

    //////////////////////////////////
    // SetDataSlotTypeMenuAction
    //////////////////////////////////

    SetDataSlotTypeMenuAction::SetDataSlotTypeMenuAction(QObject* parent)
        : JE::Editor::Tools::GraphCanvas::SlotContextMenuAction("Set Slot Type", parent)
    {
    }

    void SetDataSlotTypeMenuAction::RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        ScriptCanvas::Slot* slot = GetSlot(graphId, targetId);

        bool isEnabled = slot && slot->IsUserAdded() && slot->GetDescriptor().IsData();

        setEnabled(isEnabled);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction SetDataSlotTypeMenuAction::TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        ScriptCanvas::Slot* slot = GetSlot(graphId, GetTargetId());
        if (!slot)
        {
            return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
        }

        auto variable = slot->GetVariable();
        if (!variable)
        {
            return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
        }

        ScriptCanvas::ScriptCanvasId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasId, graphId);
        if (!scriptCanvasGraphId.IsValid())
        {
            return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
        }

        VariablePaletteRequests::VariableConfigurationInput selectedSlotSetup;
        selectedSlotSetup.m_changeVariableType = true;
        selectedSlotSetup.m_graphVariable = variable;
        selectedSlotSetup.m_currentName = slot->GetName();
        selectedSlotSetup.m_currentType = slot->GetDataType();

        QPoint scenePoint(static_cast<int>(scenePos.GetX()), static_cast<int>(scenePos.GetY()));
        VariablePaletteRequests::VariableConfigurationOutput output;
        VariablePaletteRequestBus::BroadcastResult(output, &VariablePaletteRequests::ShowVariableConfigurationWidget
            , selectedSlotSetup, scenePoint);

        bool changed = false;

        if (output.m_actionIsValid)
        {
            if ((output.m_nameChanged && !output.m_name.empty()) || (output.m_typeChanged && output.m_type.IsValid()))
            {
                GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, scriptCanvasGraphId);
                JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBlocker undoBlocker(graphId);

                if ((output.m_nameChanged && !output.m_name.empty()))
                {
                    variable->SetVariableName(output.m_name);
                }

                if (output.m_typeChanged && output.m_type.IsValid())
                {
                    variable->ModDatum().SetType(output.m_type, ScriptCanvas::Datum::TypeChange::Forced);
                    ScriptCanvas::GraphRequestBus::Event(scriptCanvasGraphId, &ScriptCanvas::GraphRequests::RefreshVariableReferences
                        , variable->GetVariableId());
                }

                changed = true;
            }
        }

        return changed ? JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::PostUndo : JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }
    
    bool SetDataSlotTypeMenuAction::IsSupportedSlotType(const AZ::EntityId& slotId)
    {
        JE::Editor::Tools::GraphCanvas::Endpoint endpoint;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(endpoint, slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);

        const ScriptCanvas::Slot* slot = SlotManipulationMenuAction::GetScriptCanvasSlot(endpoint);
        if (slot)
        {
            if (slot->GetDescriptor().IsData())
            {
                return true;
            }
        }

        return false;
    }

    ScriptCanvas::Slot* SetDataSlotTypeMenuAction::GetSlot(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasId, graphId);

        JE::Editor::Tools::GraphCanvas::Endpoint endpoint;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(endpoint, targetId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);

        ScriptCanvas::Endpoint scEndpoint;
        EditorGraphRequestBus::EventResult(scEndpoint, scriptCanvasGraphId, &EditorGraphRequests::ConvertToScriptCanvasEndpoint, endpoint);

        ScriptCanvas::Slot* slot = nullptr;
        ScriptCanvas::NodeRequestBus::EventResult(slot, scEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, scEndpoint.GetSlotId());

        return slot;

    }

    //////////////////////////////////
    // CreateAzEventHandlerSlotMenuAction
    //////////////////////////////////

    CreateAzEventHandlerSlotMenuAction::CreateAzEventHandlerSlotMenuAction(QObject* parent)
        : SlotContextMenuAction("Create event handler", parent)
    {
    }

    void CreateAzEventHandlerSlotMenuAction::RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_methodWithAzEventReturn = FindBehaviorMethodWithAzEventReturn(graphId, targetId);
        if (m_methodWithAzEventReturn)
        {
            // Store the GraphCanvas endpoint corresponding to the supplied slot
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::Event(targetId, [this](JE::Editor::Tools::GraphCanvas::SlotRequests* slotRequests)
            {
                m_methodNodeAzEventEndpoint = slotRequests->GetEndpoint();
            });
            setEnabled(true);
            return;
        }

        setEnabled(false);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction CreateAzEventHandlerSlotMenuAction::TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        if (m_methodWithAzEventReturn)
        {
            ScriptCanvas::ScriptCanvasId scriptCanvasGraphId;
            GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasId, graphId);

            // Retrieve the MethodNodeScriptCanvasId and pass that in to the CreateAzEventHandlerNode function to enforce
            // the restricted node contract
            AZ::EntityId methodNodeScriptCanvasId;
            auto GetScriptCanvasNodeId = [&methodNodeScriptCanvasId](JE::Editor::Tools::GraphCanvas::NodeRequests* nodeRequests)
            {
                if (auto scNodeId = AZStd::any_cast<AZ::EntityId>(nodeRequests->GetUserData());
                    scNodeId != nullptr)
                {
                    methodNodeScriptCanvasId = *scNodeId;
                }
            };
            JE::Editor::Tools::GraphCanvas::NodeRequestBus::Event(m_methodNodeAzEventEndpoint.GetNodeId(), GetScriptCanvasNodeId);
            NodeIdPair nodePair = Nodes::CreateAzEventHandlerNode(*m_methodWithAzEventReturn, scriptCanvasGraphId,
                methodNodeScriptCanvasId);

            if (nodePair.m_graphCanvasId.IsValid())
            {
                // Add newly created GraphCanvas node to the GraphCanvas scene
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, scenePos, false);

                // Connect the AZ::Event<Params...> data output from the MethodNode to newly created AzEventHandler node data input slot of the same type
                JE::Editor::Tools::GraphCanvas::CreateConnectionsBetweenConfig createConnectionBetweenConfig;
                createConnectionBetweenConfig.m_connectionType = JE::Editor::Tools::GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SingleConnection;
                createConnectionBetweenConfig.m_createModelConnections = true;
                JE::Editor::Tools::GraphCanvas::GraphUtils::CreateConnectionsBetween({ m_methodNodeAzEventEndpoint }, nodePair.m_graphCanvasId, createConnectionBetweenConfig);

                if (!createConnectionBetweenConfig.m_createdConnections.empty())
                {
                    JE::Editor::Tools::GraphCanvas::Endpoint otherEndpoint;
                    JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, *createConnectionBetweenConfig.m_createdConnections.begin(),
                        &JE::Editor::Tools::GraphCanvas::ConnectionRequests::FindOtherEndpoint, m_methodNodeAzEventEndpoint);

                    if (otherEndpoint.IsValid())
                    {
                        // This will connect the execution output slot from the MethodNode to the Connect input slot
                        // on our newly created AzEventHandler node
                        auto opportunisticConnections = JE::Editor::Tools::GraphCanvas::GraphUtils::CreateOpportunisticConnectionsBetween(m_methodNodeAzEventEndpoint, otherEndpoint);
                        createConnectionBetweenConfig.m_createdConnections.insert(opportunisticConnections.begin(), opportunisticConnections.end());

                        // Update the AzEventHandler node position to not overlap over the MethodNode
                        JE::Editor::Tools::GraphCanvas::GraphUtils::AlignSlotForConnection(otherEndpoint, m_methodNodeAzEventEndpoint);
                    }

                    // Disable the flags on the connections we created with the AZ::Event Handler node (e.g. selectable/movable)
                    // so that the user can't delete them
                    for (auto connectionId : createConnectionBetweenConfig.m_createdConnections)
                    {
                        JE::Editor::Tools::GraphCanvas::ConnectionUIRequestBus::Event(connectionId, &JE::Editor::Tools::GraphCanvas::ConnectionUIRequests::SetGraphicsItemFlags, QGraphicsItem::GraphicsItemFlags());
                    }
                }

                return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
            }
        }

        return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }

    const AZ::BehaviorMethod* CreateAzEventHandlerSlotMenuAction::FindBehaviorMethodWithAzEventReturn(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, AZ::EntityId targetId)
    {
        const AZ::BehaviorMethod* methodWithAzEventReturn{};

        if (JE::Editor::Tools::GraphCanvas::GraphUtils::IsSlot(targetId))
        {
            // Extract the GraphCanvas Slot Type and complete endpoint from using the targetId
            JE::Editor::Tools::GraphCanvas::SlotType slotType = JE::Editor::Tools::GraphCanvas::SlotTypes::Invalid;
            JE::Editor::Tools::GraphCanvas::Endpoint endpoint;
            auto GetGraphCanvasEndpoint = [&slotType, &endpoint](JE::Editor::Tools::GraphCanvas::SlotRequests* slotRequests)
            {
                slotType = slotRequests->GetSlotType();
                endpoint = slotRequests->GetEndpoint();
            };
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::Event(targetId, GetGraphCanvasEndpoint);

            // A slot that exposes the context menu to create an AZ::Event must be a data slot
            if (slotType != JE::Editor::Tools::GraphCanvas::SlotTypes::DataSlot)
            {
                return nullptr;
            }

            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

            ScriptCanvas::SlotId scriptCanvasSlotId;
            auto GetScriptCanvasSlotId = [&scriptCanvasSlotId](JE::Editor::Tools::GraphCanvas::SlotRequests* slotRequests)
            {
                if (auto scSlotId = AZStd::any_cast<ScriptCanvas::SlotId>(slotRequests->GetUserData());
                    scSlotId != nullptr)
                {
                    scriptCanvasSlotId = *scSlotId;
                }
            };
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::Event(endpoint.GetSlotId(), GetScriptCanvasSlotId);

            AZ::EntityId scriptCanvasNodeId;
            auto GetScriptCanvasNodeId = [&scriptCanvasNodeId](JE::Editor::Tools::GraphCanvas::NodeRequests* nodeRequests)
            {
                if (auto scNodeId = AZStd::any_cast<AZ::EntityId>(nodeRequests->GetUserData());
                    scNodeId != nullptr)
                {
                    scriptCanvasNodeId = *scNodeId;
                }
            };
            JE::Editor::Tools::GraphCanvas::NodeRequestBus::Event(endpoint.GetNodeId(), GetScriptCanvasNodeId);

            const AZ::BehaviorMethod* candidateMethod{};
            auto GetMethodFromNode = [&candidateMethod, &scriptCanvasNodeId, scriptCanvasSlotId](ScriptCanvas::GraphRequests* graphRequests)
            {
                ScriptCanvas::Slot* scriptCanvasSlot = graphRequests->FindSlot(ScriptCanvas::Endpoint{ scriptCanvasNodeId, scriptCanvasSlotId });
                ScriptCanvas::Node* scriptCanvasNode = scriptCanvasSlot ? scriptCanvasSlot->GetNode() : nullptr;
                if (auto methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(scriptCanvasNode); methodNode != nullptr)
                {
                    candidateMethod = methodNode->GetMethod();
                }
            };
            ScriptCanvas::GraphRequestBus::Event(scriptCanvasId, GetMethodFromNode);

            if (candidateMethod)
            {
                auto MethodReturnsValidAzEvent = [&methodWithAzEventReturn, &candidateMethod](AZ::ComponentApplicationRequests* requests)
                {
                    if (AZ::BehaviorContext* behaviorContext = requests->GetBehaviorContext(); behaviorContext != nullptr)
                    {
                        if (AZ::ValidateAzEventDescription(*behaviorContext, *candidateMethod))
                        {
                            methodWithAzEventReturn = candidateMethod;
                        }
                    }
                };
                AZ::ComponentApplicationBus::Broadcast(MethodReturnsValidAzEvent);
            }
        }

        return methodWithAzEventReturn;
    }

    /////////////////////
    // SceneContextMenu
    /////////////////////

    SceneContextMenu::SceneContextMenu(const NodePaletteModel& paletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
        : JE::Editor::Tools::GraphCanvas::SceneContextMenu(ScriptCanvasEditor::AssetEditorId)
    {

        auto userSettings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC_CE("ScriptCanvasPreviewSettings"), AZ::UserSettings::CT_LOCAL);
        if (userSettings)
        {
            m_userNodePaletteWidth = userSettings->m_sceneContextMenuNodePaletteWidth;
        }

        const bool inContextMenu = true;
        Widget::ScriptCanvasNodePaletteConfig paletteConfig(paletteModel, assetModel, inContextMenu);
        AddNodePaletteMenuAction(paletteConfig);
    }

    void SceneContextMenu::ResetSourceSlotFilter()
    {
        m_nodePalette->ResetSourceSlotFilter();
    }

    void SceneContextMenu::FilterForSourceSlot(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& sourceSlotId)
    {
        m_nodePalette->FilterForSourceSlot(scriptCanvasGraphId, sourceSlotId);
    }

    void SceneContextMenu::OnRefreshActions(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        JE::Editor::Tools::GraphCanvas::SceneContextMenu::OnRefreshActions(graphId, targetMemberId);

        // Don't want to overly manipulate the state. So we only modify this when we know we want to turn it on.
        if (GraphVariablesTableView::HasCopyVariableData())
        {
            m_editorActionsGroup.SetPasteEnabled(true);
        }
    }

    void SceneContextMenu::SetupDisplayForProposal()
    {
        // Disabling all of the actions here for the proposal.
        // Allows a certain 'visual consistency' in using the same menu while
        // not providing any unusable options.
        m_editorActionsGroup.SetCutEnabled(false);
        m_editorActionsGroup.SetCopyEnabled(false);
        m_editorActionsGroup.SetPasteEnabled(false);
        m_editorActionsGroup.SetDeleteEnabled(false);
        m_editorActionsGroup.SetDuplicateEnabled(false);

        m_graphCanvasConstructGroups.SetAddBookmarkEnabled(false);
        m_graphCanvasConstructGroups.SetCommentsEnabled(false);
        m_nodeGroupPresets.SetEnabled(false);
        m_alignmentActionsGroups.SetEnabled(false);
    }

    //////////////////////////
    // ConnectionContextMenu
    //////////////////////////

    ConnectionContextMenu::ConnectionContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
        : JE::Editor::Tools::GraphCanvas::ConnectionContextMenu(ScriptCanvasEditor::AssetEditorId)
    {
        const bool inContextMenu = true;
        Widget::ScriptCanvasNodePaletteConfig paletteConfig(nodePaletteModel, assetModel, inContextMenu);

        AddNodePaletteMenuAction(paletteConfig);
    }

    void ConnectionContextMenu::OnRefreshActions(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        JE::Editor::Tools::GraphCanvas::ConnectionContextMenu::OnRefreshActions(graphId, targetMemberId);

        m_connectionId = targetMemberId;
        
        // TODO: Filter nodes.
    }

    //////////////////////////
    // RenameFunctionDefinitionNodeAction
    //////////////////////////

    RenameFunctionDefinitionNodeAction::RenameFunctionDefinitionNodeAction(NodeDescriptorComponent* descriptor, QObject* parent)
        : NodeContextMenuAction("Rename Function", parent)
        , m_descriptor(descriptor)
    {}

    void RenameFunctionDefinitionNodeAction::RefreshAction(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& /*targetId*/)
    {
        AZStd::vector<AZ::EntityId> selectedNodes;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(selectedNodes, graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetSelectedNodes);

        setEnabled(selectedNodes.size() == 1);
    }

    JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction RenameFunctionDefinitionNodeAction::TriggerAction(const JE::Editor::Tools::GraphCanvas::GraphId&, const AZ::Vector2&)
    {
        if (FunctionDefinitionNodeDescriptorComponent::RenameDialog(azrtti_cast<FunctionDefinitionNodeDescriptorComponent*>(m_descriptor)))
        {
            return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
        }

        return JE::Editor::Tools::GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }
}

