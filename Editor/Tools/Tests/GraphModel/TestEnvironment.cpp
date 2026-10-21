/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Task/TaskGraphSystemComponent.h>
#include <GraphModel/TestEnvironment.h>
#include <GraphModel/GraphModelSystemComponent.h>

namespace GraphModelIntegrationTest
{
    // TestGraphContext
    TestGraphContext::TestGraphContext()
        : JE::Editor::Tools::GraphModel::GraphContext("GraphModelIntegrationTest", ".nodeTest", {})
    {
        // Construct basic data types
        const AZ::Uuid stringTypeUuid = azrtti_typeid<AZStd::string>();
        const AZ::Uuid entityIdTypeUuid = azrtti_typeid<AZ::EntityId>();
        m_dataTypes.push_back(AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(TestDataTypeEnum::TestDataTypeEnum_String, stringTypeUuid, AZStd::any(AZStd::string("")), "String", "AZStd::string"));
        m_dataTypes.push_back(AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(TestDataTypeEnum::TestDataTypeEnum_EntityId, entityIdTypeUuid, AZStd::any(AZ::EntityId()), "EntityId", "AZ::EntityId"));
    }

    // TestNode
    void TestNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TestNode, JE::Editor::Tools::GraphModel::Node>()
                ->Version(0)
                ;
        }
    }

    TestNode::TestNode(JE::Editor::Tools::GraphModel::GraphPtr graph, AZStd::shared_ptr<TestGraphContext> graphContext)
        : JE::Editor::Tools::GraphModel::Node(graph)
        , m_graphContext(graphContext)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* TestNode::GetTitle() const
    {
        return "TestNode";
    }

    void TestNode::RegisterSlots()
    {
        JE::Editor::Tools::GraphModel::DataTypePtr stringDataType = m_graphContext->GetDataType(TestDataTypeEnum::TestDataTypeEnum_String);

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Input,
            JE::Editor::Tools::GraphModel::SlotType::Data,
            TEST_STRING_INPUT_ID,
            "Test Input",
            "A test input slot for String data type",
            JE::Editor::Tools::GraphModel::DataTypeList{ stringDataType },
            stringDataType->GetDefaultValue()));

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Output,
            JE::Editor::Tools::GraphModel::SlotType::Data,
            TEST_STRING_OUTPUT_ID,
            "Test Output",
            "A test output slot for String data type",
            JE::Editor::Tools::GraphModel::DataTypeList{ stringDataType } ));

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Input,
            JE::Editor::Tools::GraphModel::SlotType::Event,
            TEST_EVENT_INPUT_ID,
            "Event In",
            "A test input event slot"));

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Output,
            JE::Editor::Tools::GraphModel::SlotType::Event,
            TEST_EVENT_OUTPUT_ID,
            "Event Out",
            "A test output event slot"));
    }

    // ExtendableSlotsNode
    void ExtendableSlotsNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ExtendableSlotsNode, JE::Editor::Tools::GraphModel::Node>()
                ->Version(0)
                ;
        }
    }

    ExtendableSlotsNode::ExtendableSlotsNode(JE::Editor::Tools::GraphModel::GraphPtr graph, AZStd::shared_ptr<TestGraphContext> graphContext)
        : JE::Editor::Tools::GraphModel::Node(graph)
        , m_graphContext(graphContext)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* ExtendableSlotsNode::GetTitle() const
    {
        return "ExtendableSlotsNode";
    }

    void ExtendableSlotsNode::RegisterSlots()
    {
        JE::Editor::Tools::GraphModel::DataTypePtr stringDataType = m_graphContext->GetDataType(TestDataTypeEnum::TestDataTypeEnum_String);

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Input,
            JE::Editor::Tools::GraphModel::SlotType::Data,
            TEST_STRING_INPUT_ID,
            "Test Input",
            "An extendable input slot for String data type",
            JE::Editor::Tools::GraphModel::DataTypeList{ stringDataType },
            stringDataType->GetDefaultValue(),
            0,
            2,
            "Add String Input",
            "Add a test string input"));

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Output,
            JE::Editor::Tools::GraphModel::SlotType::Data,
            TEST_STRING_OUTPUT_ID,
            "Test Output",
            "An extendable output slot for String data type",
            JE::Editor::Tools::GraphModel::DataTypeList{ stringDataType },
            AZStd::any{},
            1,
            100,
            "Add String Output",
            "Add a test string output"));

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Input,
            JE::Editor::Tools::GraphModel::SlotType::Event,
            TEST_EVENT_INPUT_ID,
            "Test Input Event",
            "An extendable input event",
            JE::Editor::Tools::GraphModel::DataTypeList{},
            AZStd::any{},
            1,
            100,
            "Add Input Event",
            "Add a test event input"));

        RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
            JE::Editor::Tools::GraphModel::SlotDirection::Output,
            JE::Editor::Tools::GraphModel::SlotType::Event,
            TEST_EVENT_OUTPUT_ID,
            "Test Output Event",
            "An extendable output event",
            JE::Editor::Tools::GraphModel::DataTypeList{},
            AZStd::any{},
            3,
            4,
            "Add Output Event",
            "Add a test event output"));
    }

    // GraphModelTestEnvironment
    void GraphModelTestEnvironment::SetupEnvironment()
    {
        // Create application and descriptor
        m_application = aznew AZ::ComponentApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Create basic system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        m_systemEntity->AddComponent(aznew AZ::AssetManagerComponent());
        m_systemEntity->AddComponent(aznew AZ::JobManagerComponent());
        m_systemEntity->AddComponent(aznew AZ::TaskGraphSystemComponent());
        m_systemEntity->AddComponent(aznew AZ::StreamerComponent());
        m_systemEntity->AddComponent(aznew JE::Editor::Tools::GraphModel::GraphModelSystemComponent());

        // Register descriptor for the GraphModelSystemComponent
        m_application->RegisterComponentDescriptor(JE::Editor::Tools::GraphModel::GraphModelSystemComponent::CreateDescriptor());

        // Register descriptors for our mock components
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockNodeComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockSlotComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockDataSlotComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockExecutionSlotComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockExtenderSlotComponent::CreateDescriptor());
        m_application->RegisterComponentDescriptor(MockGraphCanvasServices::MockGraphCanvasSystemComponent::CreateDescriptor());

        // Register our mock GraphCanvasSystemComponent
        m_systemEntity->AddComponent(aznew MockGraphCanvasServices::MockGraphCanvasSystemComponent());

        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void GraphModelTestEnvironment::TeardownEnvironment()
    {
        delete m_application;

        AZ::AllocatorManager::Instance().GarbageCollect();
    }
}
