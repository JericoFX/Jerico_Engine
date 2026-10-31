/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/ViewBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/VisualBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/AssetEditorBus.h>

#include <JE/Editor/Tools/GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/ConnectionStates.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ConnectionActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

namespace ScriptCanvas::Developer
{ 
    /////////////////////
    // CoupleNodesState
    /////////////////////

    CoupleNodesState::CoupleNodesState(AutomationStateModelId pickUpNode, JE::Editor::Tools::GraphCanvas::ConnectionType connectionType, AutomationStateModelId targetNode, AutomationStateModelId outputId)
        : NamedAutomationState("CoupleNodesState")
        , m_pickUpNode(pickUpNode)
        , m_targetNode(targetNode)
        , m_connectionType(connectionType)
        , m_outputId(outputId)
    {
        SetStateName(AZStd::string::format("CoupleNodes::%s::%s", m_pickUpNode.c_str(), m_targetNode.c_str()));
    }

    void CoupleNodesState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const JE::Editor::Tools::GraphCanvas::NodeId* pickUpNodeId = GetStateModel()->GetStateDataAs<JE::Editor::Tools::GraphCanvas::NodeId>(m_pickUpNode);
        const JE::Editor::Tools::GraphCanvas::NodeId* targetNodeId = GetStateModel()->GetStateDataAs<JE::Editor::Tools::GraphCanvas::NodeId>(m_targetNode);

        if (pickUpNodeId && targetNodeId)
        {
            m_coupleNodesAction = aznew CoupleNodesAction((*pickUpNodeId), m_connectionType, (*targetNodeId));
            actionRunner.AddAction(m_coupleNodesAction);
        }
        else
        {
            if (pickUpNodeId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid EntityId", m_pickUpNode.c_str()));
            }

            if (targetNodeId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid EntityId", m_targetNode.c_str()));
            }
        }
    }

    void CoupleNodesState::OnStateActionsComplete()
    {
        if (!m_outputId.empty())
        {
            AZStd::vector< JE::Editor::Tools::GraphCanvas::ConnectionId > connectionId = m_coupleNodesAction->GetConnectionIds();
            GetStateModel()->SetStateData(m_outputId, connectionId);
        }

        delete m_coupleNodesAction;
        m_coupleNodesAction = nullptr;
    }

    //////////////////////////
    // ConnectEndpointsState
    //////////////////////////

    ConnectEndpointsState::ConnectEndpointsState(AutomationStateModelId sourceEndpoint, AutomationStateModelId targetEndpoint, AutomationStateModelId outputId)
        : NamedAutomationState("ConnectEndpointsState")
        , m_sourceEndpoint(sourceEndpoint)
        , m_targetEndpoint(targetEndpoint)
        , m_outputId(outputId)
    {
        SetStateName(AZStd::string::format("ConnectEndpoints::%s::%s", sourceEndpoint.c_str(), targetEndpoint.c_str()));
    }

    void ConnectEndpointsState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const JE::Editor::Tools::GraphCanvas::Endpoint* sourceEndpoint = GetStateModel()->GetStateDataAs<JE::Editor::Tools::GraphCanvas::Endpoint>(m_sourceEndpoint);
        const JE::Editor::Tools::GraphCanvas::Endpoint* targetEndpoint = GetStateModel()->GetStateDataAs<JE::Editor::Tools::GraphCanvas::Endpoint>(m_targetEndpoint);

        if (sourceEndpoint && targetEndpoint)
        {
            m_connectEndpointsAction = aznew ConnectEndpointsAction((*sourceEndpoint), (*targetEndpoint));
            actionRunner.AddAction(m_connectEndpointsAction);
        }
        else
        {
            if (sourceEndpoint == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid GraphCanvas::Endpoint", m_sourceEndpoint.c_str()));
            }

            if (targetEndpoint == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid GraphCanvas::Endpoint", m_targetEndpoint.c_str()));
            }
        }
    }

    void ConnectEndpointsState::OnStateActionsComplete()
    {
        if (!m_outputId.empty())
        {
            JE::Editor::Tools::GraphCanvas::ConnectionId connectionId = m_connectEndpointsAction->GetConnectionId();
            GetStateModel()->SetStateData(m_outputId, connectionId);
        }

        delete m_connectEndpointsAction;
        m_connectEndpointsAction = nullptr;
    }
}
