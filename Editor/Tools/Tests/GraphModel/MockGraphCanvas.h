/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ ...
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

// Graph Canvas ...
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/EditorTypes.h>
#include <JE/Editor/Tools/GraphCanvas/GraphCanvasBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace MockGraphCanvasServices
{
    //! This mocks the GraphCanvas::SlotComponent component.
    //! This component is added to a SlotEntity that is created when a Slot is added to a Node.
    class MockSlotComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockSlotComponent, "{030690A4-6D16-4770-89B8-20A2EDF48D87}");
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Entity* CreateCoreSlotEntity();

        MockSlotComponent() = default;
        explicit MockSlotComponent(const JE::Editor::Tools::GraphCanvas::SlotType& slotType);

        MockSlotComponent(const JE::Editor::Tools::GraphCanvas::SlotType& slotType, const JE::Editor::Tools::GraphCanvas::SlotConfiguration& configuration);
        ~MockSlotComponent() override = default;

        // Component overrides ...
        void Activate() override;
        void Deactivate() override;

    protected:
        JE::Editor::Tools::GraphCanvas::SlotType m_slotType;
        JE::Editor::Tools::GraphCanvas::SlotConfiguration m_slotConfiguration;
    };

    //! This mocks the GraphCanvas::DataSlotComponent component.
    //! This component is the specific instance of a SlotComponent that is
    //! added to a SlotEntity when a DataSlot is added to a Node.
    //! Implements the GraphCanvas::DataSlotRequestBus for tests which involve data slots.
    class MockDataSlotComponent
        : public MockSlotComponent
        , public JE::Editor::Tools::GraphCanvas::DataSlotRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockDataSlotComponent, "{0E2E8F38-3B7B-427D-ABD6-38C68FDEFE88}", MockSlotComponent);
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Entity* CreateDataSlot(const JE::Editor::Tools::GraphCanvas::DataSlotConfiguration& dataSlotConfiguration);

        MockDataSlotComponent();
        MockDataSlotComponent(const JE::Editor::Tools::GraphCanvas::DataSlotConfiguration& dataSlotConfiguration);
        ~MockDataSlotComponent() = default;

        // Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // GraphCanvas::DataSlotRequestBus overrides ...
        bool ConvertToReference(bool isNewSlot = false) override;
        bool CanConvertToReference(bool isNewSlot = false) const override;
        bool ConvertToValue() override;
        bool CanConvertToValue() const override;
        bool IsUserSlot() const override;

        JE::Editor::Tools::GraphCanvas::DataSlotType GetDataSlotType() const override;
        JE::Editor::Tools::GraphCanvas::DataValueType GetDataValueType() const override;
        AZ::Uuid GetDataTypeId() const override;
        void SetDataTypeId(AZ::Uuid typeId) override;
        const JE::Editor::Tools::GraphCanvas::Styling::StyleHelper* GetDataColorPalette() const override;
        size_t GetContainedTypesCount() const override;
        AZ::Uuid GetContainedTypeId(size_t index) const override;
        const JE::Editor::Tools::GraphCanvas::Styling::StyleHelper* GetContainedTypeColorPalette(size_t index) const override;
        void SetDataAndContainedTypeIds(AZ::Uuid typeId, const AZStd::vector<AZ::Uuid>& typeIds, JE::Editor::Tools::GraphCanvas::DataValueType valueType) override;

    private:
        MockDataSlotComponent(const MockDataSlotComponent&) = delete;
        MockDataSlotComponent& operator=(const MockDataSlotComponent&) = delete;

        JE::Editor::Tools::GraphCanvas::DataSlotConfiguration m_dataSlotConfiguration;
    };

    //! This mocks the GraphCanvas::ExecutionSlotComponent component.
    //! This component is the specific instance of a SlotComponent that is
    //! added to a SlotEntity when an ExecutionSlot is added to a Node.
    class MockExecutionSlotComponent
        : public MockSlotComponent
    {
    public:
        AZ_COMPONENT(MockExecutionSlotComponent, "{3E12451C-65EB-45A6-AC98-437F06021359}", MockSlotComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        static AZ::Entity* CreateExecutionSlot(const AZ::EntityId& nodeId, const JE::Editor::Tools::GraphCanvas::SlotConfiguration& slotConfiguration);

        MockExecutionSlotComponent();
        explicit MockExecutionSlotComponent(const JE::Editor::Tools::GraphCanvas::SlotConfiguration& slotConfiguration);
        ~MockExecutionSlotComponent() = default;

    protected:
        MockExecutionSlotComponent(const MockExecutionSlotComponent&) = delete;
        MockExecutionSlotComponent& operator=(const MockExecutionSlotComponent&) = delete;

        JE::Editor::Tools::GraphCanvas::SlotConfiguration m_executionSlotConfiguration;
    };

    //! This mocks the GraphCanvas::ExtenderSlotComponent component.
    //! This component is the specific instance of a SlotComponent that is
    //! added to a SlotEntity when an ExtenderSlot is added to a Node.
    //! Implements the GraphCanvas::ExtenderSlotRequestBus for tests which involve extender slots.
    class MockExtenderSlotComponent
        : public MockSlotComponent
        , public JE::Editor::Tools::GraphCanvas::ExtenderSlotRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockExtenderSlotComponent, "{0CAE942E-5E4E-42EC-8F63-809A4DE317C0}", MockSlotComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        static AZ::Entity* CreateExtenderSlot(const AZ::EntityId& nodeId, const JE::Editor::Tools::GraphCanvas::ExtenderSlotConfiguration& slotConfiguration);

        MockExtenderSlotComponent();
        explicit MockExtenderSlotComponent(const JE::Editor::Tools::GraphCanvas::ExtenderSlotConfiguration& slotConfiguration);
        ~MockExtenderSlotComponent() = default;

        // Component overrides ...
        void Activate() override;
        void Deactivate() override;
        ////

        // ExtenderSlotComponent overrides ...
        void TriggerExtension() override;
        JE::Editor::Tools::GraphCanvas::Endpoint ExtendForConnectionProposal(const JE::Editor::Tools::GraphCanvas::ConnectionId& connectionId, const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint) override;

    protected:
        MockExtenderSlotComponent(const MockExtenderSlotComponent&) = delete;
        MockExtenderSlotComponent& operator=(const MockExtenderSlotComponent&) = delete;

        JE::Editor::Tools::GraphCanvas::ExtenderSlotConfiguration m_extenderSlotConfiguration;
    };

    //! This mocks the GraphCanvas::NodeComponent component.
    //! This component is added to a Node entity when a Node is added to the graph.
    //! Implements the GraphCanvas::NodeRequestBus for tests that invole nodes.
    class MockNodeComponent
        : public AZ::Component
        , public JE::Editor::Tools::GraphCanvas::NodeRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockNodeComponent, "{886E7216-FD58-442B-AF1E-1AC7174885F8}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context);

        static AZ::Entity* CreateCoreNodeEntity(const JE::Editor::Tools::GraphCanvas::NodeConfiguration& config = JE::Editor::Tools::GraphCanvas::NodeConfiguration());

        MockNodeComponent() = default;
        MockNodeComponent(const JE::Editor::Tools::GraphCanvas::NodeConfiguration& config);
        ~MockNodeComponent() override = default;

        // Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // GraphCanvas::NodeRequestBus overrides ...
        void SetTooltip(const AZStd::string& tooltip) override;
        const AZStd::string GetTooltip() const override;
        void SetShowInOutliner(bool showInOutliner) override;
        bool ShowInOutliner() const override;
        void AddSlot(const AZ::EntityId& slotId) override;
        void RemoveSlot(const AZ::EntityId& slotId) override;
        AZStd::vector<AZ::EntityId> GetSlotIds() const override;
        AZStd::vector<JE::Editor::Tools::GraphCanvas::SlotId> GetVisibleSlotIds() const override;
        AZStd::vector<JE::Editor::Tools::GraphCanvas::SlotId> FindVisibleSlotIdsByType(const JE::Editor::Tools::GraphCanvas::ConnectionType& connectionType, const JE::Editor::Tools::GraphCanvas::SlotType& slotType) const override;
        bool HasConnections() const override;
        AZStd::any* GetUserData() override;
        bool IsWrapped() const override;
        void SetWrappingNode(const AZ::EntityId& wrappingNode) override;
        AZ::EntityId GetWrappingNode() const override;
        void SignalBatchedConnectionManipulationBegin() override;
        void SignalBatchedConnectionManipulationEnd() override;
        JE::Editor::Tools::GraphCanvas::RootGraphicsItemEnabledState UpdateEnabledState() override;
        bool IsHidingUnusedSlots() const override;
        void ShowAllSlots() override;
        void HideUnusedSlots() override;
        bool HasHideableSlots() const override;
        void SignalConnectionMoveBegin(const JE::Editor::Tools::GraphCanvas::ConnectionId& connectionId) override;
        void SignalNodeAboutToBeDeleted() override;

    protected:
        /// This node's slots
        AZStd::vector<AZ::EntityId> m_slotIds;

        /// Serialized configuration settings
        JE::Editor::Tools::GraphCanvas::NodeConfiguration m_configuration;

        /// Stores custom user data for this node
        AZStd::any m_userData;
    };

    //! This mocks the GraphCanvas::GraphCanvasSystemComponent component.
    //! This component is created and added to the system entity created in our GraphModelIntegrationTest::TestEnvironment
    //! because this component implements the GraphCanvas::GraphCanvasRequestBus that is
    //! the entry point bus for performing basic GraphCanvas operations such as creating
    //! a new scene, creating nodes, creating slots, etc...
    class MockGraphCanvasSystemComponent
        : public AZ::Component
        , private JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockGraphCanvasSystemComponent, "{03D5474F-5FF3-4D7B-B578-2C3EC132E921}");
        static void Reflect(AZ::ReflectContext* context);

        MockGraphCanvasSystemComponent() = default;
        ~MockGraphCanvasSystemComponent() override = default;

    private:
        // Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // GraphCanvas::GraphCanvasRequestBus overrides ...
        AZ::Entity* CreateBookmarkAnchor() const override;
        AZ::Entity* CreateScene() const override;
        AZ::Entity* CreateCoreNode() const override;
        AZ::Entity* CreateGeneralNode(const char* nodeType) const override;
        AZ::Entity* CreateCommentNode() const override;
        AZ::Entity* CreateWrapperNode(const char* nodeType) const override;
        AZ::Entity* CreateNodeGroup() const override;
        AZ::Entity* CreateCollapsedNodeGroup(const JE::Editor::Tools::GraphCanvas::CollapsedNodeGroupConfiguration& groupedNodeConfiguration) const override;
        AZ::Entity* CreateSlot(const AZ::EntityId& nodeId, const JE::Editor::Tools::GraphCanvas::SlotConfiguration& slotConfiguration) const override;
        JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* CreateBooleanNodePropertyDisplay(JE::Editor::Tools::GraphCanvas::BooleanDataInterface* dataInterface) const override;
        JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* CreateNumericNodePropertyDisplay(JE::Editor::Tools::GraphCanvas::NumericDataInterface* dataInterface) const override;
        JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* CreateComboBoxNodePropertyDisplay(JE::Editor::Tools::GraphCanvas::ComboBoxDataInterface* dataInterface) const override;
        JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* CreateEntityIdNodePropertyDisplay(JE::Editor::Tools::GraphCanvas::EntityIdDataInterface* dataInterface) const override;
        JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* CreateReadOnlyNodePropertyDisplay(JE::Editor::Tools::GraphCanvas::ReadOnlyDataInterface* dataInterface) const override;
        JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* CreateStringNodePropertyDisplay(JE::Editor::Tools::GraphCanvas::StringDataInterface* dataInterface) const override;
        JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* CreateVectorNodePropertyDisplay(JE::Editor::Tools::GraphCanvas::VectorDataInterface* dataInterface) const override;
        JE::Editor::Tools::GraphCanvas::NodePropertyDisplay* CreateAssetIdNodePropertyDisplay(JE::Editor::Tools::GraphCanvas::AssetIdDataInterface* dataInterface) const override;
        AZ::Entity* CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const JE::Editor::Tools::GraphCanvas::SlotConfiguration& slotConfiguration) const override;
    };
}
