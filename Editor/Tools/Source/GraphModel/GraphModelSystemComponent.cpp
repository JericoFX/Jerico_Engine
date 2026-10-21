/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphModelSystemComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/Translation/TranslationDef.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <JE/Editor/Tools/GraphModel/Model/Connection.h>
#include <JE/Editor/Tools/GraphModel/Model/Graph.h>
#include <JE/Editor/Tools/GraphModel/Model/GraphContext.h>
#include <JE/Editor/Tools/GraphModel/Model/DataType.h>
#include <JE/Editor/Tools/GraphModel/Model/Node.h>
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>
#include <JE/Editor/Tools/GraphModel/Model/Module/InputOutputNodes.h>
#include <JE/Editor/Tools/GraphModel/Model/Module/ModuleNode.h>
#include <JE/Editor/Tools/GraphModel/Integration/GraphCanvasMetadata.h>
#include <JE/Editor/Tools/GraphModel/Integration/NodePalette/InputOutputNodePaletteItem.h>
#include <JE/Editor/Tools/GraphModel/Integration/NodePalette/GraphCanvasNodePaletteItems.h>
#include <JE/Editor/Tools/GraphModel/Integration/NodePalette/ModuleNodePaletteItem.h>

namespace JE::Editor::Tools::GraphModel
{
    void GraphModelSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect core graph classes
        DataType::Reflect(context);
        GraphContext::Reflect(context);
        GraphElement::Reflect(context);
        SlotId::Reflect(context);
        Slot::Reflect(context);
        Node::Reflect(context);
        Connection::Reflect(context);
        Graph::Reflect(context);

        // Reflect module node data types
        BaseInputOutputNode::Reflect(context);
        GraphInputNode::Reflect(context);
        GraphOutputNode::Reflect(context);
        ModuleNode::Reflect(context);

        // Reflect data types for integrating graph model with graph canvas
        JE::Editor::Tools::GraphModel::Integration::GraphCanvasMetadata::Reflect(context);
        JE::Editor::Tools::GraphModel::Integration::GraphCanvasSelectionData::Reflect(context);

        // Reflect MIME events for graph canvas nodes
        JE::Editor::Tools::GraphModel::Integration::CreateGraphCanvasNodeMimeEvent::Reflect(context);
        JE::Editor::Tools::GraphModel::Integration::CreateNodeGroupNodeMimeEvent::Reflect(context);
        JE::Editor::Tools::GraphModel::Integration::CreateCommentNodeMimeEvent::Reflect(context);

        // Reflect MIME events for module node
        JE::Editor::Tools::GraphModel::Integration::CreateInputOutputNodeMimeEvent<GraphInputNode>::Reflect(context);
        JE::Editor::Tools::GraphModel::Integration::CreateInputOutputNodeMimeEvent<GraphOutputNode>::Reflect(context);
        JE::Editor::Tools::GraphModel::Integration::CreateModuleNodeMimeEvent::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphModelSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphModelSystemComponent>(
                    QT_TRANSLATE_NOOP("GraphModel", "GraphModel"),
                    QT_TRANSLATE_NOOP("GraphModel", "A generic node graph data model"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<JE::Editor::Tools::GraphModel::Integration::GraphManagerRequestBus>("GraphManagerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Event("GetGraph", &JE::Editor::Tools::GraphModel::Integration::GraphManagerRequests::GetGraph)
                ;

            behaviorContext->EBus<JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus>("GraphControllerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Event("AddNode", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode)
                ->Event("RemoveNode", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::RemoveNode)
                ->Event("GetPosition", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetPosition)
                ->Event("WrapNode", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::WrapNode)
                ->Event("WrapNodeOrdered", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::WrapNodeOrdered)
                ->Event("UnwrapNode", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::UnwrapNode)
                ->Event("IsNodeWrapped", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::IsNodeWrapped)
                ->Event("SetWrapperNodeActionString", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::SetWrapperNodeActionString)
                ->Event("AddConnection", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddConnection)
                ->Event("AddConnectionBySlotId", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddConnectionBySlotId)
                ->Event("AreSlotsConnected", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AreSlotsConnected)
                ->Event("RemoveConnection", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::RemoveConnection)
                ->Event("ExtendSlot", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::ExtendSlot)
                ->Event("GetNodeById", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeById)
                ->Event("GetNodesFromGraphNodeIds", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodesFromGraphNodeIds)
                ->Event("GetNodeIdByNode", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeIdByNode)
                ->Event("GetSlotIdBySlot", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot)
                ->Event("GetNodes", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodes)
                ->Event("GetSelectedNodes", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSelectedNodes)
                ->Event("SetSelected", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::SetSelected)
                ->Event("ClearSelection", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::ClearSelection)
                ->Event("EnableNode", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::EnableNode)
                ->Event("DisableNode", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::DisableNode)
                ->Event("CenterOnNodes", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::CenterOnNodes)
                ->Event("GetMajorPitch", &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetMajorPitch)
            ;
        }
    }

    void GraphModelSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GraphModelService"));
    }

    void GraphModelSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GraphModelService"));
    }

    void GraphModelSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void GraphModelSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void GraphModelSystemComponent::Init()
    {
    }

    void GraphModelSystemComponent::Activate()
    {
        m_graphControllerManager = AZStd::make_unique<JE::Editor::Tools::GraphModel::Integration::GraphControllerManager>();
    }

    void GraphModelSystemComponent::Deactivate()
    {
        m_graphControllerManager.reset();
    }
}
