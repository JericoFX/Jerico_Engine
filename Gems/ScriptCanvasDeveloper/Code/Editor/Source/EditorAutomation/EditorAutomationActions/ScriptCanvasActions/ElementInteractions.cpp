/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/ViewBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/VisualBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/AssetEditorBus.h>

#include <JE/Editor/Tools/GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

namespace ScriptCanvas::Developer
{
    /////////////////////////////
    // SelectSceneElementAction
    /////////////////////////////

    SelectSceneElementAction::SelectSceneElementAction(AZ::EntityId sceneMemberId)
        : m_sceneMemberId(sceneMemberId)
    {
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(m_graphId, m_sceneMemberId, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);

        if (!JE::Editor::Tools::GraphCanvas::GraphUtils::IsConnection(sceneMemberId))
        {
            QRectF boundingRect;

            QGraphicsItem* graphicsItem = nullptr;
            JE::Editor::Tools::GraphCanvas::VisualRequestBus::EventResult(graphicsItem, sceneMemberId, &JE::Editor::Tools::GraphCanvas::VisualRequests::AsGraphicsItem);

            if (graphicsItem)
            {
                boundingRect = graphicsItem->sceneBoundingRect();
            }

            m_scenePoint = boundingRect.topLeft();
            m_scenePoint.setX(boundingRect.center().x());
            m_scenePoint.setY(m_scenePoint.y() + 5);
        }
        else
        {
            QPainterPath outlinePath;
            JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::EventResult(outlinePath, sceneMemberId, &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::GetOutline);

            m_scenePoint = outlinePath.pointAtPercent(0.5);
        }
    }

    bool SelectSceneElementAction::IsMissingPrecondition()
    {
        QRectF viewableArea;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return m_sceneMemberId.IsValid() && m_graphId.IsValid() && !viewableArea.contains(m_scenePoint);
    }

    EditorAutomationAction* SelectSceneElementAction::GenerateMissingPreconditionAction()
    {
        CompoundAction* compoundAction = aznew CompoundAction();

        QPoint startPoint(static_cast<int>(m_scenePoint.x() - 5.0), static_cast<int>(m_scenePoint.y() - 5.0));
        QPoint endPoint(static_cast<int>(m_scenePoint.x() + 5.0), static_cast<int>(m_scenePoint.y() + 5.0));

        QRect sceneRect = QRect(startPoint, endPoint);

        compoundAction->AddAction(aznew EnsureSceneRectVisibleAction(m_graphId, sceneRect));
        compoundAction->AddAction(aznew ProcessUserEventsAction());

        return compoundAction;
    }

    void SelectSceneElementAction::SetupAction()
    {
        ClearActionQueue();

        AZ::Vector2 screenPoint = AZ::Vector2::CreateZero();
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(screenPoint, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::MapToGlobal, JE::Editor::Tools::GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

        AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint(screenPoint).toPoint()));
        AddAction(aznew ProcessUserEventsAction());

        CompoundAction::SetupAction();
    }

    ///////////////////////////////
    // AltClickSceneElementAction
    ///////////////////////////////

    AltClickSceneElementAction::AltClickSceneElementAction(AZ::EntityId sceneMemberId)
        : m_sceneMemberId(sceneMemberId)
    {
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(m_graphId, m_sceneMemberId, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);

        if (!JE::Editor::Tools::GraphCanvas::GraphUtils::IsConnection(sceneMemberId))
        {
            QRectF boundingRect;

            QGraphicsItem* graphicsItem = nullptr;
            JE::Editor::Tools::GraphCanvas::VisualRequestBus::EventResult(graphicsItem, sceneMemberId, &JE::Editor::Tools::GraphCanvas::VisualRequests::AsGraphicsItem);

            if (graphicsItem)
            {
                boundingRect = graphicsItem->sceneBoundingRect();
            }

            m_scenePoint = boundingRect.topLeft();
            m_scenePoint.setX(boundingRect.center().x());
            m_scenePoint.setY(m_scenePoint.y() + 5);
        }
        else
        {
            QPainterPath outlinePath;
            JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::EventResult(outlinePath, sceneMemberId, &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::GetOutline);

            m_scenePoint = outlinePath.pointAtPercent(0.5);
        }
    }

    bool AltClickSceneElementAction::IsMissingPrecondition()
    {
        QRectF viewableArea;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return !viewableArea.contains(m_scenePoint);
    }

    EditorAutomationAction* AltClickSceneElementAction::GenerateMissingPreconditionAction()
    {
        CompoundAction* compoundAction = aznew CompoundAction();

        QPoint startPoint(static_cast<int>(m_scenePoint.x() - 5.0), static_cast<int>(m_scenePoint.y() - 5.0));
        QPoint endPoint(static_cast<int>(m_scenePoint.x() + 5.0), static_cast<int>(m_scenePoint.y() + 5.0));

        QRect sceneRect = QRect(startPoint, endPoint);

        compoundAction->AddAction(aznew EnsureSceneRectVisibleAction(m_graphId, sceneRect));
        compoundAction->AddAction(aznew ProcessUserEventsAction());

        return compoundAction;
    }

    void AltClickSceneElementAction::SetupAction()
    {
        ClearActionQueue();
        m_sceneMemberRemoved = false;
        JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphId);
#if defined(AZ_COMPILER_MSVC)
        AddAction(aznew KeyPressAction(VK_LMENU));

        AZ::Vector2 screenPoint = AZ::Vector2::CreateZero();
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(screenPoint, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::MapToGlobal, JE::Editor::Tools::GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

        AddAction(aznew MouseClickAction(Qt::MouseButton::LeftButton, JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint(screenPoint).toPoint()));
        AddAction(aznew KeyReleaseAction(VK_LMENU));
        AddAction(aznew ProcessUserEventsAction(AZStd::chrono::milliseconds(750)));
#endif
        CompoundAction::SetupAction();
    }

    ActionReport AltClickSceneElementAction::GenerateReport() const
    {
        if (!m_sceneMemberRemoved)
        {
            return AZ::Failure<AZStd::string>("Failed to delete target scene element with Alt+Click");
        }

        return CompoundAction::GenerateReport();
    }

    void AltClickSceneElementAction::OnNodeRemoved(const AZ::EntityId& nodeId)
    {
        if (m_sceneMemberId == nodeId)
        {
            m_sceneMemberRemoved = true;
        }
    }

    void AltClickSceneElementAction::OnConnectionRemoved(const AZ::EntityId& connectionId)
    {
        if (m_sceneMemberId == connectionId)
        {
            m_sceneMemberRemoved = true;
        }
    }

    void AltClickSceneElementAction::OnActionsComplete()
    {
        JE::Editor::Tools::GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////
    // MouseToNodePropertyEditorAction
    ////////////////////////////////////

    MouseToNodePropertyEditorAction::MouseToNodePropertyEditorAction(JE::Editor::Tools::GraphCanvas::SlotId slotId)
        : m_slotId(slotId)
    {
    }

    void MouseToNodePropertyEditorAction::SetupAction()
    {
        ClearActionQueue();

        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(nodeId, m_slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetNode);

        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, nodeId, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        QRectF sceneBoundingRect;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(nodeId, m_slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetNode);
        JE::Editor::Tools::GraphCanvas::DataSlotLayoutRequestBus::EventResult(sceneBoundingRect, m_slotId, &JE::Editor::Tools::GraphCanvas::DataSlotLayoutRequests::GetWidgetSceneBoundingRect);

        AddAction(aznew SceneMouseMoveAction(graphId, sceneBoundingRect.center()));

        CompoundAction::SetupAction();
    }
}
