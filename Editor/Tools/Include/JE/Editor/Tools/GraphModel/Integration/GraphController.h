/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/std/containers/unordered_map.h>

// Qt
#include <QPixmap>

// Graph Canvas
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <JE/Editor/Tools/GraphModel/Model/Common.h>
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

class QGraphicsLinearLayout;

namespace JE::Editor::Tools::GraphModel::Integration
{
    class GraphCanvasMetadata;
    class ThumbnailItem;

    //! This is the main class for binding the node graph data to the UI provided by Graph Canvas
    class GraphController
        : private GraphCanvas::GraphModelRequestBus::Handler
        , private GraphCanvas::SceneNotificationBus::Handler
        , public GraphCanvas::EntitySaveDataRequestBus::Router
        , public GraphControllerRequestBus::Handler
    {
    public:
        AZ_RTTI(GraphController, "{E8433794-4BAE-4B63-B5A5-6EE69DFF0793}");

        GraphController(JE::Editor::Tools::GraphModel::GraphPtr graph, AZ::EntityId graphCanvasSceneId);
        ~GraphController() override;

        GraphController(const GraphController&) = delete;

        JE::Editor::Tools::GraphModel::GraphPtr GetGraph();
        const JE::Editor::Tools::GraphModel::GraphPtr GetGraph() const;
        const AZ::EntityId GetGraphCanvasSceneId() const;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphModel::GraphControllerRequestBus, connections

        //! Adds a node to the Graph and creates the corresponding Graph Canvas UI elements
        //! \param node  The node to add. This should be a freshly created Node that hasn't been added to the Graph yet.
        //! \param sceneDropPosition  The position in the Graph Cavnas scene where the Node was dropped
        GraphCanvas::NodeId AddNode(JE::Editor::Tools::GraphModel::NodePtr node, AZ::Vector2& sceneDropPosition) override;
        bool RemoveNode(JE::Editor::Tools::GraphModel::NodePtr node) override;
        AZ::Vector2 GetPosition(JE::Editor::Tools::GraphModel::NodePtr node) const override;

        void WrapNode(JE::Editor::Tools::GraphModel::NodePtr wrapperNode, JE::Editor::Tools::GraphModel::NodePtr node) override;
        void WrapNodeOrdered(JE::Editor::Tools::GraphModel::NodePtr wrapperNode, JE::Editor::Tools::GraphModel::NodePtr node, AZ::u32 layoutOrder) override;
        void UnwrapNode(JE::Editor::Tools::GraphModel::NodePtr wrapperNode, JE::Editor::Tools::GraphModel::NodePtr node) override;
        bool IsNodeWrapped(JE::Editor::Tools::GraphModel::NodePtr node) const override;
        void SetWrapperNodeActionString(JE::Editor::Tools::GraphModel::NodePtr node, const char* actionString) override;

        JE::Editor::Tools::GraphModel::ConnectionPtr AddConnection(JE::Editor::Tools::GraphModel::SlotPtr sourceSlot, JE::Editor::Tools::GraphModel::SlotPtr targetSlot) override;

        JE::Editor::Tools::GraphModel::ConnectionPtr AddConnectionBySlotId(
            JE::Editor::Tools::GraphModel::NodePtr sourceNode,
            const JE::Editor::Tools::GraphModel::SlotId& sourceSlotId,
            JE::Editor::Tools::GraphModel::NodePtr targetNode,
            const JE::Editor::Tools::GraphModel::SlotId& targetSlotId) override;

        bool AreSlotsConnected(
            JE::Editor::Tools::GraphModel::NodePtr sourceNode,
            const JE::Editor::Tools::GraphModel::SlotId& sourceSlotId,
            JE::Editor::Tools::GraphModel::NodePtr targetNode,
            const JE::Editor::Tools::GraphModel::SlotId& targetSlotId) const override;

        bool RemoveConnection(JE::Editor::Tools::GraphModel::ConnectionPtr connection) override;

        JE::Editor::Tools::GraphModel::SlotId ExtendSlot(JE::Editor::Tools::GraphModel::NodePtr node, const JE::Editor::Tools::GraphModel::SlotName& slotName) override;

        JE::Editor::Tools::GraphModel::NodePtr GetNodeById(const GraphCanvas::NodeId& nodeId) override;
        JE::Editor::Tools::GraphModel::NodePtrList GetNodesFromGraphNodeIds(const AZStd::vector<GraphCanvas::NodeId>& nodeIds) override;
        GraphCanvas::NodeId GetNodeIdByNode(JE::Editor::Tools::GraphModel::NodePtr node) const override;
        GraphCanvas::SlotId GetSlotIdBySlot(JE::Editor::Tools::GraphModel::SlotPtr slot) const override;

        JE::Editor::Tools::GraphModel::NodePtrList GetNodes() override;
        JE::Editor::Tools::GraphModel::NodePtrList GetSelectedNodes() override;
        void SetSelected(JE::Editor::Tools::GraphModel::NodePtrList nodes, bool selected) override;
        void ClearSelection() override;
        void EnableNode(JE::Editor::Tools::GraphModel::NodePtr node) override;
        void DisableNode(JE::Editor::Tools::GraphModel::NodePtr node) override;
        void CenterOnNodes(JE::Editor::Tools::GraphModel::NodePtrList nodes) override;
        AZ::Vector2 GetMajorPitch() const override;

        void SetThumbnailImageOnNode(JE::Editor::Tools::GraphModel::NodePtr node, const QPixmap& image) override;
        void SetThumbnailOnNode(JE::Editor::Tools::GraphModel::NodePtr node, ThumbnailItem* item) override;
        void RemoveThumbnailFromNode(JE::Editor::Tools::GraphModel::NodePtr node) override;

        ////////////////////////////////////////////////////////////////////////////////////

    private:
        //! Helper method for retrieving the UI layout for a given node
        QGraphicsLinearLayout* GetLayoutFromNode(JE::Editor::Tools::GraphModel::NodePtr node);

        //! Saves metadata for a Graph Canvas element into the Graph data model so it's ready
        //! to be serialized out with the data model. graphCanvasElement could be any number
        //! of entities including a node, comment, group, or the scene itself.
        void SaveMetadata(const AZ::EntityId& graphCanvasElement);

        //! Utility function for getting the GraphCanvasMetadata from the Graph data model
        GraphCanvasMetadata* GetGraphMetadata();

        ////////////////////////////////////////////////////////////////////////////////////
        // Functions for building Graph Canvas UI from our data model

        //! Creates the all Graph Canvas elements necessary for representing the Graph. This will be called once
        //! to instrument a Graph that was recently loaded.
        void CreateFullGraphUi();

        //! Creates the GraphCanvas slot UI representing a given Slot
        AZ::Entity* CreateSlotUi(JE::Editor::Tools::GraphModel::SlotPtr slot, AZ::EntityId nodeUiId);

        //! Creates the GraphCanvas node UI represeting a given Node
        //! \param scenePosition The node's position
        AZ::EntityId CreateNodeUi(JE::Editor::Tools::GraphModel::NodeId nodeId, JE::Editor::Tools::GraphModel::NodePtr node, const AZ::Vector2& scenePosition);

        //! Creates the GraphCanvas UI represeting a given Connection
        void CreateConnectionUi(JE::Editor::Tools::GraphModel::ConnectionPtr connection);

        //! Create a new GraphModel::Connection using the given source and target slots. This will also remove any existing connections on the target slot.
        JE::Editor::Tools::GraphModel::ConnectionPtr CreateConnection(JE::Editor::Tools::GraphModel::SlotPtr sourceSlot, JE::Editor::Tools::GraphModel::SlotPtr targetSlot);

        //! Check if creating a connection between the specified target and source node would
        //! cause a connection loopback.
        bool CheckForLoopback(JE::Editor::Tools::GraphModel::NodePtr sourceNode, JE::Editor::Tools::GraphModel::NodePtr targetNode) const;

        void WrapNodeUi(JE::Editor::Tools::GraphModel::NodePtr wrapperNode, JE::Editor::Tools::GraphModel::NodePtr node, AZ::u32 layoutOrder = JE::Editor::Tools::GraphModel::DefaultWrappedNodeLayoutOrder);
        void WrapNodeInternal(JE::Editor::Tools::GraphModel::NodePtr wrapperNode, JE::Editor::Tools::GraphModel::NodePtr node, AZ::u32 layoutOrder = JE::Editor::Tools::GraphModel::DefaultWrappedNodeLayoutOrder);

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::SceneNotificationBus, connections
        void OnNodeAdded(const AZ::EntityId& nodeUiId, bool isPaste) override;
        void OnNodeRemoved(const AZ::EntityId& nodeUiId) override;
        void OnConnectionAdded(const AZ::EntityId& connectionUiId) override;
        void OnConnectionRemoved(const AZ::EntityId& connectionUiId) override;
        void OnEntitiesSerialized(GraphCanvas::GraphSerialization& serializationTarget) override;
        void OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationSource) override;
        void OnEntitiesDeserializationComplete(const GraphCanvas::GraphSerialization& serializationSource) override;
        void OnNodeIsBeingEdited(bool isBeingEditeed) override;
        void OnSelectionChanged() override;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::EntitySaveDataRequestBus::Router
        void WriteSaveData(GraphCanvas::EntitySaveDataContainer& saveDataContainer) const override;
        void ReadSaveData(const GraphCanvas::EntitySaveDataContainer& saveDataContainer) override;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::GraphModelRequestBus, connections
        void DisconnectConnection([[maybe_unused]] const AZ::EntityId& connectionUiId) override {}
        bool CreateConnection(const AZ::EntityId& connectionUiId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) override;
        bool IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const override;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::GraphModelRequestBus, undo
        void RequestUndoPoint() override;
        void RequestPushPreventUndoStateUpdate() override;
        void RequestPopPreventUndoStateUpdate() override;
        void TriggerUndo() override;
        void TriggerRedo() override;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::GraphModelRequestBus, other
        void EnableNodes(const AZStd::unordered_set<GraphCanvas::NodeId>& nodeIds) override;
        void DisableNodes(const AZStd::unordered_set<GraphCanvas::NodeId>& nodeIds) override;

        AZStd::string GetDataTypeString(const AZ::Uuid& typeId) override;

        //! This is where we find all of the graph metadata (like node positions, comments, etc) and store it in the node graph for serialization
        void OnSaveDataDirtied(const AZ::EntityId& savedElement) override;

        void OnRemoveUnusedNodes() override {}
        void OnRemoveUnusedElements() override {}

        void ResetSlotToDefaultValue(const GraphCanvas::Endpoint& endpoint) override;

        /// Extendable slot handlers
        void RemoveSlot(const GraphCanvas::Endpoint& endpoint) override;
        bool IsSlotRemovable(const GraphCanvas::Endpoint& endpoint) const override;
        GraphCanvas::SlotId RequestExtension(
            const GraphCanvas::NodeId& nodeId,
            const GraphCanvas::ExtenderId& extenderId,
            GraphModelRequests::ExtensionRequestReason) override;

        bool ShouldWrapperAcceptDrop(const GraphCanvas::NodeId& wrapperNode, const QMimeData* mimeData) const override;
        void AddWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;
        void RemoveWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::GraphModelRequestBus, node properties

        //! Creates a GraphCanvas::NodePropertyDisplay and a data interface for editing input values
        GraphCanvas::NodePropertyDisplay* CreateDataSlotPropertyDisplay(
            const AZ::Uuid& dataType, const GraphCanvas::NodeId& nodeId, const GraphCanvas::SlotId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreatePropertySlotPropertyDisplay(
            const AZ::Crc32& propertyId, const GraphCanvas::NodeId& nodeUiId, const GraphCanvas::SlotId& slotUiId) const override;

        //! Common implementation for CreateDataSlotPropertyDisplay and CreatePropertySlotPropertyDisplay
        GraphCanvas::NodePropertyDisplay* CreateSlotPropertyDisplay(JE::Editor::Tools::GraphModel::SlotPtr inputSlot) const;

        ////////////////////////////////////////////////////////////////////////////////////

        
        //! This class maps the association between our data model's GraphElements and Graph Camvas's UI elements
        class GraphElementMap
        {
        public:
            //! Adds a 1:1 mapping between a Graph Canvas UI element and a GraphElement
            void Add(AZ::EntityId graphCanvasId, JE::Editor::Tools::GraphModel::GraphElementPtr graphElement);

            //! Removes the Graph Canvas EntityId and its associated GraphElement from the map
            void Remove(AZ::EntityId graphCanvasId);

            //! Removes the GraphElement and its associated Graph Canvas EntityId from the map
            void Remove(JE::Editor::Tools::GraphModel::ConstGraphElementPtr graphElement);

            //! Find the GraphElement that corresponds to the given Graph Canvas EntityId.
            //! Returns nullptr if the mapping doesn't exist.
            JE::Editor::Tools::GraphModel::GraphElementPtr Find(AZ::EntityId graphCanvasId);
            JE::Editor::Tools::GraphModel::ConstGraphElementPtr Find(AZ::EntityId graphCanvasId) const;

            //! Find the Graph Canvas EntityId that corresponds to the given GraphElement.
            //! Returns an invalid EntityId if the mapping doesn't exist.
            AZ::EntityId Find(JE::Editor::Tools::GraphModel::ConstGraphElementPtr graphElement) const;

        private:
            // shared_ptr can't be a key in a map so we use a raw pointer. This is fine because the two maps
            // are kept in sync, and the other map has a shared_ptr.
            typedef AZStd::unordered_map<const JE::Editor::Tools::GraphModel::GraphElement*, AZ::EntityId /*graphCanvasId*/> GraphElementToUiMap;
            GraphElementToUiMap m_graphElementToUi;

            typedef AZStd::unordered_map<AZ::EntityId /*graphCanvasId*/, JE::Editor::Tools::GraphModel::GraphElementPtr> UiToGraphElementMap;
            UiToGraphElementMap m_uiToGraphElement;
        };


        //! This class provides a collection of GraphElementMaps for the various types of elements. We could
        //! put all the elements in one GraphElementMap, but splitting them out makes debugging a lot easier.
        class GraphElementMapCollection
        {
        public:
            GraphElementMapCollection() = default;

            //! Adds a 1:1 mapping between a Graph Canvas UI element and a GraphElement.
            //! Automatically determines which GraphElementMap is appropriate.
            void Add(AZ::EntityId graphCanvasId, JE::Editor::Tools::GraphModel::GraphElementPtr graphElement);

            void Remove(AZ::EntityId graphCanvasId);
            void Remove(JE::Editor::Tools::GraphModel::ConstGraphElementPtr graphElement);


            //! Find the GraphElement that corresponds to the given Graph Canvas EntityId.
            //! Returns nullptr if the mapping doesn't exist, or the ElementType is wrong.
            template<typename ElementType>
            AZStd::shared_ptr<ElementType> Find(AZ::EntityId graphCanvasId);
            template<typename ElementType>
            AZStd::shared_ptr<const ElementType> Find(AZ::EntityId graphCanvasId) const;

            //! Find the Graph Canvas EntityId that corresponds to the given GraphElement.
            //! Returns an invalid EntityId if the mapping doesn't exist.
            AZ::EntityId Find(JE::Editor::Tools::GraphModel::ConstGraphElementPtr graphElement) const;

        private:

            GraphElementMap m_nodeMap;
            GraphElementMap m_slotMap;
            GraphElementMap m_connectionMap;

            //! Returns which is the right GraphElementMap for graphElement based on its type
            GraphElementMap* GetMapFor(JE::Editor::Tools::GraphModel::ConstGraphElementPtr graphElement);
            const GraphElementMap* GetMapFor(JE::Editor::Tools::GraphModel::ConstGraphElementPtr graphElement) const;

            // This list allows for easy iteration over all the GraphElementMaps.
            AZStd::vector<GraphElementMap*> m_allMaps = { &m_nodeMap, &m_slotMap, &m_connectionMap };
        } m_elementMap;


        AZ::SerializeContext* m_serializeContext = nullptr;
        JE::Editor::Tools::GraphModel::GraphPtr m_graph;
        AZ::EntityId m_graphCanvasSceneId;

        AZStd::unordered_map<JE::Editor::Tools::GraphModel::NodeId, JE::Editor::Tools::GraphModel::Integration::ThumbnailItem*> m_nodeThumbnails;
        AZStd::unordered_map<GraphCanvas::NodeId, AZStd::unordered_map<GraphCanvas::ExtenderId, JE::Editor::Tools::GraphModel::SlotName>> m_nodeExtenderIds;

        bool m_isCreatingConnectionUi = false;
        int m_preventUndoStateUpdateCount = 0;
    };
       

    template<typename ElementType>
    AZStd::shared_ptr<ElementType> GraphController::GraphElementMapCollection::Find(AZ::EntityId graphCanvasId)
    {
        JE::Editor::Tools::GraphModel::GraphElementPtr graphElement;

        for (GraphElementMap* map : m_allMaps)
        {
            graphElement = map->Find(graphCanvasId);
            if (graphElement)
            {
                break;
            }
        }

        return azrtti_cast<ElementType*>(graphElement);
    }


    template<typename ElementType>
    AZStd::shared_ptr<const ElementType> GraphController::GraphElementMapCollection::Find(AZ::EntityId graphCanvasId) const
    {
        JE::Editor::Tools::GraphModel::ConstGraphElementPtr graphElement;

        for (GraphElementMap* map : m_allMaps)
        {
            graphElement = map->Find(graphCanvasId);
            if (graphElement)
            {
                break;
            }
        }

        return azrtti_cast<const ElementType*>(graphElement);
    }
}
