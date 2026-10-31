/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Canvas
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasMimeEvent.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <JE/Editor/Tools/GraphModel/Model/Common.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    template<typename NodeType>
    class CreateInputOutputNodeMimeEvent;

    //! Provides a common interface for instantiating InputGraphNode and OutputGraphNode through the Node Palette
    template<typename NodeType>
    class InputOutputNodePaletteItem
        : public JE::Editor::Tools::GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(InputOutputNodePaletteItem, AZ::SystemAllocator);

        //! Constructor
        //! \param nodeName Name of the node that will show up in the Palette
        //! \param editorId Unique name of the client system editor (ex: AZ_CRC_CE("ShaderCanvas"))
        //! \param dataType The type of data that the InputGraphNode or OutputGraphNode will represent
        InputOutputNodePaletteItem(AZStd::string_view nodeName, JE::Editor::Tools::GraphCanvas::EditorId editorId, JE::Editor::Tools::GraphModel::DataTypePtr dataType)
            : DraggableNodePaletteTreeItem(nodeName, editorId)
            , m_dataType(dataType)
        {}
        ~InputOutputNodePaletteItem() = default;

        JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override
        {
            return aznew CreateInputOutputNodeMimeEvent<NodeType>(m_dataType);
        }

    protected:
        JE::Editor::Tools::GraphModel::DataTypePtr m_dataType;
    };


    template<typename NodeType>
    class CreateInputOutputNodeMimeEvent
        : public JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI((CreateInputOutputNodeMimeEvent, "{16BED069-A386-4E5C-8A5A-0827121991E7}", NodeType), JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateInputOutputNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

            if (serializeContext)
            {
                serializeContext->Class<CreateInputOutputNodeMimeEvent, JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent>()
                    ->Version(0)
                    ->Field("m_dataType", &CreateInputOutputNodeMimeEvent::m_dataType)
                    ;
            }
        }

        CreateInputOutputNodeMimeEvent() = default; // Required by SerializeContext

        explicit CreateInputOutputNodeMimeEvent(JE::Editor::Tools::GraphModel::DataTypePtr dataType)
        {
            // Copy because m_dataType has to be non-const for use with SerializeContext, and dataType is const
            m_dataType = AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(*dataType);
        }

        bool ExecuteEvent([[maybe_unused]] const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasSceneId) override
        {
            JE::Editor::Tools::GraphModel::GraphPtr graph = nullptr;
            GraphManagerRequestBus::BroadcastResult(graph, &GraphManagerRequests::GetGraph, graphCanvasSceneId);
            if (!graph)
            {
                return false;
            }

            AZStd::shared_ptr<JE::Editor::Tools::GraphModel::Node> node = AZStd::make_shared<NodeType>(graph, m_dataType);
            if (!node)
            {
                return false;
            }

            GraphControllerRequestBus::Event(graphCanvasSceneId, &GraphControllerRequests::AddNode, node, dropPosition);

            return true;
        }

    protected:
        AZStd::shared_ptr<JE::Editor::Tools::GraphModel::DataType> m_dataType;
    };
}
