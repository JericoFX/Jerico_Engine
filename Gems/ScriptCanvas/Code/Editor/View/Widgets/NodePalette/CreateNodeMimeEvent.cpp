/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <JE/Editor/Tools/GraphCanvas/Components/GridBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/VisualBus.h>

#include "CreateNodeMimeEvent.h"

#include "ScriptCanvas/Bus/RequestBus.h"

namespace ScriptCanvasEditor
{
    ////////////////////////
    // CreateNodeMimeEvent
    ////////////////////////

    void CreateNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateNodeMimeEvent, JE::Editor::Tools::GraphCanvas::CreateSplicingNodeMimeEvent>()
                ->Version(0)
                ;
        }
    }

    const ScriptCanvasEditor::NodeIdPair& CreateNodeMimeEvent::GetCreatedPair() const
    {
        return m_nodeIdPair;
    }
        
    bool CreateNodeMimeEvent::ExecuteEvent(const AZ::Vector2&, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        if (!scriptCanvasId.IsValid() || !graphCanvasGraphId.IsValid())
        {
            return false;
        }

        m_nodeIdPair = CreateNode(scriptCanvasId);

        if (m_nodeIdPair.m_graphCanvasId.IsValid() && m_nodeIdPair.m_scriptCanvasId.IsValid())
        {
            m_createdNodeId = m_nodeIdPair.m_graphCanvasId;

            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::AddNode, m_nodeIdPair.m_graphCanvasId, sceneDropPosition, false);
            JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::Event(m_nodeIdPair.m_graphCanvasId, &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::SetSelected, true);

            ScriptCanvasEditor::NodeCreationNotificationBus::Event(scriptCanvasId, &ScriptCanvasEditor::NodeCreationNotifications::OnGraphCanvasNodeCreated, m_nodeIdPair.m_graphCanvasId);

            AZ::EntityId gridId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetGrid);

            AZ_Assert(gridId.IsValid(), "Grid must be valid, graphCanvasGraphId is likely incorrect");

            AZ::Vector2 offset;
            JE::Editor::Tools::GraphCanvas::GridRequestBus::EventResult(offset, gridId, &JE::Editor::Tools::GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
            return true;
        }
        else
        {
            if (m_nodeIdPair.m_graphCanvasId.IsValid())
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_nodeIdPair.m_graphCanvasId);
            }

            if (m_nodeIdPair.m_scriptCanvasId.IsValid())
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_nodeIdPair.m_scriptCanvasId);
            }
        }

        return false;
    }

    AZ::EntityId CreateNodeMimeEvent::CreateSplicingNode(const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        ScriptCanvasEditor::NodeIdPair idPair = CreateNode(scriptCanvasId);

        if (idPair.m_graphCanvasId.IsValid() && idPair.m_scriptCanvasId.IsValid())
        {
            return idPair.m_graphCanvasId;
        }

        return AZ::EntityId();
    }

    ///////////////////////////////////
    // SpecializedCreateNodeMimeEvent
    ///////////////////////////////////

    void SpecializedCreateNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<SpecializedCreateNodeMimeEvent, JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    /////////////////////////////
    // MultiCreateNodeMimeEvent
    /////////////////////////////

    void MultiCreateNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<MultiCreateNodeMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(0)
                ;
        }
    }
}
