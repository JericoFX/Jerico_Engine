/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Tools/Document/AtomToolsDocumentApplication.h>
#include <JE/Editor/Tools/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodeManager.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <JE/Editor/Tools/GraphModel/Model/GraphContext.h>
#include <Window/PassCanvasMainWindow.h>

namespace PassCanvas
{
    //! The main application class for Pass Canvas, setting up top level systems, document types, and the main window. 
    class PassCanvasApplication
        : public JE::Editor::Tools::AtomToolsDocumentApplication
        , private AzToolsFramework::EditorWindowRequestBus::Handler
        , private JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PassCanvasApplication, AZ::SystemAllocator)
        AZ_TYPE_INFO(PassCanvas::PassCanvasApplication, "{792D3C47-F380-44BC-B47D-621D8C526360}");

        using Base = JE::Editor::Tools::AtomToolsDocumentApplication;

        PassCanvasApplication(int* argc, char*** argv);
        ~PassCanvasApplication();

        // AzFramework::Application overrides...
        void RegisterCoreComponents() override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        void Reflect(AZ::ReflectContext* context) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Destroy() override;

    private:

        // AtomToolsFramework::AtomToolsApplication overrides...
        AZStd::vector<AZStd::string> GetCriticalAssetFilters() const override;

        // AzToolsFramework::EditorWindowRequests::Bus::Handler
        QWidget* GetAppMainWindow() override;

        void InitDynamicNodeManager();
        void InitDynamicNodeEditData();
        void InitSharedGraphContext();
        void InitGraphViewSettings();
        void InitPassGraphDocumentType();
        void InitMainWindow();
        void InitDefaultDocument();

        AZStd::unique_ptr<PassCanvasMainWindow> m_window;
        AZStd::unique_ptr<JE::Editor::Tools::EntityPreviewViewportSettingsSystem> m_viewportSettingsSystem;
        AZStd::unique_ptr<JE::Editor::Tools::DynamicNodeManager> m_dynamicNodeManager;
        AZStd::shared_ptr<JE::Editor::Tools::GraphModel::GraphContext> m_graphContext;
        JE::Editor::Tools::GraphViewSettingsPtr m_graphViewSettingsPtr;
    };
} // namespace PassCanvas
