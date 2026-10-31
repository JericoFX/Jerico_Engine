/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/EntitySaveDataBus.h>
#include <JE/Editor/Tools/GraphCanvas/Types/EntitySaveData.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    class ScriptEventReceiverNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public EBusHandlerNodeDescriptorRequestBus::Handler
        , public ScriptEventReceiverNodeDescriptorRequestBus::Handler
        , public JE::Editor::Tools::GraphCanvas::WrapperNodeNotificationBus::Handler
        , public JE::Editor::Tools::GraphCanvas::GraphCanvasPropertyBusHandler
        , public JE::Editor::Tools::GraphCanvas::WrapperNodeConfigurationRequestBus::Handler
        , public JE::Editor::Tools::GraphCanvas::EntitySaveDataRequestBus::Handler
        , public JE::Editor::Tools::GraphCanvas::SceneMemberNotificationBus::Handler
        , public AZ::Data::AssetBus::Handler
        , public EditorNodeNotificationBus::Handler
    {
    public:
        class ScriptEventReceiverHandlerNodeDescriptorSaveData
            : public JE::Editor::Tools::GraphCanvas::ComponentSaveData
        {
        public:
            AZ_RTTI(ScriptEventReceiverHandlerNodeDescriptorSaveData, "{D8BBE799-7E4D-495A-B69A-1E3940670891}", JE::Editor::Tools::GraphCanvas::ComponentSaveData);
            AZ_CLASS_ALLOCATOR(ScriptEventReceiverHandlerNodeDescriptorSaveData, AZ::SystemAllocator);

            ScriptEventReceiverHandlerNodeDescriptorSaveData();
            ScriptEventReceiverHandlerNodeDescriptorSaveData(ScriptEventReceiverNodeDescriptorComponent* component);

            ~ScriptEventReceiverHandlerNodeDescriptorSaveData() = default;

            void operator=(const ScriptEventReceiverHandlerNodeDescriptorSaveData& other);

            void OnDisplayConnectionsChanged();

            bool m_displayConnections;
            AZStd::vector<AZStd::pair<ScriptCanvas::EBusEventId, AZStd::string>> m_enabledEvents;
        private:

            ScriptEventReceiverNodeDescriptorComponent * m_callback;
        };

        friend class ScriptEventReceiverHandlerNodeDescriptorSaveData;

        AZ_COMPONENT(ScriptEventReceiverNodeDescriptorComponent, "{FF9D3121-64B5-41C8-99D4-211528F39615}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        ScriptEventReceiverNodeDescriptorComponent();
        ScriptEventReceiverNodeDescriptorComponent(AZ::Data::AssetId assetId);
        ~ScriptEventReceiverNodeDescriptorComponent() = default;

        void Activate() override;
        void Deactivate() override;

        // NodeNotifications
        void OnNodeActivated() override;
        ////

        // SceneMemberNotifications
        void OnMemberSetupComplete() override;
        void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const JE::Editor::Tools::GraphCanvas::GraphSerialization&) override;
        ////

        // GraphCanvas::EntitySaveDataRequestBus::Handler
        void WriteSaveData(JE::Editor::Tools::GraphCanvas::EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const JE::Editor::Tools::GraphCanvas::EntitySaveDataContainer& saveDataContainer) override;
        ////

        // ScriptEventReceiverNodeDescriptorRequests
        AZ::Data::AssetId GetAssetId() const override;
        ////

        // EBusHandlerNodeDescriptorRequestBus
        AZStd::string_view GetBusName() const override;

        bool ContainsEvent(const ScriptCanvas::EBusEventId& eventId) const override;

        JE::Editor::Tools::GraphCanvas::WrappedNodeConfiguration GetEventConfiguration(const ScriptCanvas::EBusEventId& eventId) const override;
        AZStd::vector< HandlerEventConfiguration > GetEventConfigurations() const override;
        
        AZ::EntityId FindEventNodeId(const ScriptCanvas::EBusEventId& eventId) const override;
        AZ::EntityId FindGraphCanvasNodeIdForSlot(const ScriptCanvas::SlotId& slotId) const override;

        JE::Editor::Tools::GraphCanvas::Endpoint MapSlotToGraphCanvasEndpoint(const ScriptCanvas::SlotId& slotId) const override;
        ////

        // WrapperNodeNotificationBus
        void OnWrappedNode(const AZ::EntityId& wrappedNode) override;
        void OnUnwrappedNode(const AZ::EntityId& unwrappedNode) override;
        ////

        // WrapperNodeConfigurationRequests
        JE::Editor::Tools::GraphCanvas::WrappedNodeConfiguration GetWrappedNodeConfiguration(const AZ::EntityId& wrappedNodeId) const override;
        ////

        // GraphCanvasPropertyBus
        AZ::Component* GetPropertyComponent() override;
        ////

        // AZ::Data::AssetBus::Handler
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////

        // EditorNodeNotificationBus::Handler
        void OnVersionConversionBegin() override;
        void OnVersionConversionEnd() override;
        ////

    protected:

        void OnAddedToGraphCanvasGraph(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;

    private:

        void OnDisplayConnectionsChanged();

        void UpdateTitles(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>& asset);

        ScriptEventReceiverHandlerNodeDescriptorSaveData m_saveData;
        
        AZ::Crc32       m_busId;
        AZStd::string   m_busName;
        bool            m_loadingEvents;

        AZ::Data::AssetId m_scriptEventsAssetId;
        ScriptEvents::Method m_methodDefinition;

        AZ::EntityId m_scriptCanvasId;
        using EventKey = AZStd::pair<AZ::Uuid, AZStd::string>;
        
        AZStd::unordered_map<ScriptCanvas::EBusEventId, AZ::EntityId> m_eventTypeToId;
        AZStd::unordered_map<AZ::EntityId, ScriptCanvas::EBusEventId> m_idToEventType;
    };
}
