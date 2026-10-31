/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QMenu>

#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/GridBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/VisualBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/EditorTypes.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Include/ScriptCanvas/Bus/NodeIdPair.h>

#include <ScriptCanvasDeveloperEditor/AutomationActions/NodePaletteFullCreation.h>
#include <ScriptCanvasDeveloperEditor/DeveloperUtils.h>

namespace ScriptCanvasDeveloperEditor
{    
    namespace NodePaletteFullCreation
    {
        class FullCreationNodePaletteInterface
            : public ProcessNodePaletteInterface
        {
        public:
        
            void SetupInterface(const AZ::EntityId& activeGraphCanvasGraphId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
            {
                m_graphCanvasGraphId = activeGraphCanvasGraphId;
                m_scriptCanvasId = scriptCanvasId;
                
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(m_viewId, activeGraphCanvasGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(m_gridId, activeGraphCanvasGraphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetGrid);

                JE::Editor::Tools::GraphCanvas::GridRequestBus::EventResult(m_minorPitch, m_gridId, &JE::Editor::Tools::GraphCanvas::GridRequests::GetMinorPitch);

                JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(m_nodeCreationPos, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetViewSceneCenter);

                JE::Editor::Tools::GraphCanvas::GraphCanvasGraphicsView* graphicsView = nullptr;
                JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(graphicsView, m_viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::AsGraphicsView);

                m_viewportRectangle = graphicsView->mapToScene(graphicsView->viewport()->geometry()).boundingRect();
            }
            
            bool ShouldProcessItem([[maybe_unused]] const JE::Editor::Tools::GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem) const
            {
                return true;
            }
            
            void ProcessItem(const JE::Editor::Tools::GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem)
            {
                JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent* mimeEvent = nodePaletteTreeItem->CreateMimeEvent();

                if (ScriptCanvasEditor::MultiCreateNodeMimeEvent* multiCreateMimeEvent = azrtti_cast<ScriptCanvasEditor::MultiCreateNodeMimeEvent*>(mimeEvent))
                {
                    AZStd::vector< JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent* > mimeEvents = multiCreateMimeEvent->CreateMimeEvents();

                    for (JE::Editor::Tools::GraphCanvas::GraphCanvasMimeEvent* currentEvent : mimeEvents)
                    {
                        DeveloperUtils::HandleMimeEvent(currentEvent, m_graphCanvasGraphId, m_viewportRectangle, m_widthOffset, m_heightOffset, m_maxRowHeight, m_minorPitch);
                        delete currentEvent;
                    }
                }
                else if (mimeEvent)
                {
                    DeveloperUtils::HandleMimeEvent(mimeEvent, m_graphCanvasGraphId, m_viewportRectangle, m_widthOffset, m_heightOffset, m_maxRowHeight, m_minorPitch);
                }
            }
        
        private:
        
            AZ::EntityId m_graphCanvasGraphId;
            ScriptCanvas::ScriptCanvasId m_scriptCanvasId;
        
            AZ::Vector2 m_nodeCreationPos = AZ::Vector2::CreateZero();
            
            AZ::EntityId m_viewId;            
            AZ::EntityId m_gridId;
            
            AZ::Vector2 m_minorPitch = AZ::Vector2::CreateZero();
            
            QRectF m_viewportRectangle;
            
            int m_widthOffset = 0;
            int m_heightOffset = 0;
            
            int m_maxRowHeight = 0;
        };
        
        void NodePaletteFullCreationAction()
        {   
            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationBegin);
            
            FullCreationNodePaletteInterface nodePaletteInterface;
            DeveloperUtils::ProcessNodePalette(nodePaletteInterface);

            ScriptCanvasEditor::AutomationRequestBus::Broadcast(&ScriptCanvasEditor::AutomationRequests::SignalAutomationEnd);
        }

        QAction* CreateNodePaletteFullCreationAction(QMenu* mainMenu)
        {
            QAction* createNodePaletteAction = nullptr;

            if (mainMenu)
            {
                createNodePaletteAction = mainMenu->addAction(QAction::tr("Create Node Palette"));
                createNodePaletteAction->setAutoRepeat(false);
                createNodePaletteAction->setToolTip("Tries to create every node in the node palette. All of them. At once.");
                createNodePaletteAction->setShortcut(QKeySequence(QAction::tr("Ctrl+Shift+h", "Debug|Create Node Palette")));

                QObject::connect(createNodePaletteAction, &QAction::triggered, &NodePaletteFullCreationAction);
            }

            return createNodePaletteAction;
        }
    }
}
