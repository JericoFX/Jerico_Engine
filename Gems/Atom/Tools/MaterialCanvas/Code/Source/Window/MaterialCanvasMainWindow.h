/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Tools/Document/AtomToolsDocumentInspector.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentMainWindow.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsInspector.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportToolBar.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportWidget.h>
#include <JE/Editor/Tools/Graph/GraphViewSettings.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>


namespace MaterialCanvas
{
    //! MaterialCanvasMainWindow creates and manages all of the graph canvas and viewport related docked windows for Material Canvas. 
    class MaterialCanvasMainWindow : public JE::Editor::Tools::AtomToolsDocumentMainWindow
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasMainWindow, AZ::SystemAllocator);

        using Base = JE::Editor::Tools::AtomToolsDocumentMainWindow;

        MaterialCanvasMainWindow(
            const AZ::Crc32& toolId, JE::Editor::Tools::GraphViewSettingsPtr graphViewSettingsPtr, QWidget* parent = nullptr);
        ~MaterialCanvasMainWindow() = default;

    protected:
        // AtomToolsFramework::AtomToolsMainWindowRequestBus::Handler overrides...
        void ResizeViewportRenderTarget(uint32_t width, uint32_t height) override;
        void LockViewportRenderTargetSize(uint32_t width, uint32_t height) override;
        void UnlockViewportRenderTargetSize() override;

        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::AtomToolsDocumentMainWindow overrides...
        void PopulateSettingsInspector(JE::Editor::Tools::InspectorWidget* inspector) const override;
        void OnSettingsDialogClosed() override;
        AZStd::string GetHelpUrl() const override;

    private:
        JE::Editor::Tools::AtomToolsDocumentInspector* m_documentInspector = {};
        JE::Editor::Tools::EntityPreviewViewportSettingsInspector* m_viewportSettingsInspector = {};
        JE::Editor::Tools::EntityPreviewViewportToolBar* m_toolBar = {};
        JE::Editor::Tools::EntityPreviewViewportWidget* m_materialViewport = {};
        JE::Editor::Tools::GraphViewSettingsPtr m_graphViewSettingsPtr;
        GraphCanvas::BookmarkDockWidget* m_bookmarkDockWidget = {};
        GraphCanvas::NodePaletteDockWidget* m_nodePalette = {};
        mutable AZStd::shared_ptr<JE::Editor::Tools::DynamicPropertyGroup> m_materialCanvasCompileSettingsGroup;
    };
} // namespace MaterialCanvas
