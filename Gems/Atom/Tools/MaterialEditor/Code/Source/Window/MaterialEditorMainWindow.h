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
#include <AzCore/Component/ComponentApplicationBus.h>

namespace MaterialEditor
{
    //! MaterialEditorMainWindow is the main class. Its responsibility is limited to initializing and connecting
    //! its panels, managing selection of assets, and performing high-level actions like saving. It contains...
    //! 2) MaterialViewport        - The user can see the selected Material applied to a model.
    //! 3) MaterialPropertyInspector  - The user edits the properties of the selected Material.
    class MaterialEditorMainWindow : public JE::Editor::Tools::AtomToolsDocumentMainWindow
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialEditorMainWindow, AZ::SystemAllocator);

        using Base = JE::Editor::Tools::AtomToolsDocumentMainWindow;

        MaterialEditorMainWindow(const AZ::Crc32& toolId, QWidget* parent = nullptr);

    protected:
        // AtomToolsFramework::AtomToolsMainWindowRequestBus::Handler overrides...
        void ResizeViewportRenderTarget(uint32_t width, uint32_t height) override;
        void LockViewportRenderTargetSize(uint32_t width, uint32_t height) override;
        void UnlockViewportRenderTargetSize() override;

        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::AtomToolsDocumentMainWindow overrides...
        AZStd::string GetHelpUrl() const override;

        JE::Editor::Tools::AtomToolsDocumentInspector* m_documentInspector = {};
        JE::Editor::Tools::EntityPreviewViewportSettingsInspector* m_viewportSettingsInspector = {};
        JE::Editor::Tools::EntityPreviewViewportToolBar* m_toolBar = {};
        JE::Editor::Tools::EntityPreviewViewportWidget* m_materialViewport = {};
    };
} // namespace MaterialEditor
