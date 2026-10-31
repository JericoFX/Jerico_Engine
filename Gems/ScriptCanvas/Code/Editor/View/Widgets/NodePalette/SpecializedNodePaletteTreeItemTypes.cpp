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
#include <JE/Editor/Tools/GraphCanvas/GraphCanvasBus.h>


#include "SpecializedNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeCreateUtils.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"
#include "Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h"

#include <Core/Attributes.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////
    // CreateCommentNodeMimeEvent
    ///////////////////////////////

    void CreateCommentNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateCommentNodeMimeEvent, JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    NodeIdPair CreateCommentNodeMimeEvent::ConstructNode(const JE::Editor::Tools::GraphCanvas::GraphId& sceneId, const AZ::Vector2& scenePosition)
    {
        NodeIdPair retVal;

        AZ::Entity* graphCanvasEntity = nullptr;
        JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &JE::Editor::Tools::GraphCanvas::GraphCanvasRequests::CreateCommentNodeAndActivate);

        if (graphCanvasEntity)
        {
            retVal.m_graphCanvasId = graphCanvasEntity->GetId();
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(sceneId, &JE::Editor::Tools::GraphCanvas::SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePosition, false);
            JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        }

        return retVal;
    }

    bool CreateCommentNodeMimeEvent::ExecuteEvent([[maybe_unused]] const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const JE::Editor::Tools::GraphCanvas::GraphId& graphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        NodeIdPair nodeId = ConstructNode(graphId, sceneDropPosition);

        if (nodeId.m_graphCanvasId.IsValid())
        {
            AZ::EntityId gridId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(gridId, graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            JE::Editor::Tools::GraphCanvas::GridRequestBus::EventResult(offset, gridId, &JE::Editor::Tools::GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }

        return nodeId.m_graphCanvasId.IsValid();
    }

    ///////////////////////////////
    // CommentNodePaletteTreeItem
    ///////////////////////////////

    CommentNodePaletteTreeItem::CommentNodePaletteTreeItem(AZStd::string_view nodeName, [[maybe_unused]] const QString& iconPath)
        : DraggableNodePaletteTreeItem(nodeName, ScriptCanvasEditor::AssetEditorId)
    {
        SetToolTip("Comment box for notes. Does not affect script execution or data.");

        SetTitlePalette("CommentNodeTitlePalette");
    }

    JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent* CommentNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateCommentNodeMimeEvent();
    }

    /////////////////////////////
    // CreateNodeGroupMimeEvent
    /////////////////////////////

    void CreateNodeGroupMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateNodeGroupMimeEvent, JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    NodeIdPair CreateNodeGroupMimeEvent::ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition)
    {
        NodeIdPair retVal;

        AZ::Entity* graphCanvasEntity = nullptr;
        JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &JE::Editor::Tools::GraphCanvas::GraphCanvasRequests::CreateNodeGroupAndActivate);

        if (graphCanvasEntity)
        {
            retVal.m_graphCanvasId = graphCanvasEntity->GetId();
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePosition, false);
            JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        }

        return retVal;
    }

    bool CreateNodeGroupMimeEvent::ExecuteEvent([[maybe_unused]] const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        NodeIdPair nodeId = ConstructNode(graphCanvasGraphId, sceneDropPosition);

        if (nodeId.m_graphCanvasId.IsValid())
        {
            AZ::EntityId gridId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            JE::Editor::Tools::GraphCanvas::GridRequestBus::EventResult(offset, gridId, &JE::Editor::Tools::GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }

        return nodeId.m_graphCanvasId.IsValid();
    }

    ////////////////////////////////////
    // NodeGroupNodePaletteTreeItem
    ////////////////////////////////////

    NodeGroupNodePaletteTreeItem::NodeGroupNodePaletteTreeItem(AZStd::string_view nodeName, [[maybe_unused]] const QString& iconPath)
        : DraggableNodePaletteTreeItem(nodeName, ScriptCanvasEditor::AssetEditorId)
    {
    }

    JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent* NodeGroupNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateNodeGroupMimeEvent();
    }
}
