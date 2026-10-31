/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/ViewBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/VisualBus.h>

#include <JE/Editor/Tools/GraphCanvas/Utils/ConversionUtils.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>

namespace ScriptCanvas::Developer
{
    /////////////////////////////
    // CenterOnScenePointAction
    /////////////////////////////

    CenterOnScenePointAction::CenterOnScenePointAction(JE::Editor::Tools::GraphCanvas::GraphId graphId, QPointF scenePoint)
        : m_graphId(graphId)
        , m_scenePoint(scenePoint)
    {

    }

    bool CenterOnScenePointAction::Tick()
    {
        JE::Editor::Tools::GraphCanvas::ViewId viewId;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);

        if (viewId.IsValid())
        {
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::CenterOn, m_scenePoint);
        }

        return true;
    }

    /////////////////////////////////
    // EnsureSceneRectVisibleAction
    /////////////////////////////////

    EnsureSceneRectVisibleAction::EnsureSceneRectVisibleAction(JE::Editor::Tools::GraphCanvas::GraphId graphId, QRectF sceneRect)
        : DelayAction(AZStd::chrono::milliseconds(250))
        , m_graphId(graphId)
        , m_sceneRect(sceneRect)
    {
    }

    void EnsureSceneRectVisibleAction::SetupAction()
    {
        DelayAction::SetupAction();

        m_firstTick = true;
    }

    bool EnsureSceneRectVisibleAction::Tick()
    {
        if (m_firstTick)
        {
            m_firstTick = false;
            JE::Editor::Tools::GraphCanvas::ViewId viewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);

            if (viewId.IsValid())
            {
                JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::CenterOnArea, m_sceneRect);
            }

            return false;
        }
        else
        {
            return DelayAction::Tick();
        }
    }
    
    /////////////////////////
    // SceneMouseMoveAction
    /////////////////////////

    SceneMouseMoveAction::SceneMouseMoveAction(JE::Editor::Tools::GraphCanvas::GraphId graphId, QPointF scenePoint)
        : m_graphId(graphId)
        , m_scenePoint(scenePoint)
    {
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
    }

    bool SceneMouseMoveAction::IsMissingPrecondition()
    {
        QRectF viewableBounds = QRectF(m_scenePoint, m_scenePoint);
        viewableBounds.adjust(-10, -10, 10, 10);

        QRectF viewableArea;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return !viewableArea.isEmpty() && !viewableArea.contains(viewableBounds);
    }

    EditorAutomationAction* SceneMouseMoveAction::GenerateMissingPreconditionAction()
    {
        QRectF viewableBounds = QRectF(m_scenePoint, m_scenePoint);
        viewableBounds.adjust(-10, -10, 10, 10);

        return aznew EnsureSceneRectVisibleAction(m_graphId, viewableBounds);
    }

    void SceneMouseMoveAction::SetupAction()
    {
        ClearActionQueue();

        AZ::Vector2 tempVector = AZ::Vector2::CreateZero();

        QPoint screenPoint;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(tempVector, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::MapToGlobal, JE::Editor::Tools::GraphCanvas::ConversionUtils::QPointToVector(m_scenePoint));

        screenPoint = JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint(tempVector).toPoint();

        AddAction(aznew MouseMoveAction(screenPoint));

        CompoundAction::SetupAction();
    }

    /////////////////////////
    // SceneMouseDragAction
    /////////////////////////

    SceneMouseDragAction::SceneMouseDragAction(JE::Editor::Tools::GraphCanvas::GraphId graphId, QPointF sceneStart, QPointF sceneEnd, Qt::MouseButton mouseButton)
        : m_graphId(graphId)
        , m_sceneStart(sceneStart)
        , m_sceneEnd(sceneEnd)
        , m_mouseButton(mouseButton)
    {
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(m_viewId, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
    }

    bool SceneMouseDragAction::IsMissingPrecondition()
    {
        QRectF viewableBounds = QRectF(m_sceneStart, m_sceneEnd);
        viewableBounds.adjust(-10, -10, 10, 10);

        QRectF viewableArea;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(viewableArea, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetViewableAreaInSceneCoordinates);

        return !viewableArea.contains(viewableBounds);
    }

    EditorAutomationAction* SceneMouseDragAction::GenerateMissingPreconditionAction()
    {
        QRectF viewableBounds = QRectF(m_sceneStart, m_sceneEnd);
        viewableBounds.adjust(-10, -10, 10, 10);

        return aznew EnsureSceneRectVisibleAction(m_graphId, viewableBounds);
    }

    void SceneMouseDragAction::SetupAction()
    {
        AZ::Vector2 tempVector = AZ::Vector2::CreateZero();

        QPoint screenStart;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(tempVector, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::MapToGlobal, JE::Editor::Tools::GraphCanvas::ConversionUtils::QPointToVector(m_sceneStart));

        screenStart = JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint(tempVector).toPoint();

        tempVector = AZ::Vector2::CreateZero();

        QPoint screenEnd;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(tempVector, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::MapToGlobal, JE::Editor::Tools::GraphCanvas::ConversionUtils::QPointToVector(m_sceneEnd));

        screenEnd = JE::Editor::Tools::GraphCanvas::ConversionUtils::AZToQPoint(tempVector).toPoint();

        AddAction(aznew MouseDragAction(screenStart, screenEnd, m_mouseButton));

        CompoundAction::SetupAction();
    }
}
