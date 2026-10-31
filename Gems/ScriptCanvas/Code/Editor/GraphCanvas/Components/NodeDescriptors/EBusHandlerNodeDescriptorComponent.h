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

namespace ScriptCanvasEditor
{
    class EBusHandlerNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public EBusHandlerNodeDescriptorRequestBus::Handler
        , public JE::Editor::Tools::GraphCanvas::WrapperNodeNotificationBus::Handler
        , public JE::Editor::Tools::GraphCanvas::GraphCanvasPropertyBusHandler
        , public JE::Editor::Tools::GraphCanvas::WrapperNodeConfigurationRequestBus::Handler
        , public JE::Editor::Tools::GraphCanvas::EntitySaveDataRequestBus::Handler
        , public JE::Editor::Tools::GraphCanvas::SceneMemberNotificationBus::Handler
    {
    public:
        class EBusHandlerNodeDescriptorSaveData
            : public JE::Editor::Tools::GraphCanvas::ComponentSaveData
        {
        public:
            AZ_RTTI(EBusHandlerNodeDescriptorSaveData, "{9E81C95F-89C0-4476-8E82-63CCC4E52E04}", JE::Editor::Tools::GraphCanvas::ComponentSaveData);
            AZ_CLASS_ALLOCATOR(EBusHandlerNodeDescriptorSaveData, AZ::SystemAllocator);

            EBusHandlerNodeDescriptorSaveData();
            EBusHandlerNodeDescriptorSaveData(EBusHandlerNodeDescriptorComponent* component);

            ~EBusHandlerNodeDescriptorSaveData() = default;

            void operator=(const EBusHandlerNodeDescriptorSaveData& other);

            void OnDisplayConnectionsChanged();

            bool                            m_displayConnections;
            AZStd::vector< ScriptCanvas::EBusEventId > m_enabledEvents;
        private:

            EBusHandlerNodeDescriptorComponent* m_callback;
        };        

        friend class EBusHandlerNodeDescriptorSaveData;
    
        AZ_COMPONENT(EBusHandlerNodeDescriptorComponent, "{A93B4B22-DBB8-4F18-B741-EB041BFEA4F6}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        EBusHandlerNodeDescriptorComponent();
        EBusHandlerNodeDescriptorComponent(const AZStd::string& busName);
        ~EBusHandlerNodeDescriptorComponent() = default;

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

        // EBusNodeDescriptorRequestBus
        AZStd::string_view GetBusName() const override;

        bool ContainsEvent(const ScriptCanvas::EBusEventId& eventId) const override;
        JE::Editor::Tools::GraphCanvas::WrappedNodeConfiguration GetEventConfiguration(const ScriptCanvas::EBusEventId& eventName) const override;

        AZStd::vector< HandlerEventConfiguration > GetEventConfigurations() const override;

        AZ::EntityId FindEventNodeId(const ScriptCanvas::EBusEventId& eventId) const override;
        AZ::EntityId FindGraphCanvasNodeIdForSlot(const ScriptCanvas::SlotId& eventName) const override;

        JE::Editor::Tools::GraphCanvas::Endpoint MapSlotToGraphCanvasEndpoint(const ScriptCanvas::SlotId& eventName) const override;
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

    protected:
        void OnAddedToGraphCanvasGraph(const JE::Editor::Tools::GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;

    private:

        void OnDisplayConnectionsChanged();
        
        EBusHandlerNodeDescriptorSaveData m_saveData;

        AZStd::string   m_busName;
        bool            m_loadingEvents;

        AZ::EntityId m_scriptCanvasId;
        AZStd::unordered_map< ScriptCanvas::EBusEventId, AZ::EntityId > m_eventTypeToId;
        AZStd::unordered_map< AZ::EntityId, ScriptCanvas::EBusEventId > m_idToEventType;
    };
}
