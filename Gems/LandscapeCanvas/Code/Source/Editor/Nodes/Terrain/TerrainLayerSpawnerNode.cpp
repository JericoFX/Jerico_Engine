/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Qt
#include <QObject>

// AZ
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/Integration/Helpers.h>
#include <JE/Editor/Tools/GraphModel/Model/Slot.h>

// Landscape Canvas
#include <Editor/Core/GraphContext.h>
#include <Editor/Nodes/Terrain/TerrainLayerSpawnerNode.h>

#include <LmbrCentral/Shape/ReferenceShapeComponentBus.h>

namespace LandscapeCanvas
{
    void TerrainLayerSpawnerNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TerrainLayerSpawnerNode, BaseNode>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TerrainLayerSpawnerNode>(
                    QT_TRANSLATE_NOOP("LandscapeCanvas", "TerrainLayerSpawnerNode"), "")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                    Attribute(JE::Editor::Tools::GraphModel::Integration::Attributes::TitlePaletteOverride, "TerrainNodeTitlePalette")
                    ;
            }
        }
    }

    const char* TerrainLayerSpawnerNode::TITLE = "Terrain Layer Spawner";

    TerrainLayerSpawnerNode::TerrainLayerSpawnerNode(JE::Editor::Tools::GraphModel::GraphPtr graph)
        : BaseNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }

    const char* TerrainLayerSpawnerNode::GetTitle() const
    {
        return TITLE;
    }

    const char* TerrainLayerSpawnerNode::GetSubTitle() const
    {
        return LandscapeCanvas::TERRAIN_TITLE;
    }

    JE::Editor::Tools::GraphModel::NodeType TerrainLayerSpawnerNode::GetNodeType() const
    {
        return JE::Editor::Tools::GraphModel::NodeType::WrapperNode;
    }

    const BaseNode::BaseNodeType TerrainLayerSpawnerNode::GetBaseNodeType() const
    {
        return BaseNode::TerrainArea;
    }

    void TerrainLayerSpawnerNode::RegisterSlots()
    {
        CreateEntityNameSlot();
    }
} // namespace LandscapeCanvas
