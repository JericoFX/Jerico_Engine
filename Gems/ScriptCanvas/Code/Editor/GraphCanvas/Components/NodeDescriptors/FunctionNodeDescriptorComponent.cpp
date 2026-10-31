/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FunctionNodeDescriptorComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include <Editor/Translation/TranslationHelper.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Libraries/Core/ScriptEventBase.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////////////////
    // FunctionNodeDescriptorComponent
    /////////////////////////////////////////////

    void FunctionNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<FunctionNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ->Field("AssetId", &FunctionNodeDescriptorComponent::m_assetId)
                ;
        }
    }

    FunctionNodeDescriptorComponent::FunctionNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::FunctionNode)
    {
    }

    FunctionNodeDescriptorComponent::FunctionNodeDescriptorComponent(const AZ::Data::AssetId& assetId, const AZStd::string& name)
        : NodeDescriptorComponent(NodeDescriptorType::FunctionNode)
        , m_assetId(assetId)
        , m_name(name)
    {
        
    }

    void FunctionNodeDescriptorComponent::Activate()
    {
        NodeDescriptorComponent::Activate();
        JE::Editor::Tools::GraphCanvas::VisualNotificationBus::Handler::BusConnect(GetEntityId());
        
        AZ::Data::AssetBus::Handler::BusConnect(m_assetId);
    }

    void FunctionNodeDescriptorComponent::Deactivate()
    {
        JE::Editor::Tools::GraphCanvas::VisualNotificationBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void FunctionNodeDescriptorComponent::OnAssetReloaded([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        TriggerUpdate();
    }

    void FunctionNodeDescriptorComponent::OnAssetReloadError([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        TriggerUpdate();
    }

    void FunctionNodeDescriptorComponent::OnAssetError([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        TriggerUpdate();
    }

    void FunctionNodeDescriptorComponent::OnVersionConversionEnd()
    {}

    bool FunctionNodeDescriptorComponent::OnMouseDoubleClick(const QGraphicsSceneMouseEvent*)
    {
        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
        GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset
            , SourceHandle(nullptr, m_assetId.m_guid), Tracker::ScriptCanvasFileState::UNMODIFIED, -1);
        return openOutcome.IsSuccess();
    }

    void FunctionNodeDescriptorComponent::OnAddedToGraphCanvasGraph(const JE::Editor::Tools::GraphCanvas::GraphId&, const AZ::EntityId& scriptCanvasNodeId)
    {
        m_scriptCanvasId = scriptCanvasNodeId;
        EditorNodeNotificationBus::Handler::BusConnect(m_scriptCanvasId);
    }

    void FunctionNodeDescriptorComponent::TriggerUpdate()
    {
        AZ::EntityId graphCanvasGraphId;
        JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, GetEntityId(), &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);

        if (graphCanvasGraphId.IsValid())
        {
            bool isInUndoRedo = false;
            GeneralRequestBus::BroadcastResult(isInUndoRedo, &GeneralRequests::IsInUndoRedo, graphCanvasGraphId);

            if (!isInUndoRedo)
            {
                ScriptCanvas::ScriptCanvasId scriptCanvasId;
                GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

                EditorGraphRequestBus::Event(scriptCanvasId, &EditorGraphRequests::QueueVersionUpdate, GetEntityId());
            }
        }
    }

    void FunctionNodeDescriptorComponent::UpdateTitles()
    {
        JE::Editor::Tools::GraphCanvas::NodeRequestBus::Event(GetEntityId(), &JE::Editor::Tools::GraphCanvas::NodeRequests::SetTooltip, m_name);
        JE::Editor::Tools::GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &JE::Editor::Tools::GraphCanvas::NodeTitleRequests::SetTitle, m_name);
    }
}
