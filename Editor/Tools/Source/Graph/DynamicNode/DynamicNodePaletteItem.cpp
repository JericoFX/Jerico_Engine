/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodePaletteItem.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <JE/Editor/Tools/GraphModel/Model/Common.h>

namespace JE::Editor::Tools
{
    void CreateDynamicNodeMimeEvent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CreateDynamicNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("toolId", &CreateDynamicNodeMimeEvent::m_toolId)
                ->Field("configId", &CreateDynamicNodeMimeEvent::m_configId);
        }
    }

    CreateDynamicNodeMimeEvent::CreateDynamicNodeMimeEvent(const AZ::Crc32& toolId, const AZ::Uuid& configId)
        : m_toolId(toolId)
        , m_configId(configId)
    {
    }

    bool CreateDynamicNodeMimeEvent::ExecuteEvent(
        [[maybe_unused]] const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasSceneId)
    {
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphModel::Integration::GraphManagerRequestBus::BroadcastResult(
            graph, &JE::Editor::Tools::GraphModel::Integration::GraphManagerRequests::GetGraph, graphCanvasSceneId);

        if (graph)
        {
            if (auto node = AZStd::make_shared<DynamicNode>(graph, m_toolId, m_configId))
            {
                // Handle undo/redo adding a single node. This will need to be done at a higher level for multiple nodes.
                GraphCanvas::ScopedGraphUndoBatch undoBatch(graphCanvasSceneId);

                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(
                    m_createdNodeId, graphCanvasSceneId, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, node, dropPosition);
                return true;
            }
        }

        return false;
    }

    DynamicNodePaletteItem::DynamicNodePaletteItem(const AZ::Crc32& toolId, const DynamicNodeConfig& config)
        : DraggableNodePaletteTreeItem(config.m_title.c_str(), toolId)
        , m_toolId(toolId)
        , m_configId(config.m_id)
    {
        SetTitlePalette(!config.m_titlePaletteName.empty() ? config.m_titlePaletteName : "DefaultNodeTitlePalette");
        SetToolTip(config.m_description.c_str());
    }

    GraphCanvas::GraphCanvasMimeEvent* DynamicNodePaletteItem::CreateMimeEvent() const
    {
        return aznew CreateDynamicNodeMimeEvent(m_toolId, m_configId);
    }
} // namespace JE::Editor::Tools
