/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>

// Graph Canvas
#include <JE/Editor/Tools/GraphCanvas/GraphCanvasBus.h>

// Graph Model
#include <JE/Editor/Tools/GraphModel/Integration/GraphControllerManager.h>

namespace JE::Editor::Tools::GraphModel::Integration
{
    GraphControllerManager::GraphControllerManager()
    {
        GraphManagerRequestBus::Handler::BusConnect();
    }

    GraphControllerManager::~GraphControllerManager()
    {
        GraphManagerRequestBus::Handler::BusDisconnect();
    }

    AZ::Entity* GraphControllerManager::CreateScene(JE::Editor::Tools::GraphModel::GraphPtr graph, const JE::Editor::Tools::GraphCanvas::EditorId editorId)
    {
        AZ::Entity* scene = nullptr;
        JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::BroadcastResult(scene, &JE::Editor::Tools::GraphCanvas::GraphCanvasRequests::CreateSceneAndActivate);

        // Set the EditorId for the new scene
        const AZ::EntityId& sceneId = scene->GetId();
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(sceneId, &JE::Editor::Tools::GraphCanvas::SceneRequests::SetEditorId, editorId);

        // Create a graph controller for the new scene
        CreateGraphController(sceneId, graph);

        return scene;
    }

    void GraphControllerManager::RemoveScene(const JE::Editor::Tools::GraphCanvas::GraphId& sceneId)
    {
        DeleteGraphController(sceneId);
    }

    void GraphControllerManager::CreateGraphController(const JE::Editor::Tools::GraphCanvas::GraphId& sceneId, JE::Editor::Tools::GraphModel::GraphPtr graph)
    {
        m_graphControllers[sceneId] = AZStd::make_shared<GraphController>(graph, sceneId);
    }

    void GraphControllerManager::DeleteGraphController(const JE::Editor::Tools::GraphCanvas::GraphId& sceneId)
    {
        m_graphControllers.erase(sceneId);
    }

    JE::Editor::Tools::GraphModel::GraphPtr GraphControllerManager::GetGraph(const JE::Editor::Tools::GraphCanvas::GraphId& sceneId)
    {
        const auto it = m_graphControllers.find(sceneId);
        return it != m_graphControllers.end() ? it->second->GetGraph() : nullptr;
    }

    const GraphModelSerialization& GraphControllerManager::GetSerializedMappings()
    {
        return m_serialization;
    }

    void GraphControllerManager::SetSerializedMappings(const GraphModelSerialization& serialization)
    {
        m_serialization = serialization;
    }
} // namespace JE::Editor::Tools::GraphModel::Integration
