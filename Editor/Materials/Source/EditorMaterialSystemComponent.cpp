/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Material/MaterialSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Render/RPI/Reflect/Asset/AssetUtils.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponentNotificationBus.h>
#include <JE/Render/Materials/MaterialComponentBus.h>
#include <JE/Render/Debug/AtomImGuiToolsBus.h>
#include <JE/Editor/Tools/PreviewRenderer/PreviewRendererCaptureRequest.h>
#include <JE/Editor/Tools/PreviewRenderer/PreviewRendererInterface.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManager.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <Editor/LyViewPaneNames.h>
#include <JE/Editor/Materials/EditorMaterialComponentInspector.h>
#include <JE/Editor/Materials/EditorMaterialComponentUtil.h>
#include <JE/Editor/Materials/EditorMaterialSystemComponent.h>
#include <JE/Editor/Materials/SharedPreviewContent.h>
#include <JE/Editor/Materials/Document/MaterialDocument.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include "Workspace/MaterialWorkspaceWidget.h"
#include "Workspace/MaterialGraphWorkspaceWidget.h"
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QImage>
#include <QMainWindow>
#include <QObject>
#include <QPixmap>
#include <QProcessEnvironment>
AZ_POP_DISABLE_WARNING

constexpr AZStd::string_view MaterialCanvasActionIdentifier = "o3de.action.tools.material_canvas";
constexpr AZStd::string_view MaterialEditorActionIdentifier = "o3de.action.tools.material_editor";

void InitMaterialEditorResources()
{
    //Must register qt resources from other modules
    Q_INIT_RESOURCE(InspectorWidget);
}

namespace JE
{
    namespace Editor
    {
        EditorMaterialSystemComponent::EditorMaterialSystemComponent() = default;
        EditorMaterialSystemComponent::~EditorMaterialSystemComponent() = default;
        void EditorMaterialSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            MaterialDocument::Reflect(context);
            MaterialGraphEnvironment::Reflect(context);
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorMaterialSystemComponent, AZ::Component>()
                    ->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorMaterialSystemComponent>(QT_TRANSLATE_NOOP("AtomLyIntegration", "EditorMaterialSystemComponent"), QT_TRANSLATE_NOOP("AtomLyIntegration", "System component that manages the native material workspace."))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void EditorMaterialSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("EditorMaterialSystem"));
        }

        void EditorMaterialSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EditorMaterialSystem"));
        }

        void EditorMaterialSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PreviewRendererSystem"));
        }

        void EditorMaterialSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void EditorMaterialSystemComponent::Init()
        {
            InitMaterialEditorResources();
        }

        void EditorMaterialSystemComponent::Activate()
        {
            auto documentType = MaterialDocument::BuildDocumentTypeInfo(MaterialDocument::HistoryOwner::Editor);
            // Editing existing materials preserves document paths and their global undo identity.
            documentType.m_supportedExtensionsToOpen = {{"Material", "material"}};
            documentType.m_supportedExtensionsToCreate.clear();
            m_workspaceSession = AZStd::make_unique<MaterialWorkspaceSession>(documentType);
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorWindowRequestBus::BroadcastResult(
                mainWindow, &AzToolsFramework::EditorWindowRequests::GetAppMainWindow);
            m_workspaceSession->SetMainWindow(mainWindow);

            AZ::EntitySystemBus::Handler::BusConnect();
            EditorMaterialSystemComponentNotificationBus::Handler::BusConnect();
            EditorMaterialSystemComponentRequestBus::Handler::BusConnect();
            JE::Render::MaterialComponentNotificationBus::Router::BusRouterConnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
            AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler::BusConnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
            AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
            AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();

            // All material previews use the same model and lighting preset assets
            // models/sphere.azmodel
            m_materialPreviewModelAsset.Create(AZ::Data::AssetId("{6DE0E9A8-A1C7-5D0F-9407-4E627C1F223C}", 284780167));
            // lightingpresets/thumbnail.lightingpreset.azasset
            m_materialPreviewLightingPresetAsset.Create(AZ::Data::AssetId("{4F3761EF-E279-5FDD-98C3-EF90F924FBAC}", 0));

            m_materialBrowserInteractions.reset(aznew MaterialBrowserInteractions);
        }

        void EditorMaterialSystemComponent::Deactivate()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler::BusDisconnect();
            if (m_workspacePaneRegistered)
            {
                AzToolsFramework::UnregisterViewPane(MaterialWorkspaceWidget::PaneName);
                m_workspacePaneRegistered = false;
            }
            if (m_graphWorkspacePaneRegistered)
            {
                AzToolsFramework::UnregisterViewPane(MaterialGraphWorkspaceWidget::PaneName);
                m_graphWorkspacePaneRegistered = false;
            }
            m_workspaceSession.reset();

            AZ::EntitySystemBus::Handler::BusDisconnect();
            EditorMaterialSystemComponentNotificationBus::Handler::BusDisconnect();
            EditorMaterialSystemComponentRequestBus::Handler::BusDisconnect();
            JE::Render::MaterialComponentNotificationBus::Router::BusRouterDisconnect();
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect(); 
            AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect(); 
            AZ::SystemTickBus::Handler::BusDisconnect();
            AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

            m_materialBrowserInteractions.reset();
            m_materialPreviewRequests.clear();
            m_materialPreviewModelAsset.Release();
            m_materialPreviewLightingPresetAsset.Release();
        }

        void EditorMaterialSystemComponent::OpenMaterialEditor(const AZStd::string& sourcePath)
        {
            AZStd::string materialPath = sourcePath;
            if (materialPath.ends_with(EditorMaterialComponentUtil::MaterialTypeExtensionWithDot))
            {
                // Choose the final path before creating a document or any global undo commands.
                materialPath = MaterialBrowserInteractions::CreateMaterial({}, materialPath);
                if (materialPath.empty())
                {
                    return;
                }
            }
            if (!materialPath.empty())
            {
                if (!m_workspaceSession)
                {
                    return;
                }
                // Bind the real document before the pane constructs its inspector and preview.
                const auto id = m_workspaceSession->OpenDocument(materialPath);
                if (id.IsNull() || !m_workspaceSession->SelectDocument(id))
                {
                    return;
                }
            }
            AzToolsFramework::OpenViewPane(MaterialWorkspaceWidget::PaneName);
        }

        void EditorMaterialSystemComponent::GetAssetInspectorWidget(
            const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry,
            QWidget* parent, QWidget*& widget, bool& selectionAccepted)
        {
            if (m_workspaceSession)
            {
                MaterialWorkspaceWidget::GetAssetInspectorWidget(
                    *m_workspaceSession, selectedEntry, parent, widget, selectionAccepted);
            }
        }

        void EditorMaterialSystemComponent::OpenMaterialCanvas(const AZStd::string& sourcePath)
        {
            AzToolsFramework::OpenViewPane(MaterialGraphWorkspaceWidget::PaneName);
            if (!sourcePath.empty())
            {
                if (auto* workspace = AzToolsFramework::GetViewPaneWidget<MaterialGraphWorkspaceWidget>(MaterialGraphWorkspaceWidget::PaneName))
                {
                    workspace->OpenDocument(sourcePath);
                }
            }
        }

        void EditorMaterialSystemComponent::OpenMaterialInspector(
            const AZ::EntityId& primaryEntityId,
            const AzToolsFramework::EntityIdSet& entityIdsToEdit,
            const JE::Render::MaterialAssignmentId& materialAssignmentId)
        {
            auto dockWidget = AzToolsFramework::InstanceViewPane("Material Instance Editor");
            if (dockWidget)
            {
                auto inspector = static_cast<JE::Editor::EditorMaterialComponentInspector::MaterialPropertyInspector*>(dockWidget->widget());
                if (inspector)
                {
                    inspector->LoadMaterial(primaryEntityId, entityIdsToEdit, materialAssignmentId);
                }
            }
        }

        void EditorMaterialSystemComponent::RenderMaterialPreview(
            const AZ::EntityId& entityId, const JE::Render::MaterialAssignmentId& materialAssignmentId)
        {
            m_materialPreviewRequests.emplace(entityId, materialAssignmentId);
            AZ::SystemTickBus::Handler::BusConnect();
        }

        QPixmap EditorMaterialSystemComponent::GetRenderedMaterialPreview(
            const AZ::EntityId& entityId, const JE::Render::MaterialAssignmentId& materialAssignmentId) const
        {
            const auto& itr1 = m_materialPreviews.find(entityId);
            if (itr1 != m_materialPreviews.end())
            {
                const auto& itr2 = itr1->second.find(materialAssignmentId);
                if (itr2 != itr1->second.end())
                {
                    return itr2->second;
                }
            }
            return QPixmap();
        }

        void EditorMaterialSystemComponent::OnEntityDeactivated(const AZ::EntityId& entityId)
        {
            // Deleting any preview saved for an entity that is about to be deactivated
            m_materialPreviews.erase(entityId);
        }

        void EditorMaterialSystemComponent::OnSystemTick()
        {
            if (m_materialPreviewModelAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::NotLoaded)
            {
                m_materialPreviewModelAsset.QueueLoad();
                return;
            }

            if (m_materialPreviewLightingPresetAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::NotLoaded)
            {
                m_materialPreviewModelAsset.QueueLoad();
                return;
            }

            auto previewRenderer = AZ::Interface<JE::Editor::Tools::PreviewRendererInterface>::Get();
            if (!previewRenderer || !m_materialPreviewModelAsset.IsReady() || !m_materialPreviewLightingPresetAsset.IsReady())
            {
                return;
            }

            for (const auto& materialPreviewRequestPair : m_materialPreviewRequests)
            {
                const auto& entityId = materialPreviewRequestPair.first;
                const auto& materialAssignmentId = materialPreviewRequestPair.second;

                AZ::Data::AssetId materialAssetId = {};
                JE::Render::MaterialComponentRequestBus::EventResult(
                    materialAssetId, entityId, &JE::Render::MaterialComponentRequestBus::Events::GetMaterialAssetId, materialAssignmentId);

                JE::Render::MaterialPropertyOverrideMap propertyOverrides;
                JE::Render::MaterialComponentRequestBus::EventResult(
                    propertyOverrides, entityId, &JE::Render::MaterialComponentRequestBus::Events::GetPropertyValues, materialAssignmentId);

                // Having an invalid material asset will use the default asset on the model.
                AZ::Data::Asset<JE::Render::RPI::MaterialAsset> materialAsset;
                materialAsset.Create(materialAssetId);

                previewRenderer->AddCaptureRequest(
                    { MaterialPreviewResolution,
                      AZStd::make_shared<JE::Editor::SharedPreviewContent>(
                          previewRenderer->GetScene(),
                          previewRenderer->GetView(),
                          previewRenderer->GetEntityContextId(),
                          m_materialPreviewModelAsset,
                          materialAsset,
                          m_materialPreviewLightingPresetAsset,
                          propertyOverrides),
                      [entityId, materialAssignmentId]()
                      {
                          AZ_UNUSED(entityId);
                          AZ_UNUSED(materialAssignmentId);

                          AZ_Warning(
                              "EditorMaterialSystemComponent",
                              false,
                              "RenderMaterialPreview capture failed for entity %s slot %s.",
                              entityId.ToString().c_str(),
                              materialAssignmentId.ToString().c_str());

                          // If the capture failed to render substitute it with a black image
                          QPixmap pixmap(1, 1);
                          pixmap.fill(Qt::black);
                          JE::Editor::EditorMaterialSystemComponentNotificationBus::Broadcast(
                              &JE::Editor::EditorMaterialSystemComponentNotificationBus::Events::OnRenderMaterialPreviewRendered,
                              entityId,
                              materialAssignmentId,
                              pixmap);
                      },
                      [entityId, materialAssignmentId](const QPixmap& pixmap)
                      {
                          JE::Editor::EditorMaterialSystemComponentNotificationBus::Broadcast(
                              &JE::Editor::EditorMaterialSystemComponentNotificationBus::Events::OnRenderMaterialPreviewRendered,
                              entityId,
                              materialAssignmentId,
                              pixmap);
                      } });
            }

            m_materialPreviewRequests.clear();
            AZ::SystemTickBus::Handler::BusDisconnect();
        }

        void EditorMaterialSystemComponent::OnRenderMaterialPreviewRendered(
            [[maybe_unused]] const AZ::EntityId& entityId,
            [[maybe_unused]] const JE::Render::MaterialAssignmentId& materialAssignmentId,
            [[maybe_unused]] const QPixmap& pixmap)
        {
            // Since the "preview rendered" event is not called on the main thread, queue
            // the handling code to be executed on the main thread. This prevents any non
            // thread-safe code, such as Qt updates, from running on alternate threads
            AZ::SystemTickBus::QueueFunction(
                [this, pixmap, entityId, materialAssignmentId]()
                {
                    PurgePreviews();

                    // Caching preview so the image will not have to be regenerated every time it's requested
                    m_materialPreviews[entityId][materialAssignmentId] = pixmap;

                    JE::Editor::EditorMaterialSystemComponentNotificationBus::Broadcast(
                        &JE::Editor::EditorMaterialSystemComponentNotificationBus::Events::OnRenderMaterialPreviewReady,
                        entityId,
                        materialAssignmentId,
                        pixmap);
                });
        }

        void EditorMaterialSystemComponent::OnMaterialSlotLayoutChanged()
        {
            // Deleting any preview saved for an entity whose material configuration is about to be invalidated
            const AZ::EntityId entityId = *JE::Render::MaterialComponentNotificationBus::GetCurrentBusId();
            m_materialPreviews.erase(entityId);
        }

        void EditorMaterialSystemComponent::NotifyRegisterViews()
        {
            AzToolsFramework::ViewPaneOptions workspaceOptions;
            workspaceOptions.canHaveMultipleInstances = false;
            workspaceOptions.preferedDockingArea = Qt::RightDockWidgetArea;
            workspaceOptions.paneRect = QRect(50, 50, 600, 1000);
            AzToolsFramework::RegisterViewPane<MaterialWorkspaceWidget>(MaterialWorkspaceWidget::PaneName,
                LyViewPane::CategoryTools, workspaceOptions, [this](QWidget* parent) -> QWidget*
                {
                    return new MaterialWorkspaceWidget(*m_workspaceSession, parent);
                });
            m_workspacePaneRegistered = true;

            AzToolsFramework::ViewPaneOptions graphOptions;
            graphOptions.canHaveMultipleInstances = false;
            graphOptions.preferedDockingArea = Qt::BottomDockWidgetArea;
            graphOptions.paneRect = QRect(50, 50, 1200, 700);
            AzToolsFramework::RegisterViewPane<MaterialGraphWorkspaceWidget>(MaterialGraphWorkspaceWidget::PaneName,
                LyViewPane::CategoryTools, graphOptions, [this](QWidget* parent) -> QWidget*
                {
                    return new MaterialGraphWorkspaceWidget(*m_workspaceSession, parent);
                });
            m_graphWorkspacePaneRegistered = true;

            AzToolsFramework::ViewPaneOptions inspectorOptions;
            inspectorOptions.canHaveMultipleInstances = true;
            inspectorOptions.preferedDockingArea = Qt::NoDockWidgetArea;
            inspectorOptions.paneRect = QRect(50, 50, 600, 1000);
            inspectorOptions.showInMenu = false;
            inspectorOptions.showOnToolsToolbar = false;
            AzToolsFramework::RegisterViewPane<JE::Editor::EditorMaterialComponentInspector::MaterialPropertyInspector>(
                "Material Instance Editor", LyViewPane::CategoryTools, inspectorOptions);
        }
        
        void EditorMaterialSystemComponent::NotifyMainWindowInitialized(QMainWindow* mainWindow)
        {
            m_workspaceSession->SetMainWindow(mainWindow);
        }

        void EditorMaterialSystemComponent::AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList&)
        {
            if (newlySelectedEntities.size() == 1)
            {
                JE::Render::Debug::AtomImGuiToolsRequestBus::Broadcast(&JE::Render::Debug::AtomImGuiToolsRequests::ShowMaterialShaderDetailsForEntity, newlySelectedEntities[0], false);
            }
            else
            {
                JE::Render::Debug::AtomImGuiToolsRequestBus::Broadcast(&JE::Render::Debug::AtomImGuiToolsRequests::ShowMaterialShaderDetailsForEntity, AZ::EntityId{}, false);
            }
        }

        AzToolsFramework::AssetBrowser::SourceFileDetails EditorMaterialSystemComponent::GetSourceFileDetails(
            const char* fullSourceFileName)
        {
            const AZStd::string_view path(fullSourceFileName);
            if (path.ends_with("physxmaterial") || path.ends_with("physicsmaterial"))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/PhysXMaterial_80.svg");
            }
            if (path.ends_with(JE::Render::RPI::MaterialSourceData::Extension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/Material_80.svg");
            }
            if (path.ends_with(JE::Render::RPI::MaterialTypeSourceData::Extension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/MaterialType_80.svg");
            }
            if (path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphExtensionWithDot) ||
                path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphNodeExtensionWithDot) ||
                path.ends_with(JE::Editor::EditorMaterialComponentUtil::MaterialGraphTemplateExtensionWithDot) ||
                path.ends_with(".passgraph") || path.ends_with(".passgraphtemplate") || path.ends_with(".passgraphnode"))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/MaterialGraph_80.svg");
            }
            if (path.ends_with(JE::Render::RPI::BufferAsset::Extension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/BufferAsset_80.svg");
            }
            if (path.ends_with(JE::Render::RPI::ShaderAsset::Extension))
            {
                return AzToolsFramework::AssetBrowser::SourceFileDetails(":/Icons/Shader_80.svg");
            }
            return AzToolsFramework::AssetBrowser::SourceFileDetails();
        }

        void EditorMaterialSystemComponent::OnActionRegistrationHook()
        {
            auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            AZ_Assert(actionManagerInterface, "EditorMaterialSystemComponent - could not get ActionManagerInterface");

            auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
            AZ_Assert(hotKeyManagerInterface, "EditorMaterialSystemComponent - could not get HotKeyManagerInterface");

            {
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = QObject::tr("Material Editor").toUtf8().constData();
                actionProperties.m_iconPath = ":/Menu/material_editor.svg";

                auto outcome = actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    MaterialEditorActionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        OpenMaterialEditor("");
                    }
                );
                AZ_Assert(outcome.IsSuccess(), "Failed to RegisterAction %s", MaterialEditorActionIdentifier.data());

                hotKeyManagerInterface->SetActionHotKey(MaterialEditorActionIdentifier, "Ctrl+M");
            }

            {
                AzToolsFramework::ActionProperties actionProperties;
                actionProperties.m_name = QObject::tr("Graph Workspace").toUtf8().constData();
                actionProperties.m_iconPath = ":/Menu/material_canvas.svg";

                auto outcome = actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    MaterialCanvasActionIdentifier,
                    actionProperties,
                    [this]()
                    {
                        OpenMaterialCanvas("");
                    });
                AZ_Assert(outcome.IsSuccess(), "Failed to RegisterAction %s", MaterialCanvasActionIdentifier.data());

                outcome = hotKeyManagerInterface->SetActionHotKey(MaterialCanvasActionIdentifier, "Ctrl+Shift+M");
                AZ_Assert(outcome.IsSuccess(), "Failed to ActionHotKey for %s", MaterialCanvasActionIdentifier.data());
            }

        }

        void EditorMaterialSystemComponent::OnMenuBindingHook()
        {
            auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            AZ_Assert(actionManagerInterface, "EditorMaterialSystemComponent - could not get ActionManagerInterface");

            auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
            AZ_Assert(menuManagerInterface, "EditorMaterialSystemComponent - could not get MenuManagerInterface");

            {
                auto outcome = menuManagerInterface->AddActionToMenu(
                    EditorIdentifiers::ToolsMenuIdentifier,
                    MaterialEditorActionIdentifier,
                    actionManagerInterface->GenerateActionAlphabeticalSortKey(MaterialEditorActionIdentifier));
                AZ_Assert(
                    outcome.IsSuccess(),
                    "Failed to AddAction %s to Menu %s",
                    MaterialEditorActionIdentifier.data(),
                    EditorIdentifiers::ToolsMenuIdentifier.data());
            }

            {
                auto outcome = menuManagerInterface->AddActionToMenu(
                    EditorIdentifiers::ToolsMenuIdentifier,
                    MaterialCanvasActionIdentifier,
                    actionManagerInterface->GenerateActionAlphabeticalSortKey(MaterialCanvasActionIdentifier));
                AZ_Assert(
                    outcome.IsSuccess(),
                    "Failed to AddAction %s to Menu %s",
                    MaterialCanvasActionIdentifier.data(),
                    EditorIdentifiers::ToolsMenuIdentifier.data());
            }
        }

        void EditorMaterialSystemComponent::PurgePreviews()
        {
            // Deleting all saved previews after a certain threshold has been reached
            size_t materialPreviewCount = 0;
            for (const auto& materialPreviewPair : m_materialPreviews)
            {
                materialPreviewCount += materialPreviewPair.second.size();
            }

            if (materialPreviewCount > MaterialPreviewLimit)
            {
                m_materialPreviews.clear();
            }
        }
    } // namespace Editor
} // namespace JE
