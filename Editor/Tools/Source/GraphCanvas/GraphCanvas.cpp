/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/GraphCanvas.h>

#include <AzFramework/Translation/TranslationDef.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <GraphCanvas/Components/BookmarkManagerComponent.h>
#include <GraphCanvas/Components/BookmarkAnchor/BookmarkAnchorComponent.h>
#include <GraphCanvas/Components/BookmarkAnchor/BookmarkAnchorLayerControllerComponent.h>
#include <GraphCanvas/Components/BookmarkAnchor/BookmarkAnchorVisualComponent.h>
#include <GraphCanvas/Components/GeometryComponent.h>
#include <GraphCanvas/Components/GridComponent.h>
#include <GraphCanvas/Components/GridVisualComponent.h>
#include <GraphCanvas/Components/PersistentIdComponent.h>
#include <GraphCanvas/Components/SceneComponent.h>
#include <GraphCanvas/Components/SceneMemberComponent.h>
#include <GraphCanvas/Components/StylingComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionComponent.h>
#include <GraphCanvas/Components/Connections/ConnectionVisualComponent.h>
#include <GraphCanvas/Components/Connections/DataConnections/DataConnectionComponent.h>
#include <GraphCanvas/Components/Connections/DataConnections/DataConnectionVisualComponent.h>

#include <GraphCanvas/Components/Nodes/NodeComponent.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentNodeLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/Group/CollapsedNodeGroupComponent.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupFrameComponent.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeLayoutComponent.h>

#include <GraphCanvas/Components/NodePropertyDisplays/AssetIdNodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplays/BooleanNodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplays/ComboBoxNodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplays/EntityIdNodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplays/NumericNodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplays/ReadOnlyNodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplays/StringNodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplays/VectorNodePropertyDisplay.h>

#include <GraphCanvas/Components/Slots/Data/DataSlotComponent.h>
#include <GraphCanvas/Components/Slots/Execution/ExecutionSlotComponent.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotComponent.h>
#include <GraphCanvas/Components/Slots/Property/PropertySlotComponent.h>

#include <JE/Editor/Tools/GraphCanvas/Styling/Selector.h>
#include <JE/Editor/Tools/GraphCanvas/Styling/SelectorImplementations.h>
#include <JE/Editor/Tools/GraphCanvas/Styling/PseudoElement.h>

#include <JE/Editor/Tools/GraphCanvas/Types/ConstructPresets.h>
#include <JE/Editor/Tools/GraphCanvas/Types/EntitySaveData.h>

#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasAssetEditorMainWindow.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasTreeModel.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/MimeEvents/CreateSplicingNodeMimeEvent.h>

#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>

namespace JE::Editor::Tools::GraphCanvas
{
    bool EntitySaveDataContainerVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 1)
        {
            AZ::Crc32 componentDataId = AZ_CRC_CE("ComponentData");
            AZStd::unordered_map< AZ::Uuid, JE::Editor::Tools::GraphCanvas::ComponentSaveData* > componentSaveData;

            AZ::SerializeContext::DataElementNode* dataNode = classElement.FindSubElement(componentDataId);

            if (dataNode)
            {
                dataNode->GetDataHierarchy(context, componentSaveData);
            }

            classElement.RemoveElementByName(componentDataId);

            AZStd::unordered_map< AZ::Uuid, JE::Editor::Tools::GraphCanvas::ComponentSaveData* > remappedComponentSaveData;

            for (const auto& mapPair : componentSaveData)
            {
                auto mapIter = remappedComponentSaveData.find(azrtti_typeid(mapPair.second));

                if (mapIter == remappedComponentSaveData.end())
                {
                    remappedComponentSaveData[azrtti_typeid(mapPair.second)] = mapPair.second;
                }
            }

            classElement.AddElementWithData(context, "ComponentData", remappedComponentSaveData);
        }

        return true;
    }

    void GraphCanvasSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphCanvasSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            // Reflect information for the stripped down saving
            serializeContext->Class<ComponentSaveData>()
                ->Version(1)
                ;

            serializeContext->Class<EntitySaveDataContainer>()
                ->Version(2, &EntitySaveDataContainerVersionConverter)
                ->Field("ComponentData", &EntitySaveDataContainer::m_entityData)
                ;
            ////

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphCanvasSystemComponent>(
                    QT_TRANSLATE_NOOP("GraphCanvas", "LmbrCentral"), QT_TRANSLATE_NOOP("GraphCanvas", "Provides factory methods for Graph Canvas components"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }            
            
            NodeConfiguration::Reflect(serializeContext);
            Styling::SelectorImplementation::Reflect(serializeContext);
            Styling::Selector::Reflect(serializeContext);
            Styling::NullSelector::Reflect(serializeContext);
            Styling::BasicSelector::Reflect(serializeContext);
            Styling::DefaultSelector::Reflect(serializeContext);
            Styling::CompoundSelector::Reflect(serializeContext);
            Styling::NestedSelector::Reflect(serializeContext);
            Styling::Style::Reflect(serializeContext);
            AssetEditorUserSettings::Reflect(serializeContext);
        }

        EditorConstructPresets::Reflect(context);
        GraphCanvasMimeEvent::Reflect(context);
        GraphCanvasTreeModel::Reflect(context);        
        CreateSplicingNodeMimeEvent::Reflect(context);
        TranslationAsset::Reflect(context);

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AssetEditorRequestBus>("AssetEditorRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "AssetEditor")
                ->Attribute(AZ::Script::Attributes::Module, EditorGraphModuleName)
                ->Event("CreateNewGraph", &AssetEditorRequests::CreateNewGraph)
                ->Event("ContainsGraph", &AssetEditorRequests::ContainsGraph)
                ->Event("CloseGraph", &AssetEditorRequests::CloseGraph)
                ;

            behaviorContext->EBus<SceneRequestBus>("SceneRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Scene")
                ->Attribute(AZ::Script::Attributes::Module, EditorGraphModuleName)
                ->Event("CutSelection", &SceneRequests::CutSelection)
                ->Event("CopySelection", &SceneRequests::CopySelection)
                ->Event("Paste", &SceneRequests::Paste)
                ->Event("DuplicateSelection", &SceneRequests::DuplicateSelection)
                ;
        }
    }

    void GraphCanvasSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AssetDatabaseService"));
    }

    void GraphCanvasSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(GraphCanvasRequestsServiceId);
    }

    void GraphCanvasSystemComponent::Init()
    {
        m_translationDatabase.Init();
    }

    void GraphCanvasSystemComponent::Activate()
    {
        RegisterTranslationAssetHandler();

        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        GraphCanvasRequestBus::Handler::BusConnect();
        Styling::PseudoElementFactoryRequestBus::Handler::BusConnect();
    }

    void GraphCanvasSystemComponent::RegisterTranslationAssetHandler()
    {
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(
            &AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType,
            azrtti_typeid<TranslationAsset>(), TranslationAsset::GetFileFilter());

        m_assetHandler = AZStd::make_unique<TranslationAssetHandler>();
        m_assetHandler->Register();
    }

    void GraphCanvasSystemComponent::Deactivate()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        Styling::PseudoElementFactoryRequestBus::Handler::BusDisconnect();
        GraphCanvasRequestBus::Handler::BusDisconnect();

        UnregisterAssetHandler();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateBookmarkAnchor() const
    {
        AZ::Entity* entity = aznew AZ::Entity("BookmarkAnchor");
        entity->CreateComponent<BookmarkAnchorComponent>();
        entity->CreateComponent<BookmarkAnchorVisualComponent>();

        const bool isGroupable = false;
        entity->CreateComponent<SceneMemberComponent>(isGroupable);

        entity->CreateComponent<GeometryComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::BookmarkAnchor, AZ::EntityId());
        entity->CreateComponent<PersistentIdComponent>();
        entity->CreateComponent<BookmarkAnchorLayerControllerComponent>();

        return entity;
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateScene() const
    {
        // create a new empty canvas, give it a name to avoid serialization generating one based on
        // the ID (which in some cases caused diffs to fail in the editor)
        AZ::Entity* entity = aznew AZ::Entity("GraphCanvasScene");
        entity->CreateComponent<SceneComponent>();
        entity->CreateComponent<BookmarkManagerComponent>();
        return entity;
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateCoreNode() const
    {
        return NodeComponent::CreateCoreNodeEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateGeneralNode(const char* nodeType) const
    {
        return GeneralNodeLayoutComponent::CreateGeneralNodeEntity(nodeType);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateCommentNode() const
    {
        return CommentNodeLayoutComponent::CreateCommentNodeEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateWrapperNode(const char* nodeType) const
    {
        return WrapperNodeLayoutComponent::CreateWrapperNodeEntity(nodeType);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateNodeGroup() const
    {
        return NodeGroupLayoutComponent::CreateNodeGroupEntity();
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateCollapsedNodeGroup(const CollapsedNodeGroupConfiguration& collapsedNodeGroupConfiguration) const
    {
        return CollapsedNodeGroupComponent::CreateCollapsedNodeGroupEntity(collapsedNodeGroupConfiguration);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreateSlot(const AZ::EntityId& nodeId, const SlotConfiguration& slotConfiguration) const
    {
        if (const DataSlotConfiguration* dataSlotConfiguration = azrtti_cast<const DataSlotConfiguration*>(&slotConfiguration))
        {
            return DataSlotComponent::CreateDataSlot(nodeId, (*dataSlotConfiguration));
        }
        else if (const ExecutionSlotConfiguration* executionSlotConfiguration = azrtti_cast<const ExecutionSlotConfiguration*>(&slotConfiguration))
        {
            return ExecutionSlotComponent::CreateExecutionSlot(nodeId, (*executionSlotConfiguration));
        }
        else if (const ExtenderSlotConfiguration* extenderSlotConfiguration = azrtti_cast<const ExtenderSlotConfiguration*>(&slotConfiguration))
        {
            return ExtenderSlotComponent::CreateExtenderSlot(nodeId, (*extenderSlotConfiguration));
        }
        else
        {
            AZ_Error("GraphCanvas", false, "Trying to create using an unknown Slot Configuration");
        }

        return nullptr;
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateBooleanNodePropertyDisplay(BooleanDataInterface* dataInterface) const
    {
        return aznew BooleanNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateNumericNodePropertyDisplay(NumericDataInterface* dataInterface) const
    {
        return aznew NumericNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateComboBoxNodePropertyDisplay(ComboBoxDataInterface* dataInterface) const
    {
        return aznew ComboBoxNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateEntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface) const
    {
        return aznew EntityIdNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateReadOnlyNodePropertyDisplay(ReadOnlyDataInterface* dataInterface) const
    {
        return aznew ReadOnlyNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateStringNodePropertyDisplay(StringDataInterface* dataInterface) const
    {
        return aznew StringNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateVectorNodePropertyDisplay(VectorDataInterface* dataInterface) const
    {
        return aznew VectorNodePropertyDisplay(dataInterface);
    }

    NodePropertyDisplay* GraphCanvasSystemComponent::CreateAssetIdNodePropertyDisplay(AssetIdDataInterface* dataInterface) const
    {
        return aznew AssetIdNodePropertyDisplay(dataInterface);
    }

    AZ::Entity* GraphCanvasSystemComponent::CreatePropertySlot(const AZ::EntityId& nodeId, const AZ::Crc32& propertyId, const SlotConfiguration& configuration) const
    {
        return PropertySlotComponent::CreatePropertySlot(nodeId, propertyId, configuration);
    }

    AZ::EntityId GraphCanvasSystemComponent::CreateStyleEntity(const AZStd::string& style) const
    {
        return StylingComponent::CreateStyleEntity(style);
    }

    AZ::EntityId GraphCanvasSystemComponent::CreateVirtualChild(const AZ::EntityId& real, const AZStd::string& virtualChildElement) const
    {
        return Styling::VirtualChildElement::Create(real, virtualChildElement);
    }

    void GraphCanvasSystemComponent::OnCatalogLoaded(const char* /*catalogFile*/)
    {
        JE::Editor::Tools::GraphCanvas::TranslationRequestBus::Broadcast(&JE::Editor::Tools::GraphCanvas::TranslationRequests::Restore);
    }

    void GraphCanvasSystemComponent::OnCatalogAssetRemoved(const AZ::Data::AssetId& /*assetId*/, const AZ::Data::AssetInfo& assetInfo)
    {
        if (assetInfo.m_assetType == azrtti_typeid<TranslationAsset>())
        {
            JE::Editor::Tools::GraphCanvas::TranslationRequestBus::Broadcast(&JE::Editor::Tools::GraphCanvas::TranslationRequests::Restore);
        }
    }

    void GraphCanvasSystemComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        ReloadDatabase(assetId);
    }

    void GraphCanvasSystemComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        ReloadDatabase(assetId);
    }

    void GraphCanvasSystemComponent::ReloadDatabase(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);
        if (assetInfo.m_assetType == azrtti_typeid<TranslationAsset>())
        {
            JE::Editor::Tools::GraphCanvas::TranslationRequestBus::Broadcast(&JE::Editor::Tools::GraphCanvas::TranslationRequests::Restore);
        }
    }

    void GraphCanvasSystemComponent::UnregisterAssetHandler()
    {
        if (m_assetHandler)
        {
            AZ::Data::AssetManager::Instance().UnregisterHandler(m_assetHandler.get());
            m_assetHandler.reset();
        }
    }
}
