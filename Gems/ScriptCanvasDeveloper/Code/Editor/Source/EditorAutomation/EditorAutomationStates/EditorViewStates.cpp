/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QTableView>

#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/ViewBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/VisualBus.h>

#include <JE/Editor/Tools/GraphCanvas/Utils/ConversionUtils.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/EditorViewStates.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvas::Developer
{
    ////////////////////////
    // SceneMouseMoveState
    ////////////////////////

    SceneMouseMoveState::SceneMouseMoveState(AutomationStateModelId targetPoint)
        : NamedAutomationState("SceneMouseMoveState")
        , m_targetPoint(targetPoint)
    {
        SetStateName(AZStd::string::format("SceneMouseMoveState::%s", targetPoint.c_str()));
    }

    void SceneMouseMoveState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const JE::Editor::Tools::GraphCanvas::GraphId* graphId = GetStateModel()->GetStateDataAs<JE::Editor::Tools::GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);
        const AZ::Vector2* scenePoint = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_targetPoint);

        if (scenePoint && graphId)
        {
            m_moveAction = aznew SceneMouseMoveAction((*graphId), JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint((*scenePoint)));
            actionRunner.AddAction(m_moveAction);
        }
        else
        {
            if (graphId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid GraphCanvas::GraphId", StateModelIds::GraphCanvasId));
            }

            if (scenePoint == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid AZ::Vector2", m_targetPoint.c_str()));
            }
        }
    }

    void SceneMouseMoveState::OnStateActionsComplete()
    {
        delete m_moveAction;
        m_moveAction = nullptr;
    }    

    ////////////////////////
    // SceneMouseDragState
    ////////////////////////

    SceneMouseDragState::SceneMouseDragState(AutomationStateModelId startPoint, AutomationStateModelId endPoint, Qt::MouseButton mouseButton)
        : NamedAutomationState("SceneMouseDragState")
        , m_startPoint(startPoint)
        , m_endPoint(endPoint)
        , m_mouseButton(mouseButton)
    {
        SetStateName(AZStd::string::format("SceneMouseDragState::%s::%s", m_startPoint.c_str(), m_endPoint.c_str()));
    }

    void SceneMouseDragState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const JE::Editor::Tools::GraphCanvas::GraphId* graphId = GetStateModel()->GetStateDataAs<JE::Editor::Tools::GraphCanvas::GraphId>(StateModelIds::GraphCanvasId);

        const AZ::Vector2* startPoint = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_startPoint);
        const AZ::Vector2* endPoint = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_endPoint);

        if (startPoint && endPoint && graphId)
        {
            m_dragAction = aznew SceneMouseDragAction((*graphId), JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint((*startPoint)), JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint((*endPoint)), m_mouseButton);
            actionRunner.AddAction(m_dragAction);
        }
        else
        {
            if (startPoint == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid AZ::Vector2", m_startPoint.c_str()));
            }

            if (endPoint == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid AZ::Vector2", m_endPoint.c_str()));
            }

            if (graphId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid GraphCanvas::GraphId", StateModelIds::GraphCanvasId));
            }
        }
    }

    void SceneMouseDragState::OnStateActionsComplete()
    {
        delete m_dragAction;
        m_dragAction = nullptr;
    }

    ////////////////////////
    // FindViewCenterState
    ////////////////////////

    FindViewCenterState::FindViewCenterState(AutomationStateModelId outputId)
        : CustomActionState("FindViewCenterState")
        , m_outputId(outputId)
    {
        SetStateName(AZStd::string::format("FindViewCenterState::%s", outputId.c_str()));
    }

    void FindViewCenterState::OnCustomAction()
    {
        if (!m_outputId.empty())
        {
            const JE::Editor::Tools::GraphCanvas::ViewId* viewId = GetStateModel()->GetStateDataAs<JE::Editor::Tools::GraphCanvas::ViewId>(StateModelIds::ViewId);

            if (viewId)
            {
                QRectF viewableRect;
                JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(viewableRect, (*viewId), &JE::Editor::Tools::GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

                AZ::Vector2 viewCenter = JE::Editor::Tools::GraphCanvas::ConversionUtils::QPointToVector(viewableRect.center());
                GetStateModel()->SetStateData(m_outputId, viewCenter);
            }
        }
    }
}
