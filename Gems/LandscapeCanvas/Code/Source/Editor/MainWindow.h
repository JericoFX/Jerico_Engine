/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CrySystemBus.h>

// AZ
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

// GraphModel
#include <JE/Editor/Tools/GraphModel/Integration/EditorMainWindow.h>

// LandscapeCanvas
#include <LandscapeCanvas/LandscapeCanvasBus.h>

namespace AzToolsFramework
{
    class ReadOnlyEntityPublicInterface;

    namespace Prefab
    {
        class PrefabFocusPublicInterface;
        class PrefabPublicInterface;
    }
}

namespace LandscapeCanvasEditor
{
    ////////////////////////////////////////////////////////////////////////
    // Temporary classes for using a custom Pinned Inspector as a Node Inspector
    // that will use the selected nodes in the graph to drive the Node Inspector
    // based on the corresponding Vegetation Entities. These will be removed
    // once a generic Node Inspector has been implemented for the base EditorMainWindow.
    class CustomEntityPropertyEditor
        : public AzToolsFramework::EntityPropertyEditor
    {
    public:
        AZ_CLASS_ALLOCATOR(CustomEntityPropertyEditor, AZ::SystemAllocator);

        CustomEntityPropertyEditor(QWidget* parent = nullptr);

    protected:
        void CloseInspectorWindow() override;
        QString GetEntityDetailsLabelText() const override;
    };

    class CustomNodeInspectorDockWidget
        : public AzQtComponents::StyledDockWidget
    {
    public:
        AZ_CLASS_ALLOCATOR(CustomNodeInspectorDockWidget, AZ::SystemAllocator);

        CustomNodeInspectorDockWidget(QWidget* parent = nullptr);

        CustomEntityPropertyEditor* GetEntityPropertyEditor();

    private:
        CustomEntityPropertyEditor* m_propertyEditor = nullptr;
    };
    ////////////////////////////////////////////////////////////////////////

    struct LandscapeCanvasConfig
        : GraphCanvas::AssetEditorWindowConfig
    {
        GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteRoot() override;
    };

    class MainWindow
        : public JE::Editor::Tools::GraphModel::Integration::EditorMainWindow
        , protected LandscapeCanvas::LandscapeCanvasRequestBus::Handler
        , private AZ::EntitySystemBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::EditorPickModeNotificationBus::Handler
        , private AzToolsFramework::EntityCompositionNotificationBus::Handler
        , private AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
        , private AzToolsFramework::Prefab::PrefabFocusNotificationBus::Handler
        , private AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler
        , private CrySystemEventBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(MainWindow, AZ::SystemAllocator)
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

    private:
        JE::Editor::Tools::GraphModel::GraphContextPtr GetGraphContext() const override;

        ////////////////////////////////////////////////////////////////////////
        // GraphModelIntegration::GraphControllerNotificationBus::Handler overrides
        void OnGraphModelNodeAdded(JE::Editor::Tools::GraphModel::NodePtr node) override;
        void OnGraphModelNodeRemoved(JE::Editor::Tools::GraphModel::NodePtr node) override;
        void PreOnGraphModelNodeRemoved(JE::Editor::Tools::GraphModel::NodePtr node) override;
        void OnGraphModelConnectionAdded(JE::Editor::Tools::GraphModel::ConnectionPtr connection) override;
        void OnGraphModelConnectionRemoved(JE::Editor::Tools::GraphModel::ConnectionPtr connection) override;
        void PreOnGraphModelNodeWrapped(JE::Editor::Tools::GraphModel::NodePtr wrapperNode, JE::Editor::Tools::GraphModel::NodePtr node) override;
        void OnGraphModelNodeWrapped(JE::Editor::Tools::GraphModel::NodePtr wrapperNode, JE::Editor::Tools::GraphModel::NodePtr node) override;
        void OnGraphModelGraphModified(JE::Editor::Tools::GraphModel::NodePtr node) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // GraphModelIntegration::EditorMainWindow overrides ...
        void OnEditorOpened(GraphCanvas::EditorDockWidget* dockWidget) override;
        void OnEditorClosing(GraphCanvas::EditorDockWidget* dockWidget) override;
        QAction* AddFileNewAction(QMenu* menu) override;
        QAction* AddFileOpenAction(QMenu* menu) override;
        QAction* AddFileSaveAction(QMenu* menu) override;
        QAction* AddFileSaveAsAction(QMenu* menu) override;
        QMenu* AddEditMenu() override;
        QAction* AddEditCutAction(QMenu* menu) override;
        QAction* AddEditCopyAction(QMenu* menu) override;
        QAction* AddEditPasteAction(QMenu* menu) override;
        void HandleWrapperNodeActionWidgetClicked(JE::Editor::Tools::GraphModel::NodePtr wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint) override;
        GraphCanvas::Endpoint CreateNodeForProposal(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint) override;
        void OnSelectionChanged() override;
        void OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationTarget) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::EntitySystemBus
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name) override;
        ////////////////////////////////////////////////////////////////////////

        bool HandleGraphOpened(const AZ::EntityId& rootEntityId, const GraphCanvas::DockWidgetId& dockWidgetId);

        ////////////////////////////////////////////////////////////////////////
        // AssetEditorRequestBus::Handler overrides
        GraphCanvas::ContextMenuAction::SceneReaction ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // LandscapeCanvas::LandscapeCanvasRequestBus::Handler overrides
        GraphCanvas::GraphId OnGraphEntity(const AZ::EntityId& entityId) override;
        JE::Editor::Tools::GraphModel::NodePtr GetNodeMatchingEntityInGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& entityId) override;
        JE::Editor::Tools::GraphModel::NodePtr GetNodeMatchingEntityComponentInGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityComponentIdPair& entityComponentId) override;
        JE::Editor::Tools::GraphModel::NodePtrList GetAllNodesMatchingEntity(const AZ::EntityId& entityId) override;
        JE::Editor::Tools::GraphModel::NodePtrList GetAllNodesMatchingEntityComponent(const AZ::EntityComponentIdPair& entityComponentId) override;
        ////////////////////////////////////////////////////////////////////////

        JE::Editor::Tools::GraphModel::NodePtrList GetAllNodesMatchingEntityInGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& entityId);
        JE::Editor::Tools::GraphModel::NodePtrList GetAllNodesMatchingEntityComponentInGraph(
            const GraphCanvas::GraphId& graphId, const AZ::EntityComponentIdPair& entityComponentId);

        ////////////////////////////////////////////////////////////////////////
        // GraphCanvas::AssetEditorMainWindow overrides
        bool ConfigureDefaultLayout() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorEntityContextNotificationBus overrides
        void OnEditorEntityCreated(const AZ::EntityId& entityId) override;
        void OnEditorEntityDeleted(const AZ::EntityId& entityId) override;
        ////////////////////////////////////////////////////////////////////////

        void HandleEditorEntityCreated(const AZ::EntityId& entityId, GraphCanvas::GraphId graphId = GraphCanvas::GraphId());
        void QueuedEditorEntityDeleted(const AZ::EntityId& entityId);
        void HandleEditorEntityDeleted(const AZ::EntityId& entityId);

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorPickModeNotificationBus overrides
        void OnEntityPickModeStarted() override;
        void OnEntityPickModeStopped() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EntityCompositionNotificationBus overrides
        void OnEntityComponentAdded(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        void OnEntityComponentRemoved(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        void OnEntityComponentEnabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        void OnEntityComponentDisabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::PropertyEditorEntityChangeNotificationBus overrides
        void OnEntityComponentPropertyChanged(AZ::ComponentId changedComponentId) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::ToolsApplicationNotificationBus overrides
        void EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId) override;
        ////////////////////////////////////////////////////////////////////////

        //! PrefabFocusNotificationBus overrides
        void OnPrefabFocusChanged(AZ::EntityId previousContainerEntityId, AZ::EntityId newContainerEntityId) override;

        //! PrefabPublicNotificationBus overrides
        void OnPrefabInstancePropagationBegin() override;
        void OnPrefabInstancePropagationEnd() override;

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus overrides
        void OnCryEditorEndCreate() override;
        void OnCryEditorEndLoad() override;
        void OnCryEditorCloseScene() override;
        void OnCryEditorSceneClosed() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // GraphCanvas::AssetEditorNotificationBus overrides
        void PostOnActiveGraphChanged() override;
        ////////////////////////////////////////////////////////////////////////

        void GetChildrenTree(const AZ::EntityId& rootEntityId, AzToolsFramework::EntityIdList& childrenList);

        QString GetPropertyPathForSlot(JE::Editor::Tools::GraphModel::SlotPtr slot, JE::Editor::Tools::GraphModel::DataType::Enum dataType, int elementIndex = 0);
        void UpdateConnectionData(JE::Editor::Tools::GraphModel::ConnectionPtr connection, bool added);
        void HandleSetImageAssetPath(const AZ::EntityId& sourceEntityId, const AZ::EntityId& targetEntityId);

        AZ::u32 GetWrappedNodeLayoutOrder(JE::Editor::Tools::GraphModel::NodePtr node);

        AZ::EntityId GetRootEntityIdForGraphId(const GraphCanvas::GraphId& graphId);

        AZ::ComponentId AddComponentTypeIdToEntity(
            const AZ::EntityId& entityId,
            AZ::TypeId componentToAddTypeId,
            AZStd::span<const AZ::ComponentServiceType> optionalServices = {});
        void AddComponentForNode(JE::Editor::Tools::GraphModel::NodePtr node, const AZ::EntityId& entityId);
        void HandleNodeCreated(JE::Editor::Tools::GraphModel::NodePtr node);
        void HandleNodeAdded(JE::Editor::Tools::GraphModel::NodePtr node);

        using EntityIdNodeMap = AZStd::unordered_map<AZ::EntityId, JE::Editor::Tools::GraphModel::NodePtr>;
        using EntityIdNodeMaps = AZStd::vector<EntityIdNodeMap>;
        enum EntityIdNodeMapEnum
        {
            Invalid = -1,
            Shapes = 0,
            Gradients,
            WrapperNodes,
            Count
        };

        using EntityComponentCallback = AZStd::function<void(const AZ::EntityId& entityId, AZ::Component* component, bool isDisabled)>;
        using NodeSlotPair = AZStd::pair<JE::Editor::Tools::GraphModel::NodePtr, JE::Editor::Tools::GraphModel::SlotPtr>;
        using ConnectionsList = AZStd::vector<AZStd::pair<NodeSlotPair, NodeSlotPair>>;
        void UpdateEntityIdNodeMap(GraphCanvas::GraphId, JE::Editor::Tools::GraphModel::NodePtr node);
        EntityIdNodeMap* GetEntityIdNodeMap(GraphCanvas::GraphId, JE::Editor::Tools::GraphModel::NodePtr node);
        void ParseNodeConnections(GraphCanvas::GraphId graphId, JE::Editor::Tools::GraphModel::NodePtr node, ConnectionsList& connections);
        void UpdateConnections(JE::Editor::Tools::GraphModel::NodePtr node);
        GraphCanvas::GraphId FindGraphContainingEntity(const AZ::EntityId& entityId);
        void EnumerateEntityComponentTree(const AZ::EntityId& rootEntityId, EntityComponentCallback callback);
        void InitialEntityGraph(const AZ::EntityId& entityId, GraphCanvas::GraphId graphId);
        JE::Editor::Tools::GraphModel::NodePtrList RefreshEntityComponentNodes(const AZ::EntityId& targetEntityId, GraphCanvas::GraphId graphId);
        void PlaceNewNode(GraphCanvas::GraphId graphId, LandscapeCanvas::BaseNode::BaseNodePtr node);
        int GetInboundDataSlotIndex(JE::Editor::Tools::GraphModel::NodePtr node, JE::Editor::Tools::GraphModel::DataTypePtr dataType, JE::Editor::Tools::GraphModel::SlotPtr targetSlot);
        void HandleImageAssetSlot(JE::Editor::Tools::GraphModel::NodePtr targetNode, const EntityIdNodeMap& gradientNodeMap, ConnectionsList& connections);
        void HandleDeserializedNodes();

        //! Determines whether or not we should allow the user to interact with the graph
        //! This should be disabled when there is no level currently loaded
        void UpdateGraphEnabled();

        //! Return the input data slot on the node that matches the specified data type for the specified index.
        //!   - If this is a normal slot, it will just return the appropriate slot that is found.
        //!   - If this is an extendable slot, then slot(s) may need to be added in order to satisfy the requested index.
        JE::Editor::Tools::GraphModel::SlotPtr EnsureInboundDataSlotWithIndex(GraphCanvas::GraphId graphId, JE::Editor::Tools::GraphModel::NodePtr node, JE::Editor::Tools::GraphModel::DataTypePtr dataType, int index);

        AZ::SerializeContext* m_serializeContext = nullptr;

        static AzFramework::EntityContextId s_editorEntityContextId;
        AzToolsFramework::Prefab::PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
        AzToolsFramework::Prefab::PrefabPublicInterface* m_prefabPublicInterface = nullptr;
        AzToolsFramework::ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;

        bool m_ignoreGraphUpdates = false;
        bool m_prefabPropagationInProgress = false;
        bool m_inObjectPickMode = false;

        using DeletedNodePositionsMap = AZStd::unordered_map<AZ::EntityComponentIdPair, AZ::Vector2>;
        AZStd::unordered_map<GraphCanvas::GraphId, DeletedNodePositionsMap> m_deletedNodePositions;
        JE::Editor::Tools::GraphModel::NodePtrList m_addedWrappedNodes;
        JE::Editor::Tools::GraphModel::NodePtrList m_deletedWrappedNodes;
        JE::Editor::Tools::GraphModel::NodePtrList m_deserializedNodes;
        AzToolsFramework::EntityIdList m_queuedEntityDeletes;
        AzToolsFramework::EntityIdList m_queuedEntityRefresh;

        AzToolsFramework::EntityIdList m_ignoreEntityComponentPropertyChanges;

        /// Keep track of the dock widget for the graph that represents the Vegetation Entity
        AZStd::unordered_map<AZ::EntityId, GraphCanvas::DockWidgetId> m_dockWidgetsByEntity;

        /// Keep track of the EntityId/Node mappings per graph for performance reasons so that we
        /// don't have to parse through all the nodes in a graph to find right one when connecting
        /// slots based on the EntityId fields in the component properties.  The mappings are tracked
        /// by type as well for faster lookup since the slot data types are separated (shape, gradient, area).
        AZStd::unordered_map<GraphCanvas::GraphId, EntityIdNodeMaps> m_entityIdNodeMapsByGraph;

        CustomNodeInspectorDockWidget* m_customNodeInspector = nullptr;

        QAction* m_fileNewAction = nullptr;
    };
} 
