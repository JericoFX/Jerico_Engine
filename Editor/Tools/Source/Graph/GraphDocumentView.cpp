/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Tools/Graph/GraphDocumentRequestBus.h>
#include <JE/Editor/Tools/Graph/GraphDocumentView.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <QEvent>
#include <QShowEvent>
#include <QTimer>

namespace JE::Editor::Tools
{
    GraphDocumentView::GraphDocumentView(
        const AZ::Crc32& toolId, const AZ::Uuid& documentId, GraphViewSettingsPtr graphViewSettingsPtr, QWidget* parent)
        : GraphView(toolId, JE::Editor::Tools::GraphCanvas::GraphId(), graphViewSettingsPtr, parent)
        , m_documentId(documentId)
    {
        m_graphicsView->installEventFilter(this);
        m_graphicsView->viewport()->installEventFilter(this);
        AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
        OnDocumentOpened(m_documentId);

        // Monitor graph settings changes and refresh the graph view if the view settings change
        if (auto registry = AZ::SettingsRegistry::Get())
        {
            m_graphViewSettingsNotifyEventHandler = registry->RegisterNotifier(
                [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
                {
                    if (AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual(
                            "/O3DE/Atom/GraphView/ViewSettings", notifyEventArgs.m_jsonKeyPath))
                    {
                        OnSettingsChanged();
                    }
                });
        }
    }

    GraphDocumentView::~GraphDocumentView()
    {
        AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    bool GraphDocumentView::eventFilter(QObject* watched, QEvent* event)
    {
        if (!m_openedBefore && (watched == m_graphicsView || watched == m_graphicsView->viewport()) &&
            (event->type() == QEvent::Wheel || event->type() == QEvent::MouseButtonPress || event->type() == QEvent::KeyPress))
        {
            m_openedBefore = true;
        }
        return GraphView::eventFilter(watched, event);
    }

    void GraphDocumentView::showEvent(QShowEvent* event)
    {
        GraphView::showEvent(event);
        QueueInitialFraming();
    }

    void GraphDocumentView::QueueInitialFraming()
    {
        if (!m_openedBefore && isVisible() && m_activeGraphId.IsValid())
        {
            const auto graphId = m_activeGraphId;
            QTimer::singleShot(0, this, [this, graphId]()
            {
                if (!m_openedBefore && isVisible() && m_activeGraphId == graphId &&
                    !m_graphicsView->viewport()->size().isEmpty())
                {
                    m_openedBefore = true;
                    JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(
                        m_graphicsView->GetViewId(), &JE::Editor::Tools::GraphCanvas::ViewRequests::ShowEntireGraph);
                }
            });
        }
    }

    void GraphDocumentView::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            JE::Editor::Tools::GraphCanvas::GraphId activeGraphId = JE::Editor::Tools::GraphCanvas::GraphId();
            GraphDocumentRequestBus::EventResult(activeGraphId, m_documentId, &GraphDocumentRequestBus::Events::GetGraphId);
            SetActiveGraphId(activeGraphId, true);

            QueueInitialFraming();
            return;
        }

        SetActiveGraphId(JE::Editor::Tools::GraphCanvas::GraphId(), false);
    }

    void GraphDocumentView::OnDocumentClosed([[maybe_unused]] const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            SetActiveGraphId(JE::Editor::Tools::GraphCanvas::GraphId(), true);
        }
    }

    void GraphDocumentView::OnDocumentDestroyed([[maybe_unused]] const AZ::Uuid& documentId)
    {
        if (m_documentId == documentId)
        {
            SetActiveGraphId(JE::Editor::Tools::GraphCanvas::GraphId(), true);
        }
    }

    void GraphDocumentView::OnSettingsChanged()
    {
        if (m_activeGraphId.IsValid())
        {
            JE::Editor::Tools::GraphCanvas::AssetEditorSettingsNotificationBus::Event(
                m_toolId, &JE::Editor::Tools::GraphCanvas::AssetEditorSettingsNotifications::OnSettingsChanged);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(m_activeGraphId, &JE::Editor::Tools::GraphCanvas::ViewRequests::RefreshView);
        }
    }
} // namespace JE::Editor::Tools
