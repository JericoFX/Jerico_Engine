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

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ConnectionActions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

namespace ScriptCanvas::Developer
{
    //////////////////////
    // CoupleNodesAction
    //////////////////////

    CoupleNodesAction::CoupleNodesAction(JE::Editor::Tools::GraphCanvas::NodeId nodeToPickUp, JE::Editor::Tools::GraphCanvas::ConnectionType connectionType, JE::Editor::Tools::GraphCanvas::NodeId coupleTarget)
        : m_nodeToPickUp(nodeToPickUp)
        , m_connectionType(connectionType)
        , m_targetNode(coupleTarget)
    {
    }

    bool CoupleNodesAction::IsMissingPrecondition()
    {
        QGraphicsItem* sourceGraphicsItem = nullptr;
        JE::Editor::Tools::GraphCanvas::VisualRequestBus::EventResult(sourceGraphicsItem, m_nodeToPickUp, &JE::Editor::Tools::GraphCanvas::VisualRequests::AsGraphicsItem);

        if (sourceGraphicsItem)
        {
            m_pickUpRect = sourceGraphicsItem->sceneBoundingRect();
        }

        QGraphicsItem* targetGraphicsItem = nullptr;
        JE::Editor::Tools::GraphCanvas::VisualRequestBus::EventResult(targetGraphicsItem, m_targetNode, &JE::Editor::Tools::GraphCanvas::VisualRequests::AsGraphicsItem);

        if (targetGraphicsItem)
        {
            m_targetRect = targetGraphicsItem->sceneBoundingRect();
        }

        m_sceneRect = m_pickUpRect;
        m_sceneRect |= m_targetRect;

        // Provide some extra spacing just to provide some safety
        m_sceneRect = m_sceneRect.adjusted(-m_sceneRect.width() * 0.25f, -m_sceneRect.height() * 0.25f, m_sceneRect.width() * 0.25f, m_sceneRect.height() * 0.25f);

        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, m_nodeToPickUp, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        JE::Editor::Tools::GraphCanvas::ViewId viewId;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);

        QRectF viewableArea;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(viewableArea, viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return !viewableArea.contains(m_sceneRect);
    }

    EditorAutomationAction* CoupleNodesAction::GenerateMissingPreconditionAction()
    {
        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, m_nodeToPickUp, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        return aznew EnsureSceneRectVisibleAction(graphId, m_sceneRect);
    }

    AZStd::vector< JE::Editor::Tools::GraphCanvas::ConnectionId > CoupleNodesAction::GetConnectionIds() const
    {
        return m_connections;
    }

    void CoupleNodesAction::OnConnectionAdded(const AZ::EntityId& connectionId)
    {
        m_connections.emplace_back(connectionId);
    }

    void CoupleNodesAction::SetupAction()
    {
        ClearActionQueue();

        QPointF mouseStartPoint = QPointF(m_pickUpRect.center().x(), m_pickUpRect.top() + 5);

        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, m_nodeToPickUp, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        JE::Editor::Tools::GraphCanvas::ViewId viewId;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);

        JE::Editor::Tools::GraphCanvas::ViewRequests* viewRequests = JE::Editor::Tools::GraphCanvas::ViewRequestBus::FindFirstHandler(viewId);

        if (viewRequests)
        {
            QPoint initialMousePosition = JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint(viewRequests->MapToGlobal(JE::Editor::Tools::GraphCanvas::ConversionUtils::QPointToVector(mouseStartPoint))).toPoint();
            AddAction(aznew MouseMoveAction(initialMousePosition));
            AddAction(aznew PressMouseButtonAction(Qt::MouseButton::LeftButton));

            QPointF targetPoint = m_targetRect.center();
            QPointF startPoint = m_pickUpRect.center();

            // Depending on which side of the pick-up node we want to connect, we need to target a different edge of the target node.
            if (m_connectionType == JE::Editor::Tools::GraphCanvas::ConnectionType::CT_Input)
            {
                targetPoint = QPointF(m_targetRect.right(), targetPoint.y());
            }
            else if (m_connectionType == JE::Editor::Tools::GraphCanvas::ConnectionType::CT_Output)
            {
                targetPoint = QPointF(m_targetRect.left(), targetPoint.y());
            }

            QPointF lineDelta = QPointF(targetPoint.x() - startPoint.x(), targetPoint.y() - startPoint.y());

            QPointF targetMousePosition = QPointF(mouseStartPoint.x() + lineDelta.x(), mouseStartPoint.y() + lineDelta.y());

            AddAction(aznew MouseMoveAction(JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint(viewRequests->MapToGlobal(JE::Editor::Tools::GraphCanvas::ConversionUtils::QPointToVector(targetMousePosition))).toPoint()));

            JE::Editor::Tools::GraphCanvas::EditorId editorId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(editorId, graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetEditorId);

            AZStd::chrono::milliseconds coupleDuration;
            JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::EventResult(coupleDuration, editorId, &JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequests::GetDragCouplingTime);

            // Double the couple duration to be safe.
            coupleDuration = coupleDuration + coupleDuration;

            AddAction(aznew DelayAction(coupleDuration));
            AddAction(aznew MouseMoveAction(initialMousePosition));
            AddAction(aznew ReleaseMouseButtonAction(Qt::MouseButton::LeftButton));
            AddAction(aznew DelayAction(AZStd::chrono::milliseconds(250)));
        }

        m_connections.clear();
        JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler::BusConnect(graphId);

        CompoundAction::SetupAction();
    }

    void CoupleNodesAction::OnActionsComplete()
    {
        JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }

    ///////////////////////////
    // ConnectEndpointsAction
    ///////////////////////////

    ConnectEndpointsAction::ConnectEndpointsAction(JE::Editor::Tools::GraphCanvas::Endpoint startEndpoint, JE::Editor::Tools::GraphCanvas::Endpoint targetEndpoint)
        : m_startEndpoint(startEndpoint)
        , m_targetEndpoint(targetEndpoint)
    {
        QPointF startScenePoint;
        JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(startScenePoint, startEndpoint.m_slotId, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetPinCenter);

        QPointF targetScenePoint;
        JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(targetScenePoint, targetEndpoint.m_slotId, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetPinCenter);

        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, startEndpoint.m_nodeId, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        AddAction(aznew SceneMouseDragAction(graphId, startScenePoint, targetScenePoint, Qt::MouseButton::LeftButton));
    }

    JE::Editor::Tools::GraphCanvas::ConnectionId ConnectEndpointsAction::GetConnectionId() const
    {
        return m_connectionId;
    }

    void ConnectEndpointsAction::OnActionsComplete()
    {
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(m_connectionId, m_startEndpoint.GetSlotId(), &JE::Editor::Tools::GraphCanvas::SlotRequests::GetLastConnection);
    }
}
