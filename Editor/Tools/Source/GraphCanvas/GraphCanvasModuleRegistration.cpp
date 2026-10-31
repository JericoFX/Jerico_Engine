/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/GraphCanvasModuleRegistration.h>
#include <GraphCanvas/GraphCanvas.h>

#include <GraphCanvas/Components/BookmarkAnchor/BookmarkAnchorComponent.h>
#include <GraphCanvas/Components/BookmarkAnchor/BookmarkAnchorLayerControllerComponent.h>
#include <GraphCanvas/Components/BookmarkAnchor/BookmarkAnchorVisualComponent.h>
#include <GraphCanvas/Components/BookmarkManagerComponent.h>
#include <GraphCanvas/Components/GeometryComponent.h>
#include <GraphCanvas/Components/GridComponent.h>
#include <GraphCanvas/Components/GridVisualComponent.h>
#include <GraphCanvas/Components/PersistentIdComponent.h>
#include <GraphCanvas/Components/SceneComponent.h>
#include <GraphCanvas/Components/SceneMemberComponent.h>
#include <GraphCanvas/Components/StylingComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionComponent.h>
#include <GraphCanvas/Components/Connections/ConnectionLayerControllerComponent.h>
#include <GraphCanvas/Components/Connections/ConnectionVisualComponent.h>
#include <GraphCanvas/Components/Connections/DataConnections/DataConnectionComponent.h>
#include <GraphCanvas/Components/Connections/DataConnections/DataConnectionVisualComponent.h>

#include <GraphCanvas/Components/Nodes/NodeComponent.h>
#include <GraphCanvas/Components/Nodes/NodeLayerControllerComponent.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentLayerControllerComponent.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentNodeFrameComponent.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentNodeLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentNodeTextComponent.h>
#include <GraphCanvas/Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <GraphCanvas/Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/General/GeneralNodeTitleComponent.h>
#include <GraphCanvas/Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/Group/CollapsedNodeGroupComponent.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupLayerControllerComponent.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupFrameComponent.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeLayoutComponent.h>

#include <GraphCanvas/Components/Slots/SlotComponent.h>
#include <GraphCanvas/Components/Slots/SlotConnectionFilterComponent.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotComponent.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotLayoutComponent.h>
#include <GraphCanvas/Components/Slots/Default/DefaultSlotLayoutComponent.h>
#include <GraphCanvas/Components/Slots/Execution/ExecutionSlotComponent.h>
#include <GraphCanvas/Components/Slots/Execution/ExecutionSlotLayoutComponent.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotComponent.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotLayoutComponent.h>
#include <GraphCanvas/Components/Slots/Property/PropertySlotComponent.h>
#include <GraphCanvas/Components/Slots/Property/PropertySlotLayoutComponent.h>

#include <JE/Editor/Tools/GraphCanvas/Components/ColorPaletteManager/ColorPaletteManagerComponent.h>
#include <JE/Editor/Tools/GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <JE/Editor/Tools/GraphCanvas/Styling/Style.h>
#include <JE/Editor/Tools/GraphCanvas/Styling/StyleManager.h>
#include <JE/Editor/Tools/GraphCanvas/Styling/PseudoElement.h>

namespace JE::Editor::Tools::GraphCanvas
{
    void AppendGraphCanvasDescriptors(AZStd::list<AZ::ComponentDescriptor*>& descriptors)
    {
        descriptors.insert(descriptors.end(), {

            // Components
            BookmarkManagerComponent::CreateDescriptor(),
            GraphCanvasPropertyComponent::CreateDescriptor(),
            GraphCanvasSystemComponent::CreateDescriptor(),
            LayerControllerComponent::CreateDescriptor(),
            PersistentIdComponent::CreateDescriptor(),
            SceneComponent::CreateDescriptor(),
            SceneMemberComponent::CreateDescriptor(),

            // Background Grid
            GridComponent::CreateDescriptor(),
            GridVisualComponent::CreateDescriptor(),

            // BookmarkAnchor
            BookmarkAnchorComponent::CreateDescriptor(),
            BookmarkAnchorLayerControllerComponent::CreateDescriptor(),
            BookmarkAnchorVisualComponent::CreateDescriptor(),

            // General
            GeometryComponent::CreateDescriptor(),

            // Connections
            ConnectionComponent::CreateDescriptor(),
            ConnectionLayerControllerComponent::CreateDescriptor(),
            ConnectionVisualComponent::CreateDescriptor(),

            // Connections::DataConnections
            DataConnectionComponent::CreateDescriptor(),
            DataConnectionVisualComponent::CreateDescriptor(),

            // Nodes
            NodeComponent::CreateDescriptor(),
            NodeLayerControllerComponent::CreateDescriptor(),
            NodeLayoutComponent::CreateDescriptor(),

            // CommentNode
            CommentLayerControllerComponent::CreateDescriptor(),
            CommentNodeFrameComponent::CreateDescriptor(),
            CommentNodeLayoutComponent::CreateDescriptor(),
            CommentNodeTextComponent::CreateDescriptor(),

            // GeneralNode
            GeneralNodeTitleComponent::CreateDescriptor(),
            GeneralSlotLayoutComponent::CreateDescriptor(),
            GeneralNodeFrameComponent::CreateDescriptor(),
            GeneralNodeLayoutComponent::CreateDescriptor(),

            // NodeGroup
            CollapsedNodeGroupComponent::CreateDescriptor(),
            NodeGroupLayerControllerComponent::CreateDescriptor(),
            NodeGroupLayoutComponent::CreateDescriptor(),
            NodeGroupFrameComponent::CreateDescriptor(),

            // Wrapper Node
            WrapperNodeLayoutComponent::CreateDescriptor(),

            // Slots
            SlotComponent::CreateDescriptor(),
            SlotConnectionFilterComponent::CreateDescriptor(),
            DefaultSlotLayoutComponent::CreateDescriptor(),

            // Data Slots
            DataSlotComponent::CreateDescriptor(),
            DataSlotLayoutComponent::CreateDescriptor(),

            // Execution Slots
            ExecutionSlotComponent::CreateDescriptor(),
            ExecutionSlotLayoutComponent::CreateDescriptor(),

            // Extender Slots
            ExtenderSlotComponent::CreateDescriptor(),
            ExtenderSlotLayoutComponent::CreateDescriptor(),

            // Property Slots
            PropertySlotComponent::CreateDescriptor(),
            PropertySlotLayoutComponent::CreateDescriptor(),

            // Styling
            StylingComponent::CreateDescriptor(),

            Styling::ComputedStyle::CreateDescriptor(),
            Styling::VirtualChildElement::CreateDescriptor(),

            // Deprecated Components
            Deprecated::ColorPaletteManagerComponent::CreateDescriptor(),
            Deprecated::StyleSheetComponent::CreateDescriptor()
        });
    }
}

// Qt resources are defined in the native EditorTools static library, so we must
// initialize them manually
extern int qInitResources_GraphCanvasEditorResources();
extern int qCleanupResources_GraphCanvasEditorResources();
namespace {
    struct initializer {
        initializer() { qInitResources_GraphCanvasEditorResources(); }
        ~initializer() { qCleanupResources_GraphCanvasEditorResources(); }
    } dummy;
}
