/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>


#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////
    // NodeDescriptorComponent
    ////////////////////////////

    void NodeDescriptorComponent::Reflect(AZ::ReflectContext* reflect)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflect))
        {
            serializeContext->Class<NodeDescriptorComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    NodeDescriptorComponent::NodeDescriptorComponent()
        : m_nodeDescriptorType(NodeDescriptorType::Unknown)
    {
    }

    NodeDescriptorComponent::NodeDescriptorComponent(NodeDescriptorType descriptorType)
        : m_nodeDescriptorType(descriptorType)
    {
    }

    void NodeDescriptorComponent::Init()
    {
    }

    void NodeDescriptorComponent::Activate()
    {
        NodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        JE::Editor::Tools::GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void NodeDescriptorComponent::Deactivate()
    {
        JE::Editor::Tools::GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        NodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    void NodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        AZ::EntityId scriptCanvasNodeId = FindScriptCanvasNodeId();

        ScriptCanvas::NodeNotificationsBus::Handler::BusConnect(scriptCanvasNodeId);

        OnAddedToGraphCanvasGraph(sceneId, scriptCanvasNodeId);

        bool isEnabled = true;
        ScriptCanvas::NodeRequestBus::EventResult(isEnabled, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::IsNodeEnabled);

        if (!isEnabled)
        {
            JE::Editor::Tools::GraphCanvas::GraphId graphId;
            JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

            JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::DisableVisualState, GetEntityId());
        }
    }

    void NodeDescriptorComponent::OnAddedToGraphCanvasGraph(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(scriptCanvasNodeId);
    }

    void NodeDescriptorComponent::OnNodeDisabled()
    {
        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::DisableVisualState, GetEntityId());
    }

    void NodeDescriptorComponent::OnNodeEnabled()
    {
        JE::Editor::Tools::GraphCanvas::GraphId graphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::EnableVisualState, GetEntityId());
    }

    AZ::EntityId NodeDescriptorComponent::FindScriptCanvasNodeId() const
    {
        AZStd::any* userData = nullptr;
        JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &JE::Editor::Tools::GraphCanvas::NodeRequests::GetUserData);

        AZ::EntityId* scNodeId = AZStd::any_cast<AZ::EntityId>(userData);
        if (scNodeId)
        {
            return (*scNodeId);            
        }

        return AZ::EntityId();
    }
}
