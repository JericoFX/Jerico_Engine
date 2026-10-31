/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

// Graph Canvas
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/EditorTypes.h>
#include <JE/Editor/Tools/GraphCanvas/GraphCanvasBus.h>

// GraphModel
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>

// GraphModel test harness and minimum setup for a custom GraphModel instance
#include <GraphModel/TestEnvironment.h>

// These tests rely on this test environment to setup the necessary
// mocked GraphCanvas buses and system components
AZ_UNIT_TEST_HOOK(new GraphModelIntegrationTest::GraphModelTestEnvironment);

namespace GraphModelIntegrationTest
{
    /**
    *   Used for testing the GraphModelIntegration namespace, which relies on GraphCanvasWidgets and Qt.
    *   NOTE: These tests rely on being run in the GraphModelTestEnvironment, which sets up the
    *   necessary mocked GraphCanvas buses and system components.
    */
    class GraphModelIntegrationTests
        : public ::testing::Test
        , public UnitTest::TraceBusRedirector
    {
    protected:
        void SetUp() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();

            // Create our test graph context
            m_graphContext = AZStd::make_shared<TestGraphContext>();

            // Create a new node graph
            m_graph = AZStd::make_shared<JE::Editor::Tools::GraphModel::Graph>(m_graphContext);

            // Create a new scene for the graph
            AZ::Entity* scene = nullptr;
            JE::Editor::Tools::GraphModel::Integration::GraphManagerRequestBus::BroadcastResult(scene, &JE::Editor::Tools::GraphModel::Integration::GraphManagerRequests::CreateScene, m_graph, NODE_GRAPH_TEST_EDITOR_ID);
            m_scene.reset(scene);
            m_sceneId = m_scene->GetId();
        }

        void TearDown() override
        {
            // Release shared pointers that were setup
            m_graphContext.reset();
            m_graph.reset();
            m_scene.reset();

            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        AZStd::shared_ptr<TestGraphContext> m_graphContext = nullptr;
        JE::Editor::Tools::GraphModel::GraphPtr m_graph = nullptr;
        AZStd::unique_ptr<AZ::Entity> m_scene = nullptr;

        AZ::EntityId m_sceneId;
    };

    TEST_F(GraphModelIntegrationTests, NodeAddedToScene)
    {
        // Create our test node
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // Make sure the correct node was added to the scene
        JE::Editor::Tools::GraphModel::NodePtrList nodeList;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeList, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodes);
        EXPECT_EQ(nodeList.size(), 1);
        EXPECT_EQ(nodeList[0], testNode);
    }

    /**
    *   Make sure the data type of the slot on a node is set properly.  There was a new DataSlotConfiguration added
    *   in GraphCanvas, which the GraphModel implementation hadn't previously accounted for, resulting in all data slots on
    *   GraphModel nodes having an invalid data type.
    */
    TEST_F(GraphModelIntegrationTests, NodeWithDataSlotHasProperDataType)
    {
        // Create our test node
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // Retrieve the data type (string) for the string input slot on our test node
        JE::Editor::Tools::GraphModel::SlotPtr testNodeStringInputSlot = testNode->GetSlot(TEST_STRING_INPUT_ID);
        EXPECT_TRUE(testNodeStringInputSlot != nullptr);
        JE::Editor::Tools::GraphModel::DataTypePtr stringDataType = testNodeStringInputSlot->GetDataType();
        AZ::Uuid stringDataTypeId = stringDataType->GetTypeUuid();

        // Make sure our node has the expected slots
        AZStd::vector<AZ::EntityId> slotIds;
        JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(slotIds, nodeId, &JE::Editor::Tools::GraphCanvas::NodeRequests::GetSlotIds);
        EXPECT_EQ(slotIds.size(), 4);

        // Make sure the data type of the input string slot on our test node matches the expected data type
        AZ::EntityId slotId = slotIds[0];
        AZ::Uuid slotDataTypeId;
        JE::Editor::Tools::GraphCanvas::DataSlotRequestBus::EventResult(slotDataTypeId, slotId, &JE::Editor::Tools::GraphCanvas::DataSlotRequests::GetDataTypeId);
        EXPECT_EQ(stringDataTypeId, slotDataTypeId);
    }

    TEST_F(GraphModelIntegrationTests, GetNodeById)
    {
        // Create our test node
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // Test that we can retrieve the expected node by NodeId
        JE::Editor::Tools::GraphModel::NodePtr retrievedNode = nullptr;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(retrievedNode, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeById, nodeId);
        EXPECT_EQ(retrievedNode, testNode);

        // Test requesting an invalid NodeId returns nullptr
        retrievedNode = nullptr;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(retrievedNode, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeById, JE::Editor::Tools::GraphCanvas::NodeId());
        EXPECT_EQ(retrievedNode, nullptr);

        // Test requesting a valid NodeId but one that doesn't exist in the scene returns nullptr
        retrievedNode = nullptr;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(retrievedNode, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeById, JE::Editor::Tools::GraphCanvas::NodeId(1234));
        EXPECT_EQ(retrievedNode, nullptr);
    }

    TEST_F(GraphModelIntegrationTests, GetNodesFromGraphNodeIds)
    {
        // Create a test node
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // Retrieve Nodes by their NodeId
        AZStd::vector<JE::Editor::Tools::GraphCanvas::NodeId> nodeIds = {
            nodeId, // Valid NodeId for a node in the scene
            JE::Editor::Tools::GraphCanvas::NodeId(), // Invalid NodeId
            JE::Editor::Tools::GraphCanvas::NodeId(1234) // Valid NodeId but not in the scene
        };
        JE::Editor::Tools::GraphModel::NodePtrList retrievedNodes;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(retrievedNodes, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodesFromGraphNodeIds, nodeIds);
        // Test that only one node was found.
        EXPECT_EQ(retrievedNodes.size(), 1);

        // Test the first node in the list should be our valid test node
        EXPECT_EQ(retrievedNodes[0], testNode);
    }

    TEST_F(GraphModelIntegrationTests, ExtendableSlotsWithDifferentMinimumValues)
    {
        // Create a node with extendable slots
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<ExtendableSlotsNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // The input string extendable slot has a minimum of 0 slots, so there should be none
        auto extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        int numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 0);
        EXPECT_EQ(numExtendableSlots, 0);

        // The output string and input event extendable slots both use the default minimum (1)
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);
        extendableSlots = testNode->GetExtendableSlots(TEST_EVENT_INPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_EVENT_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);

        // The output event extendable slot has a minimum of 3 slots
        extendableSlots = testNode->GetExtendableSlots(TEST_EVENT_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 3);
    }

    TEST_F(GraphModelIntegrationTests, AddingExtendableSlotsPastMaximum)
    {
        // Create a node with extendable slots
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<ExtendableSlotsNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // The input string extendable slot has a minimum of 0 slots, so it starts with 0
        auto extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        int numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 0);
        EXPECT_EQ(numExtendableSlots, 0);

        // The input string extendable slot has a maximum of 2 slots, so the first add should succeed
        JE::Editor::Tools::GraphModel::SlotPtr firstSlot = testNode->AddExtendedSlot(TEST_STRING_INPUT_ID);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_TRUE(firstSlot != nullptr);
        EXPECT_EQ(firstSlot->GetName(), TEST_STRING_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);

        // The input string extendable slot has a maximum of 2 slots, so the second add should also succeed
        JE::Editor::Tools::GraphModel::SlotPtr secondSlot = testNode->AddExtendedSlot(TEST_STRING_INPUT_ID);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_TRUE(secondSlot != nullptr);
        EXPECT_EQ(secondSlot->GetName(), TEST_STRING_INPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 2);
        EXPECT_EQ(numExtendableSlots, 2);

        // The input string extendable slot has a maximum of 2 slots, so the third add should fail
        JE::Editor::Tools::GraphModel::SlotPtr thirdSlot = testNode->AddExtendedSlot(TEST_STRING_INPUT_ID);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_INPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_INPUT_ID);
        EXPECT_TRUE(thirdSlot == nullptr);
        EXPECT_EQ(extendableSlots.size(), 2);
        EXPECT_EQ(numExtendableSlots, 2);
    }

    TEST_F(GraphModelIntegrationTests, RemovingExtendableSlotsBelowMinimum)
    {
        // Create a node with extendable slots
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<ExtendableSlotsNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // The output string extendable slot has a minimum of 1 slot, so it starts with 1
        auto extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        int numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);

        // The output string extendable slot has the default maximum (100), so we can add one
        JE::Editor::Tools::GraphModel::SlotPtr firstSlot = testNode->AddExtendedSlot(TEST_STRING_OUTPUT_ID);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_TRUE(firstSlot != nullptr);
        EXPECT_EQ(firstSlot->GetName(), TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 2);
        EXPECT_EQ(numExtendableSlots, 2);

        // The output string extednable slot has a minimum of 1, so we can remove 1
        testNode->DeleteSlot(firstSlot);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);

        // The output string extednable slot has a minimum of 1, so attempting to remove one
        // when there is only one left will fail
        JE::Editor::Tools::GraphModel::SlotPtr lastSlot = *extendableSlots.begin();
        testNode->DeleteSlot(lastSlot);
        extendableSlots = testNode->GetExtendableSlots(TEST_STRING_OUTPUT_ID);
        numExtendableSlots = testNode->GetExtendableSlotCount(TEST_STRING_OUTPUT_ID);
        EXPECT_EQ(extendableSlots.size(), 1);
        EXPECT_EQ(numExtendableSlots, 1);
    }

    TEST_F(GraphModelIntegrationTests, CannotAddNonExtendableSlot)
    {
        // Create a test node
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // Can't add a non-extendable slot, so adding a non-extendable slot will fail
        AZ_TEST_START_TRACE_SUPPRESSION;
        JE::Editor::Tools::GraphModel::SlotPtr newSlot = testNode->AddExtendedSlot(TEST_STRING_INPUT_ID);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(newSlot, nullptr);
    }

    TEST_F(GraphModelIntegrationTests, CannotDeleteNonExtendableSlot)
    {
        // Create a test node
        JE::Editor::Tools::GraphModel::NodePtr testNode = AZStd::make_shared<TestNode>(m_graph, m_graphContext);
        EXPECT_TRUE(testNode != nullptr);

        // Add our test node to the scene
        AZ::Vector2 offset;
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_sceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, testNode, offset);

        // Can't delete a non-extendable slot, so deleting a non-extendable slot will fail
        auto beforeSlots = testNode->GetSlots();
        JE::Editor::Tools::GraphModel::SlotPtr inputSlot = testNode->GetSlot(TEST_STRING_INPUT_ID);
        testNode->DeleteSlot(inputSlot);
        auto afterSlots = testNode->GetSlots();
        EXPECT_EQ(beforeSlots.size(), afterSlots.size());
    }

    TEST_F(GraphModelIntegrationTests, ParentNodeCacheDoesNotKeepTheOwnerAlive)
    {
        JE::Editor::Tools::GraphModel::GraphPtr graph = AZStd::make_shared<JE::Editor::Tools::GraphModel::Graph>(m_graphContext);
        JE::Editor::Tools::GraphModel::NodePtr node = AZStd::make_shared<TestNode>(graph, m_graphContext);
        graph->AddNode(node);
        JE::Editor::Tools::GraphModel::SlotPtr retainedSlot = node->GetSlot(TEST_STRING_INPUT_ID);
        ASSERT_NE(retainedSlot, nullptr);

        EXPECT_EQ(retainedSlot->GetParentNode(), node);
        EXPECT_EQ(retainedSlot->GetParentNode(), node);
        retainedSlot->ClearCachedData();
        EXPECT_EQ(retainedSlot->GetParentNode(), node);
        EXPECT_EQ(retainedSlot->GetParentNode(), node);

        AZStd::weak_ptr<JE::Editor::Tools::GraphModel::Node> weakNode = node;
        AZStd::weak_ptr<JE::Editor::Tools::GraphModel::Graph> weakGraph = graph;
        node.reset();
        graph.reset();

        EXPECT_TRUE(weakNode.expired());
        EXPECT_EQ(retainedSlot->GetParentNode(), nullptr);
        EXPECT_TRUE(weakGraph.expired());

        // Cleanup after the ownership oracles breaks the RED implementation's cycle, so it cannot leak into other tests.
        // Passing behavior does not need this cleanup; do not query the dirty cache after its graph has been destroyed.
        retainedSlot->ClearCachedData();
    }

    TEST_F(GraphModelIntegrationTests, RemovingTheParentNodeInvalidatesTheCachedLookup)
    {
        JE::Editor::Tools::GraphModel::GraphPtr graph = AZStd::make_shared<JE::Editor::Tools::GraphModel::Graph>(m_graphContext);
        JE::Editor::Tools::GraphModel::NodePtr node = AZStd::make_shared<TestNode>(graph, m_graphContext);
        graph->AddNode(node);
        JE::Editor::Tools::GraphModel::SlotPtr retainedSlot = node->GetSlot(TEST_STRING_INPUT_ID);
        ASSERT_NE(retainedSlot, nullptr);
        EXPECT_EQ(retainedSlot->GetParentNode(), node);
        EXPECT_EQ(retainedSlot->GetParentNode(), node);

        EXPECT_TRUE(graph->RemoveNode(node));
        EXPECT_EQ(retainedSlot->GetParentNode(), nullptr);
        EXPECT_EQ(retainedSlot->GetParentNode(), nullptr);
        AZStd::weak_ptr<JE::Editor::Tools::GraphModel::Node> weakNode = node;
        node.reset();
        EXPECT_TRUE(weakNode.expired());
    }
}
