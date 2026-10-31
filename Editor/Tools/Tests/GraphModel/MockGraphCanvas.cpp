/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphModel/MockGraphCanvas.h>

namespace MockGraphCanvasServices
{
    // MockSlotComponent
    void MockSlotComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MockSlotComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    AZ::Entity* MockSlotComponent::CreateCoreSlotEntity()
    {
        AZ::Entity* entity = aznew AZ::Entity("Slot");
        return entity;
    }

    MockSlotComponent::MockSlotComponent(const JE::Editor::Tools::GraphCanvas::SlotType& slotType)
        : m_slotType(slotType)
    {
    }

    MockSlotComponent::MockSlotComponent(const JE::Editor::Tools::GraphCanvas::SlotType& slotType, const JE::Editor::Tools::GraphCanvas::SlotConfiguration& configuration)
        : m_slotType(slotType)
        , m_slotConfiguration(configuration)
    {
    }

    void MockSlotComponent::Activate()
    {
    }

    void MockSlotComponent::Deactivate()
    {
    }

    // MockDataSlotComponent
    void MockDataSlotComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MockDataSlotComponent, MockSlotComponent>()
                ->Version(0)
                ;
        }
    }

    AZ::Entity* MockDataSlotComponent::CreateDataSlot(const JE::Editor::Tools::GraphCanvas::DataSlotConfiguration& dataSlotConfiguration)
    {
        AZ::Entity* entity = MockSlotComponent::CreateCoreSlotEntity();

        MockDataSlotComponent* dataSlot = aznew MockDataSlotComponent(dataSlotConfiguration);

        if (!entity->AddComponent(dataSlot))
        {
            delete dataSlot;
            delete entity;
            return nullptr;
        }

        return entity;
    }

    MockDataSlotComponent::MockDataSlotComponent()
        : MockSlotComponent(JE::Editor::Tools::GraphCanvas::SlotTypes::DataSlot)
    {
    }

    MockDataSlotComponent::MockDataSlotComponent(const JE::Editor::Tools::GraphCanvas::DataSlotConfiguration& dataSlotConfiguration)
        : MockSlotComponent(JE::Editor::Tools::GraphCanvas::SlotTypes::DataSlot, dataSlotConfiguration)
        , m_dataSlotConfiguration(dataSlotConfiguration)
    {
    }

    void MockDataSlotComponent::Activate()
    {
        JE::Editor::Tools::GraphCanvas::DataSlotRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MockDataSlotComponent::Deactivate()
    {
        JE::Editor::Tools::GraphCanvas::DataSlotRequestBus::Handler::BusDisconnect();
    }

    bool MockDataSlotComponent::ConvertToReference([[maybe_unused]] bool isNewSlot)
    {
        return false;
    }

    bool MockDataSlotComponent::CanConvertToReference([[maybe_unused]] bool isNewSlot) const
    {
        return false;
    }

    bool MockDataSlotComponent::IsUserSlot() const
    {
        return false;
    }

    bool MockDataSlotComponent::ConvertToValue()
    {
        return false;
    }

    bool MockDataSlotComponent::CanConvertToValue() const
    {
        return false;
    }

    JE::Editor::Tools::GraphCanvas::DataSlotType MockDataSlotComponent::GetDataSlotType() const
    {
        return m_dataSlotConfiguration.m_dataSlotType;
    }

    JE::Editor::Tools::GraphCanvas::DataValueType MockDataSlotComponent::GetDataValueType() const
    {
        return m_dataSlotConfiguration.m_dataValueType;
    }

    AZ::Uuid MockDataSlotComponent::GetDataTypeId() const
    {
        return m_dataSlotConfiguration.m_typeId;
    }

    void MockDataSlotComponent::SetDataTypeId(AZ::Uuid typeId)
    {
        m_dataSlotConfiguration.m_typeId = typeId;
    }

    const JE::Editor::Tools::GraphCanvas::Styling::StyleHelper* MockDataSlotComponent::GetDataColorPalette() const
    {
        return nullptr;
    }

    size_t MockDataSlotComponent::GetContainedTypesCount() const
    {
        return m_dataSlotConfiguration.m_containerTypeIds.size();
    }

    AZ::Uuid MockDataSlotComponent::GetContainedTypeId(size_t index) const
    {
        return m_dataSlotConfiguration.m_containerTypeIds[index];
    }

    const JE::Editor::Tools::GraphCanvas::Styling::StyleHelper* MockDataSlotComponent::GetContainedTypeColorPalette([[maybe_unused]] size_t index) const
    {
        return nullptr;
    }

    void MockDataSlotComponent::SetDataAndContainedTypeIds(AZ::Uuid typeId, const AZStd::vector<AZ::Uuid>& typeIds, JE::Editor::Tools::GraphCanvas::DataValueType valueType)
    {
        AZ_UNUSED(typeId);
        AZ_UNUSED(typeIds);
        AZ_UNUSED(valueType);
    }

    // MockExecutionSlotComponent
    void MockExecutionSlotComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MockExecutionSlotComponent, MockSlotComponent>()
                ->Version(0)
                ;
        }
    }

    AZ::Entity* MockExecutionSlotComponent::CreateExecutionSlot(const AZ::EntityId& nodeId, const JE::Editor::Tools::GraphCanvas::SlotConfiguration& slotConfiguration)
    {
        AZ_UNUSED(nodeId);

        AZ::Entity* entity = MockSlotComponent::CreateCoreSlotEntity();

        MockExecutionSlotComponent* executionSlot = aznew MockExecutionSlotComponent(slotConfiguration);

        if (!entity->AddComponent(executionSlot))
        {
            delete executionSlot;
            delete entity;
            return nullptr;
        }

        return entity;
    }

    MockExecutionSlotComponent::MockExecutionSlotComponent()
        : MockSlotComponent(JE::Editor::Tools::GraphCanvas::SlotTypes::ExecutionSlot)
    {
    }

    MockExecutionSlotComponent::MockExecutionSlotComponent(const JE::Editor::Tools::GraphCanvas::SlotConfiguration& slotConfiguration)
        : MockSlotComponent(JE::Editor::Tools::GraphCanvas::SlotTypes::ExecutionSlot, slotConfiguration)
        , m_executionSlotConfiguration(slotConfiguration)
    {
    }

    // MockExtenderSlotComponent
    void MockExtenderSlotComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MockExtenderSlotComponent, MockSlotComponent>()
                ->Version(0)
                ;
        }
    }

    AZ::Entity* MockExtenderSlotComponent::CreateExtenderSlot(const AZ::EntityId& nodeId, const JE::Editor::Tools::GraphCanvas::ExtenderSlotConfiguration& slotConfiguration)
    {
        AZ_UNUSED(nodeId);

        AZ::Entity* entity = MockSlotComponent::CreateCoreSlotEntity();

        MockExtenderSlotComponent* extenderSlot = aznew MockExtenderSlotComponent(slotConfiguration);

        if (!entity->AddComponent(extenderSlot))
        {
            delete extenderSlot;
            delete entity;
            return nullptr;
        }

        return entity;
    }

    MockExtenderSlotComponent::MockExtenderSlotComponent()
        : MockSlotComponent(JE::Editor::Tools::GraphCanvas::SlotTypes::ExtenderSlot)
    {
    }

    MockExtenderSlotComponent::MockExtenderSlotComponent(const JE::Editor::Tools::GraphCanvas::ExtenderSlotConfiguration& slotConfiguration)
        : MockSlotComponent(JE::Editor::Tools::GraphCanvas::SlotTypes::ExtenderSlot, slotConfiguration)
        , m_extenderSlotConfiguration(slotConfiguration)
    {
    }

    void MockExtenderSlotComponent::Activate()
    {
        JE::Editor::Tools::GraphCanvas::ExtenderSlotRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MockExtenderSlotComponent::Deactivate()
    {
        JE::Editor::Tools::GraphCanvas::ExtenderSlotRequestBus::Handler::BusDisconnect();
    }

    void MockExtenderSlotComponent::TriggerExtension()
    {
    }

    JE::Editor::Tools::GraphCanvas::Endpoint MockExtenderSlotComponent::ExtendForConnectionProposal(const JE::Editor::Tools::GraphCanvas::ConnectionId& connectionId, const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint)
    {
        AZ_UNUSED(connectionId);
        AZ_UNUSED(endpoint);

        return JE::Editor::Tools::GraphCanvas::Endpoint();
    }

    // MockNodeComponent
    void MockNodeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MockNodeComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    AZ::Entity* MockNodeComponent::CreateCoreNodeEntity(const JE::Editor::Tools::GraphCanvas::NodeConfiguration& config)
    {
        // Create this Node's entity.
        AZ::Entity* entity = aznew AZ::Entity();

        entity->CreateComponent<MockNodeComponent>(config);

        return entity;
    }

    MockNodeComponent::MockNodeComponent(const JE::Editor::Tools::GraphCanvas::NodeConfiguration& config)
        : m_configuration(config)
    {
    }

    void MockNodeComponent::Activate()
    {
        JE::Editor::Tools::GraphCanvas::NodeRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MockNodeComponent::Deactivate()
    {
        JE::Editor::Tools::GraphCanvas::NodeRequestBus::Handler::BusDisconnect();
    }

    void MockNodeComponent::SetTooltip(const AZStd::string& tooltip)
    {
        m_configuration.SetTooltip(tooltip);
    }

    const AZStd::string MockNodeComponent::GetTooltip() const
    {
        return m_configuration.GetTooltip();
    }

    void MockNodeComponent::SetShowInOutliner(bool showInOutliner)
    {
        m_configuration.SetShowInOutliner(showInOutliner);
    }
    bool MockNodeComponent::ShowInOutliner() const
    {
        return m_configuration.GetShowInOutliner();
    }

    void MockNodeComponent::AddSlot(const AZ::EntityId& slotId)
    {
        AZ_Assert(slotId.IsValid(), "Slot entity (ID: %s) is not valid!", slotId.ToString().data());

        m_slotIds.emplace_back(slotId);
    }

    void MockNodeComponent::RemoveSlot(const AZ::EntityId& slotId)
    {
        AZ_Assert(slotId.IsValid(), "Slot (ID: %s) is not valid!", slotId.ToString().data());

        auto entry = AZStd::find(m_slotIds.begin(), m_slotIds.end(), slotId);
        AZ_Assert(entry != m_slotIds.end(), "Slot (ID: %s) is unknown", slotId.ToString().data());

        if (entry != m_slotIds.end())
        {
            m_slotIds.erase(entry);
        }
    }

    AZStd::vector<AZ::EntityId> MockNodeComponent::GetSlotIds() const
    {
        return m_slotIds;
    }

    AZStd::vector<JE::Editor::Tools::GraphCanvas::SlotId> MockNodeComponent::GetVisibleSlotIds() const
    {
        return m_slotIds;
    }

    AZStd::vector<JE::Editor::Tools::GraphCanvas::SlotId> MockNodeComponent::FindVisibleSlotIdsByType([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::ConnectionType& connectionType, [[maybe_unused]] const JE::Editor::Tools::GraphCanvas::SlotType& slotType) const
    {
        AZStd::vector<JE::Editor::Tools::GraphCanvas::SlotId> empty;
        return empty;
    }

    bool MockNodeComponent::HasConnections() const
    {
        bool hasConnections = false;

        for (auto slotId : m_slotIds)
        {
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(hasConnections, slotId, &JE::Editor::Tools::GraphCanvas::SlotRequests::HasConnections);

            if (hasConnections)
            {
                break;
            }
        }

        return hasConnections;
    }

    AZStd::any* MockNodeComponent::GetUserData()
    {
        return &m_userData;
    }

    bool MockNodeComponent::IsWrapped() const
    {
        return false;
    }

    void MockNodeComponent::SetWrappingNode([[maybe_unused]] const AZ::EntityId& wrappingNode)
    {
    }

    AZ::EntityId MockNodeComponent::GetWrappingNode() const
    {
        return AZ::EntityId();
    }

    void MockNodeComponent::SignalBatchedConnectionManipulationBegin()
    {
    }

    void MockNodeComponent::SignalBatchedConnectionManipulationEnd()
    {
    }

    JE::Editor::Tools::GraphCanvas::RootGraphicsItemEnabledState MockNodeComponent::UpdateEnabledState()
    {
        return JE::Editor::Tools::GraphCanvas::RootGraphicsItemEnabledState::ES_Enabled;
    }

    bool MockNodeComponent::IsHidingUnusedSlots() const
    {
        return false;
    }

    void MockNodeComponent::ShowAllSlots()
    {
    }

    void MockNodeComponent::HideUnusedSlots()
    {
    }

    bool MockNodeComponent::HasHideableSlots() const
    {
        return false;
    }

    void MockNodeComponent::SignalConnectionMoveBegin([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::ConnectionId& connectionId)
    {
    }
    
    void MockNodeComponent::SignalNodeAboutToBeDeleted()
    {
    }

    // MockGraphCanvasSystemComponent
    void MockGraphCanvasSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MockGraphCanvasSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void MockGraphCanvasSystemComponent::Activate()
    {
        JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::Handler::BusConnect();
    }
    void MockGraphCanvasSystemComponent::Deactivate()
    {
        JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::Handler::BusDisconnect();
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateBookmarkAnchor() const
    {
        return nullptr;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateScene() const
    {
        AZ::Entity* entity = aznew AZ::Entity("GraphCanvasScene");
        return entity;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateCoreNode() const
    {
        return nullptr;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateGeneralNode([[maybe_unused]] const char* nodeType) const
    {
        // Create this Node's entity.
        AZ::Entity* entity = MockNodeComponent::CreateCoreNodeEntity();

        return entity;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateCommentNode() const
    {
        return nullptr;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateWrapperNode([[maybe_unused]] const char* nodeType) const
    {
        return nullptr;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateNodeGroup() const
    {
        return nullptr;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateCollapsedNodeGroup([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::CollapsedNodeGroupConfiguration& groupedNodeConfiguration) const
    {
        return nullptr;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreateSlot(const AZ::EntityId& nodeId, const JE::Editor::Tools::GraphCanvas::SlotConfiguration& slotConfiguration) const
    {
        if (const JE::Editor::Tools::GraphCanvas::DataSlotConfiguration* dataSlotConfiguration = azrtti_cast<const JE::Editor::Tools::GraphCanvas::DataSlotConfiguration*>(&slotConfiguration))
        {
            return MockDataSlotComponent::CreateDataSlot((*dataSlotConfiguration));
        }
        else if (const JE::Editor::Tools::GraphCanvas::ExecutionSlotConfiguration* executionSlotConfiguration = azrtti_cast<const JE::Editor::Tools::GraphCanvas::ExecutionSlotConfiguration*>(&slotConfiguration))
        {
            return MockExecutionSlotComponent::CreateExecutionSlot(nodeId, (*executionSlotConfiguration));
        }
        else if (const JE::Editor::Tools::GraphCanvas::ExtenderSlotConfiguration* extenderSlotConfiguration = azrtti_cast<const JE::Editor::Tools::GraphCanvas::ExtenderSlotConfiguration*>(&slotConfiguration))
        {
            return MockExtenderSlotComponent::CreateExtenderSlot(nodeId, (*extenderSlotConfiguration));
        }
        else
        {
            AZ_Error("GraphCanvas", false, "Trying to create using an unknown Slot Configuration");
        }

        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* MockGraphCanvasSystemComponent::CreateBooleanNodePropertyDisplay([[maybe_unused]] JE::Editor::Tools::GraphCanvas::BooleanDataInterface* dataInterface) const
    {
        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* MockGraphCanvasSystemComponent::CreateNumericNodePropertyDisplay([[maybe_unused]] JE::Editor::Tools::GraphCanvas::NumericDataInterface* dataInterface) const
    {
        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* MockGraphCanvasSystemComponent::CreateComboBoxNodePropertyDisplay([[maybe_unused]] JE::Editor::Tools::GraphCanvas::ComboBoxDataInterface* dataInterface) const
    {
        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* MockGraphCanvasSystemComponent::CreateEntityIdNodePropertyDisplay([[maybe_unused]] JE::Editor::Tools::GraphCanvas::EntityIdDataInterface* dataInterface) const
    {
        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* MockGraphCanvasSystemComponent::CreateReadOnlyNodePropertyDisplay([[maybe_unused]] JE::Editor::Tools::GraphCanvas::ReadOnlyDataInterface* dataInterface) const
    {
        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* MockGraphCanvasSystemComponent::CreateStringNodePropertyDisplay([[maybe_unused]] JE::Editor::Tools::GraphCanvas::StringDataInterface* dataInterface) const
    {
        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* MockGraphCanvasSystemComponent::CreateVectorNodePropertyDisplay([[maybe_unused]] JE::Editor::Tools::GraphCanvas::VectorDataInterface* dataInterface) const
    {
        return nullptr;
    }

    JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* MockGraphCanvasSystemComponent::CreateAssetIdNodePropertyDisplay([[maybe_unused]] JE::Editor::Tools::GraphCanvas::AssetIdDataInterface* dataInterface) const
    {
        return nullptr;
    }

    AZ::Entity* MockGraphCanvasSystemComponent::CreatePropertySlot([[maybe_unused]] const AZ::EntityId& nodeId, [[maybe_unused]] const AZ::Crc32& propertyId, [[maybe_unused]] const JE::Editor::Tools::GraphCanvas::SlotConfiguration& slotConfiguration) const
    {
        return nullptr;
    }
}
