/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "Workspace/MaterialWorkspaceSession.h"
#include "Workspace/MaterialWorkspaceWidget.h"
#include <JE/Editor/Materials/MaterialBrowserInteractions.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialUtils.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertyValueSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "MaterialTextureSet.h"
#include <QLabel>
#include <QPushButton>
#include <JE/Editor/Tools/Util/Util.h>
#include <Atom/ImageProcessing/ImageProcessingEditorBus.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <QFileInfo>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Shader/ShaderSourceData.h>

#include <JE/Editor/Tools/Document/AtomToolsAnyDocument.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentInspector.h>
#include <QSplitter>
#include <JE/Editor/Tools/Document/CreateDocumentDialog.h>
#include <JE/Editor/Tools/AssetSelection/AssetSelectionComboBox.h>
#include <JE/Editor/Tools/Inspector/InspectorWidget.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentSystemRequestBus.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodeConfig.h>
#include <Tests/AssetSystemMocks.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzTest/AzTest.h>
#include <QApplication>
#include <QMainWindow>
#include <QPointer>
#include <QAbstractButton>
#include <QMessageBox>
#include <QDialog>
#include <QTimer>
#include <QTemporaryDir>
#include <QImage>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QScopeGuard>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesManager.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <QAction>
#include <QMenu>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMimeData>
#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <JE/Render/Materials/MaterialComponentController.h>
#include <JE/Render/Materials/MaterialComponentConstants.h>
#include <JE/Editor/Materials/EditorMaterialComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionComponent.h>

namespace UnitTest
{
    class MaterialWorkspaceTests : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            if (!QApplication::instance())
            {
                m_qtApplication = AZStd::make_unique<QApplication>(m_argc, m_argv);
            }
            m_application = AZStd::make_unique<AZ::ComponentApplication>();
            AZ::ComponentApplication::Descriptor descriptor;
            descriptor.m_useExistingAllocator = true;
            AZ::ComponentApplication::StartupParameters startup;
            startup.m_loadDynamicModules = false;
            startup.m_loadStaticModules = false;
            startup.m_loadSettingsRegistry = false;
            startup.m_createEditContext = true;
            m_application->Create(descriptor, startup);
            // This isolated application skips static modules, including the native JSON serializers.
            AZ::ReflectionEnvironment::GetReflectionManager()->Reflect(
                azrtti_typeid<AZ::JsonSystemComponent>(), &AZ::JsonSystemComponent::Reflect);
            JE::Editor::Tools::InspectorWidget::Reflect(m_application->GetSerializeContext());
        }

        void TearDown() override
        {
            m_application->Destroy();
            m_application.reset();
            m_qtApplication.reset();
            LeakDetectionFixture::TearDown();
        }

        JE::Editor::Tools::DocumentTypeInfo EmptyDocumentType()
        {
            JE::Editor::Tools::DocumentTypeInfo type;
            type.m_documentTypeName = "Workspace lifecycle";
            type.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& info)
            {
                return aznew JE::Editor::Tools::AtomToolsDocument(toolId, info);
            };
            return type;
        }

        AZ::Uuid CreateDocument(const JE::Editor::MaterialWorkspaceSession& session)
        {
            AZ::Uuid id;
            JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(id, session.GetToolId(),
                &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CreateDocumentFromTypeName,
                session.GetDocumentType().m_documentTypeName);
            return id;
        }

        JE::Editor::Tools::DocumentTypeInfo EditableDocumentType()
        {
            // Use native document editing/serialization to test session lifetime without an asset/GPU pipeline.
            return JE::Editor::Tools::AtomToolsAnyDocument::BuildDocumentTypeInfo(
                "Workspace value", {"workspace"}, {}, AZStd::any(0.25f), AZ::Uuid::CreateNull());
        }

        void Edit(const AZ::Uuid& id)
        {
            bool success = false;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
            ASSERT_TRUE(success);
            JE::Editor::Tools::DocumentObjectInfoVector objects;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                objects, id, &JE::Editor::Tools::AtomToolsDocumentRequests::GetObjectInfo);
            ASSERT_EQ(objects.size(), 1);
            ASSERT_EQ(objects.front().m_objectType, azrtti_typeid<float>());
            *static_cast<float*>(objects.front().m_objectPtr) = 0.75f;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
            ASSERT_TRUE(success);
        }

        bool IsModified(const AZ::Uuid& id)
        {
            bool modified = false;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                modified, id, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
            return modified;
        }

        void ChooseCloseDecision(QMessageBox::StandardButton decision, bool& observed)
        {
            // Interact with the real modal dialog in its nested Qt event loop, not a mocked close result.
            QTimer::singleShot(0, qApp, [decision, &observed]()
            {
                auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
                observed = dialog && dialog->windowTitle() == "Material Changes";
                EXPECT_TRUE(observed);
                if (dialog)
                {
                    if (auto* button = dialog->button(decision))
                    {
                        button->click();
                    }
                    else
                    {
                        dialog->reject();
                    }
                }
            });
        }

        int m_argc = 1;
        char m_name[22] = "MaterialWorkspaceTest";
        char* m_argv[2] = {m_name, nullptr};
        AZStd::unique_ptr<QApplication> m_qtApplication;
        AZStd::unique_ptr<AZ::ComponentApplication> m_application;
    };


    namespace
    {
        // Represents the Scene Settings provider's existing window-returning boundary, not its import/document logic.
        class SceneSettingsResultProviderForTest final
            : public AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler
        {
        public:
            SceneSettingsResultProviderForTest() : m_window(new QMainWindow) {}
            ~SceneSettingsResultProviderForTest() override { BusDisconnect(); delete m_window.data(); }
            QMainWindow* GetSceneSettings() override { ++m_requests; return m_window.data(); }
            QPointer<QMainWindow> m_window;
            int m_requests = 0;
        };
        // Real non-Scene handlers inherit these nullptr/false defaults (e.g. the Inspector window/material provider).
        class DefaultSceneSettingsObserverForTest final
            : public AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler {};
    }

    TEST_F(MaterialWorkspaceTests, SceneSettingsWindowSurvivesDefaultObserversInBothConnectionOrders)
    {
        for (bool providerFirst : {false, true})
        {
            SceneSettingsResultProviderForTest provider;
            DefaultSceneSettingsObserverForTest observer;
            if (providerFirst) { provider.BusConnect(); observer.BusConnect(); }
            else { observer.BusConnect(); provider.BusConnect(); }
            {
                // Constructor, public bus, real stacked-widget attachment and QObject ownership; no private-member seam.
                AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget inspector;
                ASSERT_FALSE(provider.m_window.isNull());
                EXPECT_TRUE(inspector.isAncestorOf(provider.m_window.data())) << "providerFirst=" << providerFirst;
                EXPECT_EQ(provider.m_requests, 1) << "Aggregation must not query/create a second window.";
            }
            EXPECT_TRUE(provider.m_window.isNull()) << "Attached Scene Settings must follow the Inspector QObject lifetime.";
        }
    }

    TEST_F(MaterialWorkspaceTests, DefaultSceneSettingsObserversDoNotCreatePlaceholderWindows)
    {
        DefaultSceneSettingsObserverForTest observer; observer.BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget inspector;
        EXPECT_TRUE(inspector.findChildren<QMainWindow*>().isEmpty());
    }

    TEST_F(MaterialWorkspaceTests, ColorPickerAlphaResourceIsARealImage)
    {
        // The production resource must contain the original PNG, not its LFS pointer.
        const QImage background(":/stylesheet/img/UI20/alpha-background.png");
        ASSERT_FALSE(background.isNull());
        EXPECT_GT(background.width(), 0);
        EXPECT_GT(background.height(), 0);
        const auto cached = AzQtComponents::Style::cachedPixmap(":/stylesheet/img/UI20/alpha-background.png");
        ASSERT_FALSE(cached.isNull());
        EXPECT_EQ(cached.toImage().convertToFormat(QImage::Format_ARGB32), background.convertToFormat(QImage::Format_ARGB32));
    }

    TEST_F(MaterialWorkspaceTests, NativeAlphaBrushComposesColorsWithoutMutatingTheCachedBackground)
    {
        const QString resource = ":/stylesheet/img/UI20/alpha-background.png";
        const QImage background = AzQtComponents::Style::cachedPixmap(resource).toImage().convertToFormat(QImage::Format_ARGB32);
        ASSERT_FALSE(background.isNull()); // Do not paint a missing resource; the UI dump already records that failure.
        const auto transparent = AzQtComponents::MakeAlphaBrush(QColor(0, 0, 0, 0)).texture().toImage().convertToFormat(QImage::Format_ARGB32);
        EXPECT_EQ(transparent, background);
        for (const QColor color : {QColor(255, 0, 0), QColor(0, 0, 255)})
        {
            const auto brush = AzQtComponents::MakeAlphaBrush(color);
            const QImage image = brush.texture().toImage();
            ASSERT_FALSE(image.isNull());
            ASSERT_EQ(image.size(), background.size());
            for (int y = 0; y < image.height(); ++y)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    EXPECT_EQ(image.pixelColor(x, y), color);
                }
            }
            EXPECT_EQ(AzQtComponents::Style::cachedPixmap(resource).toImage().convertToFormat(QImage::Format_ARGB32), background);
        }
    }

    TEST_F(MaterialWorkspaceTests, BareInspectorDestruction)
    {
        auto* inspector = new JE::Editor::Tools::AtomToolsDocumentInspector(JE::Editor::MaterialWorkspaceSession::GetToolId());
        delete inspector;
    }

    TEST_F(MaterialWorkspaceTests, SelectedInspectorDestruction)
    {
        JE::Editor::MaterialWorkspaceSession session(EmptyDocumentType());
        const auto id = CreateDocument(session);
        ASSERT_FALSE(id.IsNull());
        auto* inspector = new JE::Editor::Tools::AtomToolsDocumentInspector(JE::Editor::MaterialWorkspaceSession::GetToolId());
        inspector->SetDocumentSettingsPrefix("/JE/Editor/MaterialWorkspace/Inspector");
        inspector->SetDocumentId(id);
        inspector->SetDocumentId(AZ::Uuid::CreateNull());
        delete inspector;
    }

    TEST_F(MaterialWorkspaceTests, ClosingAndRecreatingPaneRetainsTheNativeDocument)
    {
        JE::Editor::MaterialWorkspaceSession session(EmptyDocumentType());
        const auto id = CreateDocument(session);
        ASSERT_FALSE(id.IsNull());
        ASSERT_TRUE(session.SelectDocument(id));
        {
            JE::Editor::MaterialWorkspaceWidget view(session);
            view.close();
        }
        ASSERT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
        ASSERT_EQ(session.GetDocumentIds().size(), 1);
        EXPECT_EQ(session.GetActiveDocumentId(), id);
        JE::Editor::MaterialWorkspaceWidget reopened(session);
        EXPECT_EQ(session.GetActiveDocumentId(), id);
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
    }

    TEST_F(MaterialWorkspaceTests, HiddenModifiedDocumentCanCancelMainWindowClose)
    {
        JE::Editor::MaterialWorkspaceSession session(EditableDocumentType());
        const auto id = CreateDocument(session);
        ASSERT_FALSE(id.IsNull());
        Edit(id);
        ASSERT_TRUE(IsModified(id));
        QWidget mainWindow;
        session.SetMainWindow(&mainWindow);
        mainWindow.show();
        bool observed = false;
        ChooseCloseDecision(QMessageBox::Cancel, observed);
        EXPECT_FALSE(mainWindow.close());
        EXPECT_TRUE(observed);
        EXPECT_TRUE(mainWindow.isVisible());
        EXPECT_TRUE(IsModified(id));
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
    }

    TEST_F(MaterialWorkspaceTests, DiscardApprovalKeepsDocumentAndHistoryUntilActualShutdown)
    {
        JE::Editor::MaterialWorkspaceSession session(EditableDocumentType());
        const auto id = CreateDocument(session);
        ASSERT_FALSE(id.IsNull());
        Edit(id);
        bool observed = false;
        ChooseCloseDecision(QMessageBox::Discard, observed);
        EXPECT_TRUE(session.ConfirmApplicationClose(nullptr));
        EXPECT_TRUE(observed);
        EXPECT_TRUE(IsModified(id));
        bool canUndo = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            canUndo, id, &JE::Editor::Tools::AtomToolsDocumentRequests::CanUndo);
        EXPECT_TRUE(canUndo);
        JE::Editor::Tools::DocumentObjectInfoVector objects;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            objects, id, &JE::Editor::Tools::AtomToolsDocumentRequests::GetObjectInfo);
        ASSERT_EQ(objects.size(), 1);
        EXPECT_FLOAT_EQ(*static_cast<const float*>(objects.front().m_objectPtr), 0.75f);
        // A later close attempt can still be cancelled after another editor pane vetoes exit.
        observed = false;
        ChooseCloseDecision(QMessageBox::Cancel, observed);
        EXPECT_FALSE(session.ConfirmApplicationClose(nullptr));
        EXPECT_TRUE(observed);
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
    }

    TEST_F(MaterialWorkspaceTests, SessionClosesBeforeDestroyingItsDocuments)
    {
        class CloseRecorder : public JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler
        {
        public:
            CloseRecorder() { BusConnect(JE::Editor::MaterialWorkspaceSession::GetToolId()); }
            ~CloseRecorder() override { BusDisconnect(); }
            void OnDocumentClosed(const AZ::Uuid& id) override
            {
                EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
                m_events.push_back("closed");
            }
            void OnDocumentDestroyed(const AZ::Uuid&) override { m_events.push_back("destroyed"); }
            AZStd::vector<AZStd::string> m_events;
        } recorder;
        AZ::Uuid id;
        {
            JE::Editor::MaterialWorkspaceSession session(EmptyDocumentType());
            id = CreateDocument(session);
            ASSERT_FALSE(id.IsNull());
        }
        ASSERT_EQ(recorder.m_events.size(), 2);
        EXPECT_EQ(recorder.m_events[0], "closed");
        EXPECT_EQ(recorder.m_events[1], "destroyed");
        EXPECT_FALSE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
    }

    TEST_F(MaterialWorkspaceTests, NativeNodeAndShaderDocumentsShareTheRetainedSession)
    {
        JE::Editor::MaterialWorkspaceSession session(EmptyDocumentType());
        const JE::Editor::Tools::DocumentTypeInfoVector types = {
            JE::Editor::Tools::AtomToolsAnyDocument::BuildDocumentTypeInfo("Material Graph Node Config",
                {"materialgraphnode"}, {"materialgraphnodetemplate"}, AZStd::any(JE::Editor::Tools::DynamicNodeConfig()), AZ::Uuid::CreateNull()),
            JE::Editor::Tools::AtomToolsAnyDocument::BuildDocumentTypeInfo("Shader Source Data",
                {"shader"}, {}, AZStd::any(JE::Render::RPI::ShaderSourceData()), azrtti_typeid<JE::Render::RPI::ShaderSourceData>())};
        session.RegisterGraphDocumentTypes(types);
        session.RegisterGraphDocumentTypes(types);
        ASSERT_EQ(session.GetGraphDocumentTypes().size(), 2);
        const auto materialId = CreateDocument(session);
        ASSERT_FALSE(materialId.IsNull());
        EXPECT_FALSE(session.IsGraphDocument(materialId));
        const AZ::Uuid expectedTypes[] = {azrtti_typeid<JE::Editor::Tools::DynamicNodeConfig>(), azrtti_typeid<JE::Render::RPI::ShaderSourceData>()};
        AZ::Uuid ids[2];
        for (size_t index = 0; index < types.size(); ++index)
        {
            JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(ids[index], session.GetToolId(),
                &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CreateDocumentFromTypeName, types[index].m_documentTypeName);
            ASSERT_FALSE(ids[index].IsNull());
            EXPECT_TRUE(session.IsGraphDocument(ids[index]));
            JE::Editor::Tools::DocumentObjectInfoVector objects;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(objects, ids[index],
                &JE::Editor::Tools::AtomToolsDocumentRequests::GetObjectInfo);
            ASSERT_EQ(objects.size(), 1);
            EXPECT_EQ(objects.front().m_objectType, expectedTypes[index]);
            EXPECT_NE(objects.front().m_objectPtr, nullptr);
        }
        EXPECT_NE(ids[0], ids[1]);
        EXPECT_EQ(session.GetDocumentIds().size(), 3);
        AZ::u32 count = 0;
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(count, session.GetToolId(),
            &JE::Editor::Tools::AtomToolsDocumentSystemRequests::GetDocumentCount);
        EXPECT_EQ(count, 3);
        ASSERT_TRUE(session.SelectDocument(ids[0]));
        ASSERT_TRUE(session.SelectDocument(ids[1]));
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(ids[0]));
        EXPECT_EQ(session.GetActiveDocumentId(), ids[1]);
    }

    TEST_F(MaterialWorkspaceTests, CompilationStateOutlivesObserversAndRetainsEditsDuringTheJob)
    {
        using Session = JE::Editor::MaterialWorkspaceSession;
        using State = Session::CompileState;
        JE::Editor::MaterialWorkspaceSession session(EmptyDocumentType());
        const auto type = EditableDocumentType();
        session.RegisterGraphDocumentTypes({type});
        AZ::Uuid id;
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(id, session.GetToolId(),
            &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CreateDocumentFromTypeName, type.m_documentTypeName);
        ASSERT_FALSE(id.IsNull());
        int notifications = 0;
        {
            QWidget observer;
            QObject::connect(&session, &Session::CompileStateChanged, &observer, [&](const AZ::Uuid& changedId)
            {
                EXPECT_EQ(changedId, id);
                EXPECT_EQ(session.GetCompileState(id), State::Compiling);
                ++notifications;
            });
            JE::Editor::Tools::GraphDocumentNotificationBus::Event(session.GetToolId(),
                &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphStarted, id);
            EXPECT_EQ(session.GetCompileState(id), State::Compiling);
            EXPECT_EQ(notifications, 1);
        }
        // The observer is gone. Edit the real native document, then deliver the job's completion contract.
        Edit(id);
        EXPECT_EQ(session.GetCompileState(id), State::CompilingModified);
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(session.GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphCompleted, id);
        EXPECT_EQ(session.GetCompileState(id), State::Modified);
        EXPECT_EQ(notifications, 1); // QObject context destruction disconnected the old pane observer.

        QWidget reopenedObserver;
        State displayed = session.GetCompileState(id);
        EXPECT_EQ(displayed, State::Modified);
        QObject::connect(&session, &Session::CompileStateChanged, &reopenedObserver, [&](const AZ::Uuid& changedId)
        {
            EXPECT_EQ(changedId, id);
            displayed = session.GetCompileState(id);
        });
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(session.GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphStarted, id);
        EXPECT_EQ(displayed, State::Compiling);
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(session.GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphFailed, id);
        EXPECT_EQ(displayed, State::Failed);
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(session.GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphStarted, id);
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(session.GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphCompleted, id);
        EXPECT_EQ(displayed, State::Completed);
    }

    TEST_F(MaterialWorkspaceTests, DestroyedDocumentRejectsLateCompilationNotifications)
    {
        using Session = JE::Editor::MaterialWorkspaceSession;
        using State = Session::CompileState;
        JE::Editor::MaterialWorkspaceSession session(EmptyDocumentType());
        const auto type = EditableDocumentType();
        session.RegisterGraphDocumentTypes({type});
        AZ::Uuid id;
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(id, session.GetToolId(),
            &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CreateDocumentFromTypeName, type.m_documentTypeName);
        ASSERT_FALSE(id.IsNull());
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(session.GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphStarted, id);
        EXPECT_EQ(session.GetCompileState(id), State::Compiling);
        bool destroyed = false;
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(destroyed, session.GetToolId(),
            &JE::Editor::Tools::AtomToolsDocumentSystemRequests::DestroyDocument, id);
        ASSERT_TRUE(destroyed);
        EXPECT_EQ(session.GetCompileState(id), State::NotCompiled);
        EXPECT_TRUE(session.GetDocumentIds().empty());
        int notifications = 0;
        QWidget observer;
        QObject::connect(&session, &Session::CompileStateChanged, &observer, [&](const AZ::Uuid&) { ++notifications; });
        for (const auto event : {&JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphStarted,
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphCompleted,
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphFailed})
        {
            JE::Editor::Tools::GraphDocumentNotificationBus::Event(session.GetToolId(), event, id);
            EXPECT_EQ(session.GetCompileState(id), State::NotCompiled);
        }
        EXPECT_EQ(notifications, 0);
        EXPECT_FALSE(session.RequestCompile(id));
        EXPECT_FALSE(session.RequestCompile(AZ::Uuid::CreateNull()));
    }

    class MaterialSourceCreationTests : public MaterialWorkspaceTests
    {
    protected:
        void SetUp() override
        {
            MaterialWorkspaceTests::SetUp();
            if (!AZ::IO::FileIOBase::GetInstance())
            {
                m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
                AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            }
            // Like the RPI fixtures, install the real JSON serializers and source reflections, without initializing RPI/GPU.
            // Tracking them in the native manager also removes the JSON serializers before context destruction.
            auto* reflectionManager = AZ::ReflectionEnvironment::GetReflectionManager();
            reflectionManager->Reflect(azrtti_typeid<JE::Render::RPI::MaterialTypeSourceData>(), &JE::Render::RPI::MaterialTypeSourceData::Reflect);
            reflectionManager->Reflect(azrtti_typeid<JE::Render::RPI::MaterialSourceData>(), &JE::Render::RPI::MaterialSourceData::Reflect);
        }

        void TearDown() override
        {
            if (m_fileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
                m_fileIO.reset();
            }
            MaterialWorkspaceTests::TearDown();
        }

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
    };

    TEST_F(MaterialSourceCreationTests, TextureContextMenuOffersOneGenerateMaterialActionOnlyForImages)
    {
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        const QString imagePath = directory.filePath("rock_diff.png");
        QImage image(2, 2, QImage::Format_RGBA8888);
        image.fill(Qt::white);
        ASSERT_TRUE(image.save(imagePath, "PNG"));
        const AZStd::string textPath = directory.filePath("notes.txt").toUtf8().constData();
        ASSERT_TRUE(AZ::Utils::WriteFile("not a texture", textPath).IsSuccess());

        JE::Editor::MaterialBrowserInteractions interactions;
        QWidget caller;
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry texture;
        texture.SetFullPath(AZ::IO::Path(imagePath.toUtf8().constData()));
        QMenu textureMenu;
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications::AddContextMenuActions,
            &caller, &textureMenu,
            AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>{&texture});
        int generateActions = 0;
        for (const auto* action : textureMenu.actions())
        {
            if (action->text() == QString::fromUtf8("Generar material"))
            {
                ++generateActions;
                EXPECT_TRUE(action->isEnabled());
            }
        }
        EXPECT_EQ(generateActions, 1);

        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry text;
        text.SetFullPath(AZ::IO::Path(textPath));
        QMenu textMenu;
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications::AddContextMenuActions,
            &caller, &textMenu,
            AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>{&text});
        for (const auto* action : textMenu.actions())
        {
            EXPECT_NE(action->text(), QString::fromUtf8("Generar material"));
        }
    }

    class GraphDocumentCreationTests : public MaterialSourceCreationTests
    {
    protected:
        void SetUp() override
        {
            MaterialSourceCreationTests::SetUp();
            ASSERT_TRUE(m_directory.isValid());
            // Only the source-folder boundary is supplied. Document JSON, save/open and rename remain native.
            ON_CALL(m_assetSystem, GetAssetSafeFolders(testing::_)).WillByDefault(
                [this](AZStd::vector<AZStd::string>& folders)
                {
                    folders.push_back(m_directory.path().toUtf8().constData());
                    return true;
                });
            m_assetSystem.BusConnect();
        }
        void TearDown() override
        {
            m_assetSystem.BusDisconnect();
            MaterialSourceCreationTests::TearDown();
        }
        QTemporaryDir m_directory;
        testing::NiceMock<UnitTests::MockAssetSystemRequest> m_assetSystem;
    };

    // Exercise the real browser MIME decoder, ordered viewport bus and material controller.
    // Composition/permissions are boundaries only; this fixture does not provide editor entity Undo/Redo.
    class MaterialSurfaceDropTests : public MaterialWorkspaceTests
    {
    protected:
        using SlotId = JE::Render::MaterialAssignmentId;
        using MaterialMap = JE::Render::MaterialAssignmentMap;
        using Entry = AzToolsFramework::AssetBrowser::AssetBrowserEntry;

        class Surface final : public JE::Render::MaterialConsumerRequestBus::Handler
        {
        public:
            explicit Surface(AZ::EntityId id)
            {
                m_slots.emplace(SlotId::CreateFromStableIdOnly(11), JE::Render::MaterialAssignment{});
                m_slots.emplace(SlotId::CreateFromStableIdOnly(22), JE::Render::MaterialAssignment{});
                BusConnect(id);
            }
            ~Surface() override { BusDisconnect(); }
            SlotId FindMaterialAssignmentId(JE::Render::MaterialAssignmentLodIndex, const AZStd::string&) const override { return {}; }
            JE::Render::MaterialAssignmentLabelMap GetMaterialLabels() const override { return {}; }
            MaterialMap GetDefaultMaterialMap() const override { return m_slots; }
            AZStd::unordered_set<AZ::Name> GetModelUvNames() const override { return {}; }
            MaterialMap m_slots;
        };

        class DefaultDropRoute final : public AzQtComponents::DragAndDropEventsBus::Handler
        {
        public:
            DefaultDropRoute() { BusConnect(AzQtComponents::DragAndDropContexts::EditorViewport); }
            ~DefaultDropRoute() override { BusDisconnect(); }
            void Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase&) override
            {
                if (!event->isAccepted()) { ++m_unhandledDrops; }
            }
            int m_unhandledDrops = 0;
        };

        class EntityInfo : public AzToolsFramework::EditorEntityInfoRequestBus::Handler
        {
        public:
            explicit EntityInfo(AZ::EntityId id) { BusConnect(id); }
            ~EntityInfo() override { BusDisconnect(); }
            MOCK_CONST_METHOD0(GetParent, AZ::EntityId());
            MOCK_CONST_METHOD0(GetChildren, AzToolsFramework::EntityIdList());
            MOCK_CONST_METHOD1(GetChild, AZ::EntityId(AZStd::size_t));
            MOCK_CONST_METHOD0(GetChildCount, AZStd::size_t());
            MOCK_CONST_METHOD1(GetChildIndex, AZ::u64(AZ::EntityId));
            MOCK_CONST_METHOD0(GetName, AZStd::string());
            MOCK_CONST_METHOD0(GetSliceAssetName, AZStd::string());
            MOCK_CONST_METHOD0(IsSliceEntity, bool());
            MOCK_CONST_METHOD0(IsSubsliceEntity, bool());
            MOCK_CONST_METHOD0(IsSliceRoot, bool());
            MOCK_CONST_METHOD0(IsSubsliceRoot, bool());
            MOCK_CONST_METHOD0(HasSliceEntityAnyChildrenAddedOrDeleted, bool());
            MOCK_CONST_METHOD0(HasSliceEntityPropertyOverridesInTopLevel, bool());
            MOCK_CONST_METHOD0(HasSliceEntityOverrides, bool());
            MOCK_CONST_METHOD0(HasSliceChildrenOverrides, bool());
            MOCK_CONST_METHOD0(HasSliceAnyOverrides, bool());
            MOCK_CONST_METHOD0(HasCyclicDependency, bool());
            MOCK_METHOD1(AddToCyclicDependencyList, void(const AZ::EntityId&));
            MOCK_METHOD1(RemoveFromCyclicDependencyList, void(const AZ::EntityId&));
            MOCK_CONST_METHOD0(GetCyclicDependencyList, AzToolsFramework::EntityIdList());
            MOCK_CONST_METHOD0(GetIndexForSorting, AZ::u64());
            MOCK_CONST_METHOD0(IsSelected, bool());
            MOCK_CONST_METHOD0(IsVisible, bool());
            MOCK_CONST_METHOD0(IsHidden, bool());
            MOCK_CONST_METHOD0(IsLocked, bool());
            MOCK_CONST_METHOD0(GetStartStatus, AzToolsFramework::EditorEntityStartStatus());
            MOCK_CONST_METHOD0(IsJustThisEntityLocked, bool());
            MOCK_CONST_METHOD1(IsComponentExpanded, bool(AZ::ComponentId));
            MOCK_METHOD2(SetComponentExpanded, void(AZ::ComponentId, bool));
        };

        class ReadOnly final : public AzToolsFramework::ReadOnlyEntityPublicInterface
        {
        public:
            ReadOnly() { AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Register(this); }
            ~ReadOnly() { AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Unregister(this); }
            bool IsReadOnly(const AZ::EntityId& id) override { return id == AZ::EntityId(101); }
        };

        class Composition : public AzToolsFramework::EntityCompositionRequestBus::Handler
        {
        public:
            Composition() { BusConnect(); }
            ~Composition() override { BusDisconnect(); }
            MOCK_METHOD2(AddComponentsToEntities, AddComponentsOutcome(const AzToolsFramework::EntityIdList&, const AZ::ComponentTypeList&));
            MOCK_METHOD2(AddExistingComponentsToEntityById, AddExistingComponentsOutcome(const AZ::EntityId&, AZStd::span<AZ::Component* const>));
            MOCK_METHOD1(RemoveComponents, RemoveComponentsOutcome(AZStd::span<AZ::Component* const>));
            MOCK_METHOD1(ScrubEntities, ScrubEntitiesOutcome(const AzToolsFramework::EntityList&));
            MOCK_METHOD1(CutComponents, void(AZStd::span<AZ::Component* const>));
            MOCK_METHOD1(CopyComponents, void(AZStd::span<AZ::Component* const>));
            MOCK_METHOD1(PasteComponentsToEntity, void(AZ::EntityId));
            MOCK_METHOD0(HasComponentsToPaste, bool());
            MOCK_METHOD1(EnableComponents, void(AZStd::span<AZ::Component* const>));
            MOCK_METHOD1(DisableComponents, void(AZStd::span<AZ::Component* const>));
            MOCK_METHOD1(GetPendingComponentInfo, PendingComponentInfo(const AZ::Component*));
            MOCK_METHOD1(GetComponentName, AZStd::string(const AZ::Component*));
        };

        class CompositionWarnings final : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            CompositionWarnings() { BusConnect(); }
            ~CompositionWarnings() override { BusDisconnect(); }
            bool OnPreWarning(const char* window, const char*, int, const char*, const char* message) override
            {
                if (AZStd::string_view(window) == "MaterialBrowser" && AZStd::string_view(message) ==
                    "Cannot assign material: the surface has no active material component.")
                {
                    ++m_count;
                    return true;
                }
                return false;
            }
            int m_count = 0;
        };

        class PendingComponent final : public AZ::Component
        {
        public:
            AZ_COMPONENT(PendingComponent, "{34D32E99-6EBC-439F-8072-A66D60AA285E}");
            static void Reflect(AZ::ReflectContext*) {}
            void Activate() override {}
            void Deactivate() override {}
        };

        void SetUp() override
        {
            MaterialWorkspaceTests::SetUp();
            if (!AZ::IO::FileIOBase::GetInstance())
            {
                m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
                AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            }
            ASSERT_TRUE(m_directory.isValid());
            ASSERT_EQ(AzToolsFramework::AssetBrowser::EntryCache::GetInstance(), nullptr);
            m_root = AZStd::make_shared<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry>();
            m_model = AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserModel>();
            m_model->SetRootEntry(m_root);
            m_root->Update(m_directory.path().toUtf8().constData());
            AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry folder;
            folder.m_scanFolderID = 1;
            folder.m_scanFolder = m_directory.path().toUtf8().constData();
            folder.m_displayName = "Material drop fixture";
            folder.m_isRoot = true;
            m_root->AddScanFolder(folder);
            AzToolsFramework::AssetDatabase::FileDatabaseEntry file;
            file.m_fileID = 77;
            file.m_scanFolderPK = 1;
            file.m_fileName = "surface.material";
            file.m_isFolder = false;
            m_root->AddFile(file);
            AzToolsFramework::AssetBrowser::SourceWithFileID source;
            source.first = 77;
            source.second.m_sourceID = 77;
            source.second.m_scanFolderPK = 1;
            source.second.m_sourceName = "surface.material";
            source.second.m_sourceGuid = m_source;
            ASSERT_TRUE(m_root->AddSource(source));
            m_surface = AZStd::make_unique<Surface>(AZ::EntityId(101));
            m_otherSurface = AZStd::make_unique<Surface>(AZ::EntityId(202));
            // No tick is dispatched: native assignment queues loading but never starts asset/GPU work here.
            m_controller.Activate(AZ::EntityId(101));
            m_otherController.Activate(AZ::EntityId(202));
            m_controllerActive = true;
            m_otherControllerActive = true;
            m_context.m_hitEntityId = AZ::EntityId(101);
            m_context.m_hitMaterialSlotId = 11;
            m_context.m_hitLocation = AZ::Vector3::CreateZero();
            m_interactions = AZStd::make_unique<JE::Editor::MaterialBrowserInteractions>();
        }

        void TearDown() override
        {
            m_interactions.reset();
            if (m_controllerActive) { m_controller.Deactivate(); }
            if (m_otherControllerActive) { m_otherController.Deactivate(); }
            m_surface.reset();
            m_otherSurface.reset();
            m_model.reset();
            m_root.reset();
            AzToolsFramework::AssetBrowser::EntryCache::DestroyInstance();
            AzToolsFramework::AssetBrowser::AssetBrowserFavoritesManager::DestroyInstance();
            if (m_fileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
                m_fileIO.reset();
            }
            MaterialWorkspaceTests::TearDown();
        }

        const Entry* AddProduct(AZ::u32 subId, bool material = true)
        {
            AzToolsFramework::AssetBrowser::ProductWithUuid product;
            product.first = m_source;
            product.second.m_productID = subId;
            product.second.m_subID = subId;
            product.second.m_productName = AZStd::string::format("pc/surface%u.azmaterial", subId);
            product.second.m_assetType = material ? azrtti_typeid<JE::Render::RPI::MaterialAsset>() : AZ::Uuid::CreateRandom();
            EXPECT_TRUE(m_root->AddProduct(product));
            return AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry::GetProductByAssetId(AZ::Data::AssetId(m_source, subId));
        }

        Qt::DropAction Drop(const AZStd::vector<const Entry*>& entries, bool accepted = false,
            AzQtComponents::DragAndDropContextBase* context = nullptr)
        {
            QMimeData mime;
            AzToolsFramework::AssetBrowser::Utils::ToMimeData(&mime, entries);
            QDropEvent event(QPointF(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
            event.setAccepted(accepted);
            AzQtComponents::DragAndDropEventsBus::Event(AzQtComponents::DragAndDropContexts::EditorViewport,
                &AzQtComponents::DragAndDropEvents::Drop, &event, context ? *context : m_context);
            m_lastDropAccepted = event.isAccepted();
            return event.dropAction();
        }

        void ExpectUnchanged()
        {
            EXPECT_TRUE(m_controller.GetMaterialMap().empty());
            EXPECT_TRUE(m_otherController.GetMaterialMap().empty());
        }

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
        QTemporaryDir m_directory;
        const AZ::Uuid m_source = AZ::Uuid::CreateString("{F974D67E-772F-49A2-8718-0944A367329B}");
        AZStd::shared_ptr<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry> m_root;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserModel> m_model;
        AZStd::unique_ptr<JE::Editor::MaterialBrowserInteractions> m_interactions;
        AZStd::unique_ptr<Surface> m_surface;
        AZStd::unique_ptr<Surface> m_otherSurface;
        JE::Render::MaterialComponentController m_controller;
        JE::Render::MaterialComponentController m_otherController;
        bool m_controllerActive = false;
        bool m_otherControllerActive = false;
        bool m_lastDropAccepted = false;
        AzQtComponents::ViewportDragContext m_context;
    };

    TEST_F(MaterialSurfaceDropTests, DragEnterAndMoveDoNotAssignUntilDropThenOnlyHitSlotChanges)
    {
        const auto* product = AddProduct(1);
        ASSERT_NE(product, nullptr);
        QMimeData mime;
        AzToolsFramework::AssetBrowser::Utils::ToMimeData(&mime, {product});
        QDragEnterEvent enter(QPoint(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        enter.setAccepted(false);
        AzQtComponents::DragAndDropEventsBus::Event(AzQtComponents::DragAndDropContexts::EditorViewport,
            &AzQtComponents::DragAndDropEvents::DragEnter, &enter, m_context);
        EXPECT_TRUE(enter.isAccepted());
        EXPECT_EQ(enter.dropAction(), Qt::CopyAction);
        QDragMoveEvent move(QPoint(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        move.setAccepted(false);
        AzQtComponents::DragAndDropEventsBus::Event(AzQtComponents::DragAndDropContexts::EditorViewport,
            &AzQtComponents::DragAndDropEvents::DragMove, &move, m_context);
        EXPECT_TRUE(move.isAccepted());
        ExpectUnchanged();
        DefaultDropRoute fallback;
        EXPECT_EQ(Drop({product}), Qt::CopyAction);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_EQ(m_controller.GetMaterialMap().size(), 1);
        EXPECT_EQ(m_controller.GetMaterialAssetId(SlotId::CreateFromStableIdOnly(11)), AZ::Data::AssetId(m_source, 1));
        EXPECT_FALSE(m_controller.GetMaterialAssetId(SlotId::CreateFromStableIdOnly(22)).IsValid());
        EXPECT_TRUE(m_otherController.GetMaterialMap().empty());
        EXPECT_EQ(fallback.m_unhandledDrops, 0);
    }

    TEST_F(MaterialSurfaceDropTests, MissingHitMissingSlotAndStaleSlotConsumeWithoutAssigningOrDefaultDrop)
    {
        const auto* product = AddProduct(1);
        ASSERT_NE(product, nullptr);
        DefaultDropRoute fallback;
        m_context.m_hitEntityId = AZ::EntityId{};
        EXPECT_EQ(Drop({product}), Qt::IgnoreAction);
        EXPECT_TRUE(m_lastDropAccepted);
        m_context.m_hitEntityId = AZ::EntityId(101);
        m_context.m_hitMaterialSlotId.reset();
        EXPECT_EQ(Drop({product}), Qt::IgnoreAction);
        EXPECT_TRUE(m_lastDropAccepted);
        m_context.m_hitMaterialSlotId = 99;
        EXPECT_EQ(Drop({product}), Qt::IgnoreAction);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_EQ(fallback.m_unhandledDrops, 0);
        ExpectUnchanged();
    }

    TEST_F(MaterialSurfaceDropTests, MultipleProductsOrAmbiguousSourceConsumeWithoutAssignment)
    {
        const auto* first = AddProduct(1);
        const auto* second = AddProduct(2);
        ASSERT_NE(first, nullptr);
        ASSERT_NE(second, nullptr);
        DefaultDropRoute fallback;
        EXPECT_EQ(Drop({first, second}), Qt::IgnoreAction);
        EXPECT_TRUE(m_lastDropAccepted);
        const auto* source = AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::GetSourceByUuid(m_source);
        ASSERT_NE(source, nullptr);
        EXPECT_EQ(Drop({source}), Qt::IgnoreAction);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_EQ(fallback.m_unhandledDrops, 0);
        ExpectUnchanged();
    }

    TEST_F(MaterialSurfaceDropTests, MaterialSourceWithoutCookedProductConsumesWithoutAssignment)
    {
        const auto* source = AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::GetSourceByUuid(m_source);
        ASSERT_NE(source, nullptr);
        AZStd::vector<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*> products;
        source->GetChildrenRecursively(products);
        ASSERT_TRUE(products.empty());
        DefaultDropRoute fallback;
        EXPECT_EQ(Drop({source}), Qt::IgnoreAction);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_EQ(fallback.m_unhandledDrops, 0);
        ExpectUnchanged();
    }

    TEST_F(MaterialSurfaceDropTests, ModelSourceWithMaterialProductPreservesDefaultDropButExplicitMaterialCanAssign)
    {
        AzToolsFramework::AssetDatabase::FileDatabaseEntry file;
        file.m_fileID = 88;
        file.m_scanFolderPK = 1;
        file.m_fileName = "mesh.fbx";
        file.m_isFolder = false;
        m_root->AddFile(file);
        AzToolsFramework::AssetBrowser::SourceWithFileID sourceData;
        sourceData.first = 88;
        sourceData.second.m_sourceID = 88;
        sourceData.second.m_scanFolderPK = 1;
        sourceData.second.m_sourceName = "mesh.fbx";
        sourceData.second.m_sourceGuid = AZ::Uuid::CreateRandom();
        ASSERT_TRUE(m_root->AddSource(sourceData));
        AzToolsFramework::AssetBrowser::ProductWithUuid productData;
        productData.first = sourceData.second.m_sourceGuid;
        productData.second.m_productID = 88;
        productData.second.m_subID = 88;
        productData.second.m_productName = "pc/mesh.azmaterial";
        productData.second.m_assetType = azrtti_typeid<JE::Render::RPI::MaterialAsset>();
        ASSERT_TRUE(m_root->AddProduct(productData));
        const auto* source = AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::GetSourceByUuid(productData.first);
        ASSERT_NE(source, nullptr);
        DefaultDropRoute fallback;
        Drop({source});
        EXPECT_FALSE(m_lastDropAccepted);
        EXPECT_EQ(fallback.m_unhandledDrops, 1);
        ExpectUnchanged();
        const auto assetId = AZ::Data::AssetId(productData.first, 88);
        const auto* product = AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry::GetProductByAssetId(assetId);
        ASSERT_NE(product, nullptr);
        EXPECT_EQ(Drop({product}), Qt::CopyAction);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_EQ(fallback.m_unhandledDrops, 1);
        EXPECT_EQ(m_controller.GetMaterialAssetId(SlotId::CreateFromStableIdOnly(11)), assetId);
        EXPECT_TRUE(m_otherController.GetMaterialMap().empty());
    }

    TEST_F(MaterialSurfaceDropTests, LockedAndReadOnlySurfaceConsumeWithoutAssignment)
    {
        const auto* product = AddProduct(1);
        ASSERT_NE(product, nullptr);
        DefaultDropRoute fallback;
        {
            testing::NiceMock<EntityInfo> info(AZ::EntityId(101));
            EXPECT_CALL(info, IsLocked()).WillOnce(testing::Return(true));
            EXPECT_EQ(Drop({product}), Qt::IgnoreAction);
            EXPECT_TRUE(m_lastDropAccepted);
        }
        ASSERT_EQ(AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get(), nullptr);
        {
            ReadOnly readOnly;
            EXPECT_EQ(Drop({product}), Qt::IgnoreAction);
            EXPECT_TRUE(m_lastDropAccepted);
        }
        EXPECT_EQ(fallback.m_unhandledDrops, 0);
        ExpectUnchanged();
    }

    TEST_F(MaterialSurfaceDropTests, NonMaterialWrongContextAndPreviouslyAcceptedDropRemainUntouched)
    {
        const auto* material = AddProduct(1);
        const auto* other = AddProduct(2, false);
        ASSERT_NE(material, nullptr);
        ASSERT_NE(other, nullptr);
        DefaultDropRoute fallback;
        Drop({other});
        EXPECT_FALSE(m_lastDropAccepted);
        AzQtComponents::DragAndDropContextBase wrongContext;
        Drop({material}, false, &wrongContext);
        EXPECT_FALSE(m_lastDropAccepted);
        EXPECT_EQ(fallback.m_unhandledDrops, 2);
        EXPECT_EQ(Drop({material}, true), Qt::CopyAction);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_EQ(fallback.m_unhandledDrops, 2);
        ExpectUnchanged();
    }

    TEST_F(MaterialSurfaceDropTests, MissingMaterialComponentUsesCompositionThenAssignsTheHitEntity)
    {
        const auto* product = AddProduct(1);
        ASSERT_NE(product, nullptr);
        m_controller.Deactivate();
        m_controllerActive = false;
        testing::StrictMock<Composition> composition;
        EXPECT_CALL(composition, AddComponentsToEntities(testing::_, testing::_))
            .WillOnce([this](const AzToolsFramework::EntityIdList& entities, const AZ::ComponentTypeList& types)
            {
                EXPECT_EQ(entities, (AzToolsFramework::EntityIdList{AZ::EntityId(101)}));
                EXPECT_EQ(types, (AZ::ComponentTypeList{AZ::Uuid(JE::Render::EditorMaterialComponentTypeId)}));
                m_controller.Activate(AZ::EntityId(101));
                m_controllerActive = true;
                return AZ::Success(AzToolsFramework::EntityCompositionRequests::EntityToAddedComponentsMap{});
            });
        EXPECT_EQ(Drop({product}), Qt::CopyAction);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_EQ(m_controller.GetMaterialAssetId(SlotId::CreateFromStableIdOnly(11)), AZ::Data::AssetId(m_source, 1));
        EXPECT_TRUE(m_otherController.GetMaterialMap().empty());
    }

    TEST_F(MaterialSurfaceDropTests, FailedCompositionRejectsWithoutAssignment)
    {
        const auto* product = AddProduct(1);
        ASSERT_NE(product, nullptr);
        m_controller.Deactivate();
        m_controllerActive = false;
        testing::StrictMock<Composition> composition;
        EXPECT_CALL(composition, AddComponentsToEntities(testing::_, testing::_))
            .WillOnce(testing::Return(AZ::Failure(AZStd::string("Fixture composition failure"))));
        DefaultDropRoute fallback;
        CompositionWarnings warnings;
        EXPECT_EQ(Drop({product}), Qt::IgnoreAction);
        EXPECT_EQ(warnings.m_count, 1);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(AZ::EntityId(101)));
        EXPECT_EQ(fallback.m_unhandledDrops, 0);
        ExpectUnchanged();
    }

    TEST_F(MaterialSurfaceDropTests, PendingCompositionRemovesOnlyItsReturnedComponentAndRejects)
    {
        const auto* product = AddProduct(1);
        ASSERT_NE(product, nullptr);
        m_controller.Deactivate();
        m_controllerActive = false;
        PendingComponent pending;
        testing::StrictMock<Composition> composition;
        AzToolsFramework::EntityCompositionRequests::EntityToAddedComponentsMap added;
        auto& result = added[AZ::EntityId(101)];
        result.m_componentsAdded.push_back(&pending);
        result.m_addedPendingComponents.push_back(&pending);
        EXPECT_CALL(composition, AddComponentsToEntities(testing::_, testing::_))
            .WillOnce(testing::Return(AZ::Success(added)));
        EXPECT_CALL(composition, RemoveComponents(testing::_))
            .WillOnce([&pending](AZStd::span<AZ::Component* const> components)
            {
                EXPECT_EQ(components.size(), 1);
                if (components.size() == 1) { EXPECT_EQ(components.front(), &pending); }
                return AZ::Success(AzToolsFramework::EntityCompositionRequests::EntityToRemoveComponentsResultMap{});
            });
        DefaultDropRoute fallback;
        CompositionWarnings warnings;
        EXPECT_EQ(Drop({product}), Qt::IgnoreAction);
        EXPECT_EQ(warnings.m_count, 1);
        EXPECT_TRUE(m_lastDropAccepted);
        EXPECT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(AZ::EntityId(101)));
        EXPECT_EQ(fallback.m_unhandledDrops, 0);
        ExpectUnchanged();
    }

    TEST_F(MaterialSurfaceDropTests, PreexistingDisabledOrPendingMaterialComponentIsPreservedWithoutComposition)
    {
        const auto* product = AddProduct(1);
        ASSERT_NE(product, nullptr);
        m_controller.Deactivate();
        m_controllerActive = false;
        for (const bool pending : {false, true})
        {
            SCOPED_TRACE(pending ? "Pending material" : "Disabled material");
            AZ::Entity entity(AZ::EntityId(101));
            auto pendingOwner = AZStd::make_unique<AzToolsFramework::Components::EditorPendingCompositionComponent>();
            auto disabledOwner = AZStd::make_unique<AzToolsFramework::Components::EditorDisabledCompositionComponent>();
            // Supply aliases without requesting component names from reflection omitted by this fixture.
            pendingOwner->SetSerializedIdentifier("PendingStore");
            disabledOwner->SetSerializedIdentifier("DisabledStore");
            ASSERT_TRUE(entity.AddComponent(pendingOwner.get()));
            auto* pendingStore = pendingOwner.release();
            ASSERT_TRUE(entity.AddComponent(disabledOwner.get()));
            auto* disabledStore = disabledOwner.release();
            auto material = AZStd::make_unique<JE::Editor::EditorMaterialComponent>();
            material->SetSerializedIdentifier("ExistingMaterial");
            auto* existing = material.get();
            if (pending) { pendingStore->AddPendingComponent(material.release()); }
            else { disabledStore->AddDisabledComponent(material.release()); }
            // Native Init registers Entity101 and both storage buses without activating the material.
            entity.Init();
            ASSERT_EQ(m_application->FindEntity(AZ::EntityId(101)), &entity);
            ASSERT_EQ(existing->GetEntity(), &entity);
            ASSERT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(AZ::EntityId(101)));
            testing::StrictMock<Composition> composition; // Neither add nor remove is permitted for an existing component.
            DefaultDropRoute fallback;
            EXPECT_EQ(Drop({product}), Qt::IgnoreAction);
            EXPECT_TRUE(m_lastDropAccepted);
            EXPECT_EQ(fallback.m_unhandledDrops, 0);
            AZ::Entity::ComponentArrayType stored;
            pendingStore->GetPendingComponents(stored);
            disabledStore->GetDisabledComponents(stored);
            ASSERT_EQ(stored.size(), 1);
            EXPECT_EQ(stored.front(), existing);
            EXPECT_EQ(existing->GetSerializedIdentifier(), "ExistingMaterial");
            EXPECT_EQ(existing->GetEntity(), &entity);
            EXPECT_FALSE(JE::Render::MaterialComponentRequestBus::HasHandlers(AZ::EntityId(101)));
            ExpectUnchanged();
        }
    }

    class TextureContextMenuTests : public GraphDocumentCreationTests
    {
    protected:
        // Bind the real session-backed provider to the real bus without activating renderer services in this CPU fixture.
        class InlineInspectorProvider final : public AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler
        {
        public:
            explicit InlineInspectorProvider(JE::Editor::MaterialWorkspaceSession& session) : m_session(session) { BusConnect(); }
            ~InlineInspectorProvider() override { BusDisconnect(); }
            void GetAssetInspectorWidget(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry, QWidget* parent,
                QWidget*& widget, bool& accepted) override
            {
                JE::Editor::MaterialWorkspaceWidget::GetAssetInspectorWidget(m_session, entry, parent, widget, accepted);
            }
            JE::Editor::MaterialWorkspaceSession& m_session;
        };
        // Records the image-owner boundary only. Its default false models an already compatible inference;
        // the real owner/preset/cook behavior is verified separately, never simulated as RPI readiness here.
        class PresetBoundary final : public ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::Handler
        {
        public:
            PresetBoundary() { BusConnect(); }
            ~PresetBoundary() override { BusDisconnect(); }
            void OpenSourceTextureFile(const AZ::Uuid&) override { ADD_FAILURE() << "No texture dialog expected"; }
            AZ::Outcome<bool, AZStd::string> EnsureTexturePresetIfUnset(
                const AZStd::string& path, const AZStd::string& preset) override
            {
                calls.emplace_back(path, preset);
                if (callback) { return callback(path, preset); }
                return AZ::Success(false);
            }
            AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> calls;
            AZStd::function<AZ::Outcome<bool, AZStd::string>(const AZStd::string&, const AZStd::string&)> callback;
        };
        AZStd::unique_ptr<PresetBoundary> m_presets;

        struct ProjectRootOverride
        {
            ProjectRootOverride(AZ::SettingsRegistryInterface& registry, const char* path) : m_registry(registry)
            {
                m_hadValue = m_registry.Get(m_previous, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
                EXPECT_TRUE(m_registry.Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, path));
            }
            ~ProjectRootOverride()
            {
                if (m_hadValue)
                {
                    m_registry.Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, m_previous.c_str());
                }
                else
                {
                    m_registry.Remove(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
                }
            }
            AZ::SettingsRegistryInterface& m_registry;
            AZ::SettingsRegistryInterface::FixedValueString m_previous;
            bool m_hadValue = false;
        };
        AZStd::unique_ptr<ProjectRootOverride> m_projectRoot;

        struct EngineAlias
        {
            explicit EngineAlias(const char* path) : m_io(AZ::IO::FileIOBase::GetInstance())
            {
                const auto* previous = m_io->GetAlias("@engroot@");
                m_hadAlias = previous != nullptr;
                if (previous) { m_previous = previous; }
                m_io->SetAlias("@engroot@", path);
            }
            ~EngineAlias()
            {
                if (m_hadAlias) { m_io->SetAlias("@engroot@", m_previous.c_str()); }
                else { m_io->ClearAlias("@engroot@"); }
            }
            AZ::IO::FileIOBase* m_io;
            AZStd::string m_previous;
            bool m_hadAlias = false;
        };

        void SetUp() override
        {
            GraphDocumentCreationTests::SetUp();
            auto* manager = AZ::ReflectionEnvironment::GetReflectionManager();
            // Native document opening persists RecentFilePaths as this container in the settings registry.
            manager->Reflect(azrtti_typeid<AZStd::vector<AZStd::string>>(), [](AZ::ReflectContext* context)
            {
                if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serialize->RegisterGenericType<AZStd::vector<AZStd::string>>();
                }
            });
            manager->Reflect(azrtti_typeid<JE::Render::RPI::MaterialPropertyValue>(), &JE::Render::RPI::MaterialPropertyValue::Reflect);
            manager->Reflect(azrtti_typeid<JE::Render::RPI::JsonMaterialPropertyValueSerializer>(), [](AZ::ReflectContext* context)
            {
                if (auto* json = azrtti_cast<AZ::JsonRegistrationContext*>(context))
                {
                    json->Serializer<JE::Render::RPI::JsonMaterialPropertyValueSerializer>()->HandlesType<JE::Render::RPI::MaterialPropertyValue>();
                }
            });
            // The callback fixture needs only the source version/reference, not a shader/functor or RPI instance.
            const AZStd::string typePath = m_directory.filePath(
                "Engine/Render/Features/Assets/Materials/Types/StandardPBR.materialtype").toUtf8().constData();
            ASSERT_TRUE(AZ::Utils::WriteFile(R"({"version":6})", typePath).IsSuccess());
            m_engineAlias = AZStd::make_unique<EngineAlias>(m_directory.path().toUtf8().constData());
            auto* registry = AZ::Interface<AZ::SettingsRegistryInterface>::Get();
            ASSERT_NE(registry, nullptr);
            m_projectRoot = AZStd::make_unique<ProjectRootOverride>(*registry, m_directory.path().toUtf8().constData());
            m_presets = AZStd::make_unique<PresetBoundary>();
        }

        void TearDown() override
        {
            m_presets.reset();
            m_projectRoot.reset();
            m_engineAlias.reset();
            GraphDocumentCreationTests::TearDown();
        }
        AZStd::unique_ptr<EngineAlias> m_engineAlias;

        // This document reads the real material source and exercises native document dispatch/lifetime.
        // It does not create an RPI instance or claim that a cooked texture/material is ready.
        class SourceDocument final : public JE::Editor::Tools::AtomToolsDocument
        {
        public:
            AZ_CLASS_ALLOCATOR(SourceDocument, AZ::SystemAllocator);
            SourceDocument(const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& info, int& opens)
                : AtomToolsDocument(toolId, info), m_opens(opens) {}
            bool Open(const AZStd::string& path) override
            {
                ++m_opens;
                if (!AtomToolsDocument::Open(path)) { return false; }
                if (!JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(path).IsSuccess()) { return OpenFailed(); }
                return OpenSucceeded();
            }
            // This source-loading boundary exposes no editable fields; a no-op commit succeeds like MaterialDocument.
            bool EndEdit() override { return true; }
            int& m_opens;
        };

        JE::Editor::Tools::DocumentTypeInfo SourceType(int& opens)
        {
            auto type = EmptyDocumentType();
            type.m_supportedExtensionsToOpen.push_back({"Material source", "material"});
            type.m_documentFactoryCallback = [&opens](const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& info)
                -> JE::Editor::Tools::AtomToolsDocumentRequests*
            {
                return aznew SourceDocument(toolId, info, opens);
            };
            return type;
        }

        class CreatedNotifications final
            : public AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Handler
        {
        public:
            CreatedNotifications()
            {
                BusConnect(AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId);
            }
            ~CreatedNotifications() override { BusDisconnect(); }
            void HandleAssetCreatedInEditor(const AZStd::string_view path, const AZ::Crc32&, bool rename) override
            {
                EXPECT_FALSE(rename);
                m_path = path;
                ++m_created;
            }
            AZStd::string m_path;
            int m_created = 0;
        };

        // The Editor owns pane construction. Generation must complete without requesting that boundary.
        class UnopenedPaneRequests final : public AzToolsFramework::EditorRequests::Bus::Handler
        {
        public:
            UnopenedPaneRequests() { BusConnect(); }
            ~UnopenedPaneRequests() override { BusDisconnect(); }
            void OpenViewPane(const char*) override { ++m_showRequests; }
            QWidget* GetViewPaneWidget(const char*) override { ++m_widgetRequests; return nullptr; }
            void BrowseForAssets(AzToolsFramework::AssetBrowser::AssetSelectionModel&) override {}
            int m_showRequests = 0;
            int m_widgetRequests = 0;
        };

        class MaterialSelectionRequests final
            : public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        {
        public:
            MaterialSelectionRequests() { BusConnect(); }
            ~MaterialSelectionRequests() override { BusDisconnect(); }
            void SelectAsset(QWidget* caller, const AZStd::string& path) override
            {
                m_caller = caller;
                m_paths.push_back(path);
                AzToolsFramework::AssetBrowser::AssetBrowserModel* model = nullptr;
                AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::BroadcastResult(model,
                    &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::GetAssetBrowserModel);
                const auto index = model ? model->findIndex(QDir::fromNativeSeparators(QString::fromUtf8(path.c_str()))) : QModelIndex();
                const auto* entry = index.isValid()
                    ? index.data(AzToolsFramework::AssetBrowser::AssetBrowserModel::Roles::EntryRole)
                        .value<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>() : nullptr;
                const auto* source = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(entry);
                m_sourceIds.push_back(source ? source->GetSourceID() : -1);
                m_sourceGuids.push_back(source ? source->GetSourceUuid() : AZ::Uuid::CreateNull());
            }
            QWidget* m_caller = nullptr;
            AZStd::vector<AZStd::string> m_paths;
            AZStd::vector<AZ::s64> m_sourceIds;
            AZStd::vector<AZ::Uuid> m_sourceGuids;
        };

        // Same real model/root/cache population API used by AzToolsFramework AssetBrowserTests.
        class CachedMaterialModel final
            : public AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::Handler
        {
        public:
            explicit CachedMaterialModel(const AZStd::string& directory, bool publishSourceMetadata = true)
                : m_root(AZStd::make_shared<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry>())
                , m_model(AZStd::make_unique<AzToolsFramework::AssetBrowser::AssetBrowserModel>())
            {
                m_model->SetRootEntry(m_root);
                BusConnect();
                m_root->Update(directory.c_str());
                AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry folder;
                folder.m_scanFolderID = 1;
                folder.m_scanFolder = directory;
                folder.m_displayName = "Material creation fixture";
                folder.m_isRoot = true;
                m_root->AddScanFolder(folder);
                AzToolsFramework::AssetDatabase::FileDatabaseEntry file;
                file.m_scanFolderPK = 1;
                file.m_fileID = 77;
                file.m_fileName = "rock.material";
                file.m_isFolder = false;
                m_root->AddFile(file);
                if (publishSourceMetadata)
                {
                    m_sourceAdded = PublishSourceMetadata();
                }
            }
            ~CachedMaterialModel() override
            {
                BusDisconnect();
                m_model.reset();
                m_root.reset();
                AzToolsFramework::AssetBrowser::EntryCache::DestroyInstance();
                AzToolsFramework::AssetBrowser::AssetBrowserFavoritesManager::DestroyInstance();
            }
            AzToolsFramework::AssetBrowser::AssetBrowserModel* GetAssetBrowserModel() override { return m_model.get(); }
            bool AreEntriesReady() override { return true; }
            void PickAssets(AzToolsFramework::AssetBrowser::AssetSelectionModel&, QWidget*) override {}
            AzQtComponents::StyledBusyLabel* GetStyledBusyLabel() override { return nullptr; }
            bool PublishSourceMetadata()
            {
                AzToolsFramework::AssetBrowser::SourceWithFileID source;
                source.first = 77;
                source.second.m_scanFolderPK = 1;
                source.second.m_sourceName = "rock.material";
                source.second.m_sourceID = 77;
                source.second.m_sourceGuid = AZ::Uuid::CreateString("{6EBC8841-EB4A-43CC-941F-C2A8EC00CBED}");
                return m_root->AddSource(source);
            }
            bool m_sourceAdded = false;
        private:
            AZStd::shared_ptr<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry> m_root;
            AZStd::unique_ptr<AzToolsFramework::AssetBrowser::AssetBrowserModel> m_model;
        };

        AZStd::string WriteTexture(const char* name)
        {
            const QString path = m_directory.filePath(name);
            QImage image(2, 2, QImage::Format_RGBA8888);
            image.fill(Qt::white);
            EXPECT_TRUE(image.save(path, "PNG"));
            return path.toUtf8().constData();
        }

        void InvokeWithoutUnexpectedModal(const AZStd::function<void()>& callback)
        {
            int unexpectedModals = 0;
            QTimer rejectUnexpected;
            QObject::connect(&rejectUnexpected, &QTimer::timeout, &rejectUnexpected, [&unexpectedModals]()
            {
                if (auto* modal = QApplication::activeModalWidget())
                {
                    ++unexpectedModals;
                    QString details = QStringLiteral("Unexpected modal: class=%1; title=%2")
                        .arg(QString::fromLatin1(modal->metaObject()->className()))
                        .arg(modal->windowTitle());
                    if (auto* messageBox = qobject_cast<QMessageBox*>(modal))
                    {
                        details += QStringLiteral("\ntext: %1\ndetailedText: %2")
                            .arg(messageBox->text(), messageBox->detailedText());
                    }
                    ADD_FAILURE() << details.toUtf8().constData();
                    if (auto* dialog = qobject_cast<QDialog*>(modal))
                    {
                        dialog->reject();
                    }
                    else
                    {
                        modal->close();
                    }
                }
            });
            rejectUnexpected.start(1);
            callback();
            rejectUnexpected.stop();
            EXPECT_EQ(unexpectedModals, 0);
            EXPECT_EQ(QApplication::activeModalWidget(), nullptr);
        }

        void TriggerGenerate(const AZStd::string& path, QWidget* caller = nullptr)
        {
            JE::Editor::MaterialBrowserInteractions interactions;
            AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry source;
            source.SetFullPath(AZ::IO::Path(path));
            QMenu menu;
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Broadcast(
                &AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotifications::AddContextMenuActions,
                caller, &menu,
                AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>{&source});
            QAction* generate = nullptr;
            for (auto* action : menu.actions())
            {
                if (action->text() == QString::fromUtf8("Generar material"))
                {
                    EXPECT_EQ(generate, nullptr);
                    generate = action;
                }
            }
            ASSERT_NE(generate, nullptr);
            InvokeWithoutUnexpectedModal([generate]() { generate->trigger(); });
        }

        void CreateMaterialFromReparentedChooser(const AZStd::string& texture, const AZStd::string& target,
            QWidget* browserCaller, QWidget* decorationWrapper, QPointer<QDialog>& review)
        {
            ASSERT_FALSE(AZ::IO::SystemFile::Exists(target.c_str()));
            ASSERT_NO_FATAL_FAILURE(TriggerGenerate(texture, browserCaller));
            review = browserCaller->findChild<QDialog*>("MaterialTextureReview");
            ASSERT_NE(review.data(), nullptr);
            ASSERT_TRUE(review->isVisible());
            // Reproduce the parent change observed with the native window decoration, without another editor/window manager.
            review->setParent(decorationWrapper);
            ASSERT_EQ(review->parentWidget(), decorationWrapper);
            auto* choice = review->findChild<QComboBox*>("MaterialTextureChoice0");
            ASSERT_NE(choice, nullptr);
            int chosen = -1;
            for (int index = 1; index < choice->count(); ++index)
            {
                if (AZ::IO::PathView(choice->itemText(index).toUtf8().constData()).LexicallyNormal() ==
                    AZ::IO::PathView(texture).LexicallyNormal())
                {
                    chosen = index;
                }
            }
            ASSERT_GE(chosen, 1);
            choice->setCurrentIndex(chosen);
            auto* create = review->findChild<QPushButton*>("CreateMaterialFromTextures");
            ASSERT_NE(create, nullptr);
            InvokeWithoutUnexpectedModal([create]() { create->click(); });
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(target.c_str()));
        }
    };

    TEST_F(TextureContextMenuTests, ContextCreationDoesNotRequireOrOpenAMaterialWorkspace)
    {
        const auto texture = WriteTexture("rock_diff.png");
        const AZStd::string target = AZ::IO::Path(m_directory.filePath("rock.material").toUtf8().constData()).LexicallyNormal().String();
        QWidget browserCaller;
        UnopenedPaneRequests panes;
        CreatedNotifications created;

        TriggerGenerate(texture, &browserCaller);

        EXPECT_EQ(panes.m_showRequests, 0);
        EXPECT_EQ(panes.m_widgetRequests, 0);
        EXPECT_EQ(created.m_created, 1);
        EXPECT_EQ(created.m_path, target);
        const auto bytes = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(bytes.IsSuccess());
        auto source = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(target);
        ASSERT_TRUE(source.IsSuccess());
        EXPECT_EQ(source.GetValue().m_materialTypeVersion, 6);
        ASSERT_EQ(m_presets->calls.size(), 1);
        EXPECT_EQ(m_presets->calls.front().first, AZ::IO::Path(texture).LexicallyNormal().String());
        EXPECT_EQ(m_presets->calls.front().second, "Albedo");
    }

    TEST_F(TextureContextMenuTests, ContextCreationSelectsOnlyItsMaterialWhenTheSourceEntryArrives)
    {
        const auto texture = WriteTexture("rock_diff.png");
        const AZStd::string target = AZ::IO::Path(m_directory.filePath("rock.material").toUtf8().constData()).LexicallyNormal().String();
        QWidget browserCaller;
        UnopenedPaneRequests panes;
        MaterialSelectionRequests selection;

        TriggerGenerate(texture, &browserCaller);
        EXPECT_EQ(panes.m_showRequests, 0);
        ASSERT_TRUE(AZ::IO::SystemFile::Exists(target.c_str()));
        EXPECT_TRUE(selection.m_paths.empty());

        // AP/model discovery is the external boundary; generation and its real source file are not mocked.
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry other;
        other.SetFullPath(AZ::IO::Path(m_directory.filePath("other.material").toUtf8().constData()));
        AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications::EntryAdded, &other);
        QApplication::processEvents();
        EXPECT_TRUE(selection.m_paths.empty());

        ASSERT_EQ(AzToolsFramework::AssetBrowser::EntryCache::GetInstance(), nullptr);
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::HasHandlers());
        CachedMaterialModel cached(m_directory.path().toUtf8().constData());
        ASSERT_TRUE(cached.m_sourceAdded);
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry material;
#if defined(AZ_PLATFORM_WINDOWS)
        const auto discoveredPath = QDir::toNativeSeparators(QString::fromUtf8(target.c_str())).toUpper();
        material.SetFullPath(AZ::IO::Path(discoveredPath.toUtf8().constData()));
#else
        material.SetFullPath(AZ::IO::Path(target));
#endif
        AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications::EntryAdded, &material);
        QApplication::processEvents();
        ASSERT_EQ(selection.m_paths.size(), 1);
        EXPECT_EQ(selection.m_paths.front(), target);
        EXPECT_EQ(selection.m_caller, &browserCaller);
        AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications::EntryAdded, &material);
        QApplication::processEvents();
        EXPECT_EQ(selection.m_paths.size(), 1);
        EXPECT_EQ(panes.m_widgetRequests, 0);
    }

    TEST_F(TextureContextMenuTests, DestroyedBrowserCancelsPendingMaterialSelection)
    {
        const auto texture = WriteTexture("rock_diff.png");
        const AZStd::string target = AZ::IO::Path(m_directory.filePath("rock.material").toUtf8().constData()).LexicallyNormal().String();
        UnopenedPaneRequests panes;
        MaterialSelectionRequests selection;
        {
            QWidget browserCaller;
            TriggerGenerate(texture, &browserCaller);
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(target.c_str()));
            EXPECT_EQ(panes.m_showRequests, 0);
        }
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry material;
        material.SetFullPath(AZ::IO::Path(target));
        AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications::EntryAdded, &material);
        QApplication::processEvents();
        EXPECT_TRUE(selection.m_paths.empty());
        EXPECT_EQ(panes.m_widgetRequests, 0);

        // Also cancel a selection already queued by EntryAdded, before its QObject callback runs.
        {
            QWidget browserCaller;
            TriggerGenerate(texture, &browserCaller);
            AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Broadcast(
                &AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications::EntryAdded, &material);
        }
        QApplication::processEvents();
        EXPECT_TRUE(selection.m_paths.empty());
    }

    TEST_F(TextureContextMenuTests, ExistingCachedGeneratedSourceSelectsOnceWithoutWaitingForAnotherEntryAdded)
    {
        const auto texture = WriteTexture("rock_diff.png");
        const AZStd::string target = AZ::IO::Path(m_directory.filePath("rock.material").toUtf8().constData()).LexicallyNormal().String();
        UnopenedPaneRequests panes;
        CreatedNotifications created;
        {
            QWidget browserCaller;
            TriggerGenerate(texture, &browserCaller);
        }
        ASSERT_EQ(created.m_created, 1);
        auto before = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(before.IsSuccess());
        ASSERT_EQ(AzToolsFramework::AssetBrowser::EntryCache::GetInstance(), nullptr);
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserFavoriteRequestBus::HasHandlers());
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::HasHandlers());
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserModelRequestBus::HasHandlers());
        CachedMaterialModel cached(m_directory.path().toUtf8().constData());
        ASSERT_TRUE(cached.m_sourceAdded);
        const auto* entry = AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::GetSourceByUuid(
            AZ::Uuid::CreateString("{6EBC8841-EB4A-43CC-941F-C2A8EC00CBED}"));
        ASSERT_NE(entry, nullptr);
        ASSERT_EQ(entry->GetFileID(), 77);
        ASSERT_EQ(entry->GetSourceID(), 77);
        ASSERT_TRUE(AZ::IO::PathView(entry->GetFullPath()).LexicallyNormal() == AZ::IO::PathView(target).LexicallyNormal());
        // LocalFileIO resolves model paths to forward slashes; findIndex requires the actual stored representation.
        ASSERT_TRUE(cached.GetAssetBrowserModel()->findIndex(QString::fromUtf8(entry->GetFullPath().c_str())).isValid());
        const auto key = AZ::IO::PathView(target).LexicallyNormal().String();
        auto* cache = AzToolsFramework::AssetBrowser::EntryCache::GetInstance();
        ASSERT_NE(cache, nullptr);
        ASSERT_NE(cache->m_absolutePathToFileId.find(key), cache->m_absolutePathToFileId.end());
        EXPECT_EQ(cache->m_absolutePathToFileId.at(key), 77);
        MaterialSelectionRequests selection;
        QWidget browserCaller;
        TriggerGenerate(texture, &browserCaller);
        QApplication::processEvents();
        ASSERT_EQ(selection.m_paths.size(), 1);
        EXPECT_EQ(selection.m_paths.front(), target);
        EXPECT_EQ(selection.m_caller, &browserCaller);
        AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserModelNotifications::EntryAdded, entry);
        QApplication::processEvents();
        EXPECT_EQ(selection.m_paths.size(), 1);
        EXPECT_EQ(created.m_created, 1);
        EXPECT_EQ(m_presets->calls.size(), 1);
        EXPECT_EQ(panes.m_showRequests, 0);
        EXPECT_EQ(panes.m_widgetRequests, 0);
        auto after = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), before.GetValue());
    }

    TEST_F(TextureContextMenuTests, AmbiguousCreationWaitsForSourceMetadataAfterItsDialogIsDestroyed)
    {
        const auto texture = WriteTexture("rock_diff.png");
        WriteTexture("rock_albedo.png");
        const AZStd::string target = AZ::IO::Path(m_directory.filePath("rock.material").toUtf8().constData()).LexicallyNormal().String();
        const auto expectedGuid = AZ::Uuid::CreateString("{6EBC8841-EB4A-43CC-941F-C2A8EC00CBED}");
        QWidget browserCaller;
        UnopenedPaneRequests panes;
        MaterialSelectionRequests selection;
        TriggerGenerate(texture, &browserCaller);
        QPointer<QDialog> review = browserCaller.findChild<QDialog*>("MaterialTextureReview");
        ASSERT_NE(review.data(), nullptr);
        auto* choice = review->findChild<QComboBox*>("MaterialTextureChoice0");
        ASSERT_NE(choice, nullptr);
        int chosen = -1;
        for (int index = 1; index < choice->count(); ++index)
        {
            if (AZ::IO::PathView(choice->itemText(index).toUtf8().constData()).LexicallyNormal() ==
                AZ::IO::PathView(texture).LexicallyNormal())
            {
                chosen = index;
            }
        }
        ASSERT_GE(chosen, 1);
        choice->setCurrentIndex(chosen);
        auto* create = review->findChild<QPushButton*>("CreateMaterialFromTextures");
        ASSERT_NE(create, nullptr);
        InvokeWithoutUnexpectedModal([create]() { create->click(); });
        ASSERT_TRUE(AZ::IO::SystemFile::Exists(target.c_str()));
        ASSERT_TRUE(selection.m_paths.empty());
        ASSERT_EQ(AzToolsFramework::AssetBrowser::EntryCache::GetInstance(), nullptr);
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserFavoriteRequestBus::HasHandlers());
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::HasHandlers());
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserModelRequestBus::HasHandlers());

        // This is the real AddFile stage: a row/cache entry exists, but AP has not assigned source metadata yet.
        CachedMaterialModel cached(m_directory.path().toUtf8().constData(), false);
        auto* model = cached.GetAssetBrowserModel();
        auto* cache = AzToolsFramework::AssetBrowser::EntryCache::GetInstance();
        ASSERT_NE(cache, nullptr);
        ASSERT_NE(cache->m_fileIdMap.find(77), cache->m_fileIdMap.end());
        const auto* entry = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(cache->m_fileIdMap.at(77));
        ASSERT_NE(entry, nullptr);
        ASSERT_TRUE(model->findIndex(QString::fromUtf8(entry->GetFullPath().c_str())).isValid());
        EXPECT_EQ(entry->GetFileID(), 77);
        EXPECT_EQ(entry->GetSourceID(), -1);
        EXPECT_TRUE(entry->GetSourceUuid().IsNull());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_TRUE(review.isNull());
        QApplication::processEvents();
        EXPECT_TRUE(selection.m_paths.empty());
        EXPECT_NE(browserCaller.findChild<QObject*>("PendingGeneratedMaterialSelection", Qt::FindDirectChildrenOnly), nullptr);

        int sourceDataSignals = 0;
        bool signalHasCompleteMetadata = true;
        bool signalCoversTheSourceRow = true;
        QObject::connect(model, &QAbstractItemModel::dataChanged, &browserCaller,
            [&](const QModelIndex& first, const QModelIndex& last)
            {
                const auto* changed = first.data(AzToolsFramework::AssetBrowser::AssetBrowserModel::Roles::EntryRole)
                    .value<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>();
                if (changed != entry)
                {
                    return;
                }
                ++sourceDataSignals;
                // The model canonicalizes entry indices; compare to its real source index, not hypothetical column endpoints.
                const auto canonical = model->findIndex(QString::fromUtf8(entry->GetFullPath().c_str()));
                const auto* lastEntry = last.data(AzToolsFramework::AssetBrowser::AssetBrowserModel::Roles::EntryRole)
                    .value<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>();
                signalCoversTheSourceRow &= canonical.isValid() && first.isValid() && last.isValid()
                    && canonical.model() == model && first.model() == model && last.model() == model
                    && first == canonical && last == canonical
                    && first.parent() == canonical.parent() && last.parent() == canonical.parent()
                    && first.row() == canonical.row() && last.row() == canonical.row()
                    && changed == entry && lastEntry == entry;
                signalHasCompleteMetadata &= entry->GetSourceID() == 77 && entry->GetSourceUuid() == expectedGuid &&
                    cache->m_sourceIdMap.contains(77) && cache->m_sourceIdMap.at(77) == entry &&
                    cache->m_sourceUuidMap.contains(expectedGuid) && cache->m_sourceUuidMap.at(expectedGuid) == entry;
            });
        // No synthetic signal, source replacement, timer delay or cook: the real AddSource must publish completion.
        ASSERT_TRUE(cached.PublishSourceMetadata());
        EXPECT_EQ(sourceDataSignals, 1);
        EXPECT_TRUE(signalHasCompleteMetadata);
        EXPECT_TRUE(signalCoversTheSourceRow);
        EXPECT_EQ(entry->GetSourceID(), 77);
        EXPECT_EQ(entry->GetSourceUuid(), expectedGuid);
        QApplication::processEvents();
        ASSERT_EQ(selection.m_paths.size(), 1);
        EXPECT_EQ(selection.m_paths.front(), target);
        EXPECT_EQ(selection.m_caller, &browserCaller);
        ASSERT_EQ(selection.m_sourceIds.size(), 1);
        EXPECT_EQ(selection.m_sourceIds.front(), 77);
        ASSERT_EQ(selection.m_sourceGuids.size(), 1);
        EXPECT_EQ(selection.m_sourceGuids.front(), expectedGuid);
        ASSERT_TRUE(cached.PublishSourceMetadata());
        QApplication::processEvents();
        EXPECT_EQ(sourceDataSignals, 2);
        EXPECT_EQ(selection.m_paths.size(), 1);
        EXPECT_EQ(panes.m_showRequests, 0);
        EXPECT_EQ(panes.m_widgetRequests, 0);
    }

    TEST_F(TextureContextMenuTests, ReparentedChooserKeepsSelectionOwnedByTheOriginalBrowser)
    {
        const auto texture = WriteTexture("rock_diff.png");
        WriteTexture("rock_albedo.png");
        const AZStd::string target = AZ::IO::Path(m_directory.filePath("rock.material").toUtf8().constData()).LexicallyNormal().String();
        const auto expectedGuid = AZ::Uuid::CreateString("{6EBC8841-EB4A-43CC-941F-C2A8EC00CBED}");
        QWidget browserCaller;
        auto decorationWrapper = AZStd::make_unique<QWidget>();
        UnopenedPaneRequests panes;
        CreatedNotifications created;
        MaterialSelectionRequests selection;
        QPointer<QDialog> review;
        ASSERT_NO_FATAL_FAILURE(CreateMaterialFromReparentedChooser(texture, target, &browserCaller, decorationWrapper.get(), review));
        ASSERT_EQ(created.m_created, 1);
        ASSERT_TRUE(selection.m_paths.empty());

        QPointer<QObject> pending = browserCaller.findChild<QObject*>("PendingGeneratedMaterialSelection", Qt::FindDirectChildrenOnly);
        EXPECT_NE(pending.data(), nullptr);
        // Observe the erroneous owner's destruction on RED too; the original-browser ownership assertion remains mandatory.
        if (!pending)
        {
            pending = decorationWrapper->findChild<QObject*>("PendingGeneratedMaterialSelection", Qt::FindDirectChildrenOnly);
        }
        ASSERT_NE(pending.data(), nullptr);
        EXPECT_EQ(pending->parent(), &browserCaller);
        EXPECT_EQ(decorationWrapper->findChild<QObject*>("PendingGeneratedMaterialSelection", Qt::FindDirectChildrenOnly), nullptr);

        ASSERT_EQ(AzToolsFramework::AssetBrowser::EntryCache::GetInstance(), nullptr);
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::HasHandlers());
        CachedMaterialModel cached(m_directory.path().toUtf8().constData(), false);
        auto* cache = AzToolsFramework::AssetBrowser::EntryCache::GetInstance();
        ASSERT_NE(cache, nullptr);
        ASSERT_NE(cache->m_fileIdMap.find(77), cache->m_fileIdMap.end());
        const auto* entry = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(cache->m_fileIdMap.at(77));
        ASSERT_NE(entry, nullptr);
        EXPECT_EQ(entry->GetSourceID(), -1);
        EXPECT_TRUE(entry->GetSourceUuid().IsNull());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_TRUE(review.isNull());
        decorationWrapper.reset();
        QApplication::processEvents();
        EXPECT_FALSE(pending.isNull());
        EXPECT_TRUE(selection.m_paths.empty());

        ASSERT_TRUE(cached.PublishSourceMetadata());
        QApplication::processEvents();
        EXPECT_EQ(selection.m_paths.size(), 1);
        if (selection.m_paths.size() == 1)
        {
            EXPECT_EQ(selection.m_paths.front(), target);
            EXPECT_EQ(selection.m_caller, &browserCaller);
            ASSERT_EQ(selection.m_sourceIds.size(), 1);
            EXPECT_EQ(selection.m_sourceIds.front(), 77);
            ASSERT_EQ(selection.m_sourceGuids.size(), 1);
            EXPECT_EQ(selection.m_sourceGuids.front(), expectedGuid);
        }
        ASSERT_TRUE(cached.PublishSourceMetadata());
        QApplication::processEvents();
        EXPECT_EQ(selection.m_paths.size(), 1);

        // The existing-file route still selects explicitly, without rewriting the material or preparing presets again.
        auto saved = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(saved.IsSuccess());
        const auto presetCalls = m_presets->calls.size();
        TriggerGenerate(texture, &browserCaller);
        QApplication::processEvents();
        EXPECT_EQ(selection.m_paths.size(), 2);
        EXPECT_EQ(selection.m_caller, &browserCaller);
        EXPECT_EQ(m_presets->calls.size(), presetCalls);
        EXPECT_EQ(created.m_created, 1);
        auto retained = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(retained.IsSuccess());
        EXPECT_EQ(retained.GetValue(), saved.GetValue());
        EXPECT_EQ(panes.m_showRequests, 0);
        EXPECT_EQ(panes.m_widgetRequests, 0);
    }

    TEST_F(TextureContextMenuTests, ReparentedChooserPendingSelectionIsCancelledWithTheOriginalBrowser)
    {
        const auto texture = WriteTexture("rock_diff.png");
        WriteTexture("rock_albedo.png");
        const AZStd::string target = AZ::IO::Path(m_directory.filePath("rock.material").toUtf8().constData()).LexicallyNormal().String();
        auto browserCaller = AZStd::make_unique<QWidget>();
        auto decorationWrapper = AZStd::make_unique<QWidget>();
        UnopenedPaneRequests panes;
        MaterialSelectionRequests selection;
        QPointer<QDialog> review;
        ASSERT_NO_FATAL_FAILURE(CreateMaterialFromReparentedChooser(texture, target, browserCaller.get(), decorationWrapper.get(), review));
        QPointer<QObject> pending = browserCaller->findChild<QObject*>("PendingGeneratedMaterialSelection", Qt::FindDirectChildrenOnly);
        EXPECT_NE(pending.data(), nullptr);
        if (!pending)
        {
            pending = decorationWrapper->findChild<QObject*>("PendingGeneratedMaterialSelection", Qt::FindDirectChildrenOnly);
        }
        ASSERT_NE(pending.data(), nullptr);
        EXPECT_EQ(pending->parent(), browserCaller.get());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_TRUE(review.isNull());
        QPointer<QWidget> originalBrowser = browserCaller.get();
        browserCaller.reset();
        EXPECT_TRUE(originalBrowser.isNull());
        EXPECT_TRUE(pending.isNull());
        // Keep the wrapper alive: cancellation must come from the original caller's real QObject lifetime.
        ASSERT_NE(decorationWrapper.get(), nullptr);
        ASSERT_EQ(AzToolsFramework::AssetBrowser::EntryCache::GetInstance(), nullptr);
        ASSERT_FALSE(AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::HasHandlers());
        CachedMaterialModel cached(m_directory.path().toUtf8().constData(), false);
        ASSERT_TRUE(cached.PublishSourceMetadata());
        QApplication::processEvents();
        EXPECT_TRUE(selection.m_paths.empty());
        ASSERT_TRUE(cached.PublishSourceMetadata());
        QApplication::processEvents();
        EXPECT_TRUE(selection.m_paths.empty());
        EXPECT_EQ(panes.m_showRequests, 0);
        EXPECT_EQ(panes.m_widgetRequests, 0);
    }

    TEST_F(TextureContextMenuTests, CancelledAmbiguityDoesNotCreateMaterialOrPreparePresets)
    {
        const auto texture = WriteTexture("rock_diff.png");
        WriteTexture("rock_albedo.png");
        const AZStd::string target = AZ::IO::Path(m_directory.filePath("rock.material").toUtf8().constData()).LexicallyNormal().String();
        QWidget browserCaller;
        UnopenedPaneRequests panes;
        CreatedNotifications created;
        TriggerGenerate(texture, &browserCaller);
        auto* review = browserCaller.findChild<QDialog*>("MaterialTextureReview");
        ASSERT_NE(review, nullptr);
        EXPECT_TRUE(review->isVisible());
        review->reject();
        QApplication::processEvents();
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(target.c_str()));
        EXPECT_TRUE(m_presets->calls.empty());
        EXPECT_EQ(created.m_created, 0);
        EXPECT_EQ(panes.m_showRequests, 0);
        EXPECT_EQ(panes.m_widgetRequests, 0);
    }

    TEST_F(TextureContextMenuTests, WorkspaceConstructedAfterExplicitOpenAlreadySelectsItsDocument)
    {
        const AZStd::string target = m_directory.filePath("explicit.material").toUtf8().constData();
        const AZStd::string type = m_directory.filePath(
            "Engine/Render/Features/Assets/Materials/Types/StandardPBR.materialtype").toUtf8().constData();
        ASSERT_TRUE(JE::Editor::CreateMaterialSourceFile(type, target).IsSuccess());
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        const auto id = session.OpenDocument(target);
        ASSERT_FALSE(id.IsNull());
        ASSERT_TRUE(session.SelectDocument(id));
        EXPECT_EQ(opens, 1);
        JE::Editor::MaterialWorkspaceWidget pane(session);
        const auto* documents = pane.findChild<QComboBox*>();
        ASSERT_NE(documents, nullptr);
        ASSERT_EQ(documents->count(), 1);
        EXPECT_EQ(AZ::Uuid::CreateString(documents->currentData().toString().toUtf8().constData()), id);
        EXPECT_EQ(session.GetActiveDocumentId(), id);
    }

    TEST_F(TextureContextMenuTests, ContextCallbackPreservesAnOccupiedManualMaterialAndReportsWithoutWorkspace)
    {
        const auto texture = WriteTexture("rock_diff.png");
        auto set = JE::Editor::AnalyzeMaterialTextureSet(texture);
        ASSERT_TRUE(set.IsSuccess());
        const auto target = set.GetValue().suggestedMaterialPath;
        ASSERT_TRUE(AZ::Utils::WriteFile("manual material sentinel", target).IsSuccess());
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        EXPECT_EQ(requests.m_showRequests, 0);
        EXPECT_EQ(requests.m_widgetRequests, 0);
        EXPECT_TRUE(m_presets->calls.empty());
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 0);
        EXPECT_TRUE(session.GetDocumentIds().empty());
        auto existing = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(existing.IsSuccess());
        EXPECT_EQ(existing.GetValue(), "manual material sentinel");
        const auto* status = pane.findChild<QLabel*>("MaterialTextureStatus");
        ASSERT_NE(status, nullptr);
        EXPECT_TRUE(status->text().contains("ocupado"));
    }

    TEST_F(TextureContextMenuTests, ContextCallbackCreatesThenSelectsTheSameGeneratedMaterialWithoutWriting)
    {
        const auto texture = WriteTexture("rock_diff.png");
        auto set = JE::Editor::AnalyzeMaterialTextureSet(texture);
        ASSERT_TRUE(set.IsSuccess());
        AZStd::string target = set.GetValue().suggestedMaterialPath;
        ASSERT_TRUE(JE::Editor::Tools::ValidateDocumentPath(target));
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        EXPECT_EQ(requests.m_showRequests, 0);
        EXPECT_EQ(requests.m_widgetRequests, 0);
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 1);
        EXPECT_EQ(notifications.m_path, target);
        EXPECT_TRUE(session.GetDocumentIds().empty());
        ASSERT_TRUE(JE::Editor::IsMaterialForTextureSet(set.GetValue(), target));
        auto before = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(before.IsSuccess());
        auto modified = AZ::JsonSerializationUtils::ReadJsonFile(target);
        ASSERT_TRUE(modified.IsSuccess());
        auto& json = modified.GetValue();
        ASSERT_TRUE(json.HasMember("description"));
        ASSERT_TRUE(json["description"].IsString());
        AZStd::string description = json["description"].GetString();
        description += "\nManual description after generation";
        json["description"].SetString(description.c_str(), static_cast<rapidjson::SizeType>(description.size()), json.GetAllocator());
        AZStd::string edited;
        ASSERT_TRUE(AZ::JsonSerializationUtils::WriteJsonString(json, edited).IsSuccess());
        ASSERT_TRUE(AZ::Utils::WriteFile(edited, target).IsSuccess());
        TriggerGenerate(texture, &pane);
        EXPECT_EQ(requests.m_showRequests, 0);
        EXPECT_EQ(requests.m_widgetRequests, 0);
        EXPECT_EQ(m_presets->calls.size(), 1);
        // Generation selects the source; only explicit editing may load a document.
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 1);
        EXPECT_TRUE(session.GetDocumentIds().empty());
        auto after = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), edited);
    }

    TEST_F(TextureContextMenuTests, AmbiguousCandidatesWaitForAnExplicitCompactChoiceWithoutWorkspace)
    {
        const auto texture = WriteTexture("rock_diff.png");
        WriteTexture("rock_albedo.png");
        auto set = JE::Editor::AnalyzeMaterialTextureSet(texture);
        ASSERT_TRUE(set.IsSuccess());
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 0);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(set.GetValue().suggestedMaterialPath.c_str()));
        auto* choice = pane.findChild<QComboBox*>("MaterialTextureChoice0");
        ASSERT_NE(choice, nullptr);
        ASSERT_EQ(choice->count(), 3);
        EXPECT_TRUE(m_presets->calls.empty());
        EXPECT_EQ(choice->currentData().toInt(), -1);
        auto* create = pane.findChild<QPushButton*>("CreateMaterialFromTextures");
        ASSERT_NE(create, nullptr);
        // Keeping Sin asignar is an explicit partial-source decision, not a first-file heuristic.
        InvokeWithoutUnexpectedModal([create]() { create->click(); });
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 1);
        EXPECT_FALSE(create->isEnabled());
        auto source = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(set.GetValue().suggestedMaterialPath);
        ASSERT_TRUE(source.IsSuccess());
        EXPECT_FALSE(source.GetValue().HasPropertyValue(AZ::Name("baseColor.textureMap")));
        EXPECT_NE(source.GetValue().m_description.find("rock_diff.png"), AZStd::string::npos);
        EXPECT_NE(source.GetValue().m_description.find("rock_albedo.png"), AZStd::string::npos);
        auto before = AZ::Utils::ReadFile<AZStd::string>(set.GetValue().suggestedMaterialPath);
        ASSERT_TRUE(before.IsSuccess());
        TriggerGenerate(texture, &pane);
        const auto* repeatedReview = pane.findChild<QDialog*>("MaterialTextureReview");
        ASSERT_NE(repeatedReview, nullptr);
        EXPECT_FALSE(repeatedReview->isVisible());
        EXPECT_EQ(repeatedReview->findChild<QComboBox*>("MaterialTextureChoice0"), nullptr);
        EXPECT_EQ(notifications.m_created, 1);
        EXPECT_TRUE(m_presets->calls.empty());
        auto after = AZ::Utils::ReadFile<AZStd::string>(set.GetValue().suggestedMaterialPath);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), before.GetValue());
    }

    TEST_F(TextureContextMenuTests, ExplicitCandidateSelectionCreatesThatSourceRatherThanTheFirstFile)
    {
        const auto texture = WriteTexture("rock_diff.png");
        const auto selectedPath = WriteTexture("rock_albedo.png");
        auto set = JE::Editor::AnalyzeMaterialTextureSet(texture);
        ASSERT_TRUE(set.IsSuccess());
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        auto* choice = pane.findChild<QComboBox*>("MaterialTextureChoice0");
        ASSERT_NE(choice, nullptr);
        int selectedIndex = -1;
        for (int index = 1; index < choice->count(); ++index)
        {
            if (QDir::fromNativeSeparators(choice->itemText(index)) ==
                QDir::fromNativeSeparators(QString::fromUtf8(selectedPath.c_str())))
            {
                selectedIndex = index;
            }
        }
        ASSERT_GE(selectedIndex, 1);
        choice->setCurrentIndex(selectedIndex);
        auto* create = pane.findChild<QPushButton*>("CreateMaterialFromTextures");
        ASSERT_NE(create, nullptr);
        InvokeWithoutUnexpectedModal([create]() { create->click(); });
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 1);
        auto source = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(set.GetValue().suggestedMaterialPath);
        ASSERT_TRUE(source.IsSuccess());
        const auto& value = source.GetValue().GetPropertyValue(AZ::Name("baseColor.textureMap"));
        ASSERT_TRUE(value.Is<AZStd::string>());
        EXPECT_EQ(value.GetValue<AZStd::string>(), "rock_albedo.png");
        EXPECT_NE(source.GetValue().m_description.find("rock_diff.png"), AZStd::string::npos);
        EXPECT_NE(source.GetValue().m_description.find("rock_albedo.png"), AZStd::string::npos);
    }

    TEST_F(TextureContextMenuTests, AllSelectedTextureRolesReachTheirNativePresetOwnerBeforeCreation)
    {
        const auto texture = WriteTexture("rock_diff.png");
        WriteTexture("rock_normal.png");
        WriteTexture("rock_rough.png");
        WriteTexture("rock_metal.png");
        WriteTexture("rock_ao.png");
        WriteTexture("rock_emissive.png");
        WriteTexture("rock_disp.png");
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 1);
        ASSERT_EQ(m_presets->calls.size(), 7);
        const AZStd::pair<const char*, const char*> expected[] = {
            {"rock_diff.png", "Albedo"}, {"rock_normal.png", "Normals"},
            {"rock_rough.png", "Reflectance"}, {"rock_metal.png", "Reflectance"},
            {"rock_ao.png", "AmbientOcclusion"}, {"rock_emissive.png", "Emissive"},
            {"rock_disp.png", "Displacement"}};
        for (const auto& entry : expected)
        {
            int count = 0;
            for (const auto& call : m_presets->calls)
            {
                if (QFileInfo(QString::fromUtf8(call.first.c_str())).fileName() == entry.first)
                {
                    EXPECT_EQ(call.second, entry.second);
                    ++count;
                }
            }
            EXPECT_EQ(count, 1);
        }
    }

    TEST_F(TextureContextMenuTests, PackedChannelsShareOneLinearRgbPresetRequest)
    {
        const auto texture = WriteTexture("rock_arm.png");
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 1);
        ASSERT_EQ(m_presets->calls.size(), 1);
        EXPECT_EQ(m_presets->calls.front().second, "LayerMask");
    }

    TEST_F(TextureContextMenuTests, PresetReviewFailurePreservesManualSettingsAndDoesNotPublishAMaterial)
    {
        const auto texture = WriteTexture("rock_diff.png");
        const auto sidecar = texture + ".assetinfo";
        ASSERT_TRUE(AZ::Utils::WriteFile("manual settings sentinel", sidecar).IsSuccess());
        m_presets->callback = [](const AZStd::string&, const AZStd::string&) -> AZ::Outcome<bool, AZStd::string>
        {
            return AZ::Failure(AZStd::string("Existing settings require explicit review."));
        };
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 0);
        auto data = AZ::Utils::ReadFile<AZStd::string>(sidecar);
        ASSERT_TRUE(data.IsSuccess());
        EXPECT_EQ(data.GetValue(), "manual settings sentinel");
        auto set = JE::Editor::AnalyzeMaterialTextureSet(texture);
        ASSERT_TRUE(set.IsSuccess());
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(set.GetValue().suggestedMaterialPath.c_str()));
        const auto* status = pane.findChild<QLabel*>("MaterialTextureStatus");
        ASSERT_NE(status, nullptr);
        EXPECT_TRUE(status->text().contains("review"));
    }

    TEST_F(TextureContextMenuTests, MaterialCreateRaceRollsBackOnlyTheSidecarJustCreatedByTheOperation)
    {
        const auto texture = WriteTexture("rock_diff.png");
        auto set = JE::Editor::AnalyzeMaterialTextureSet(texture);
        ASSERT_TRUE(set.IsSuccess());
        const auto target = set.GetValue().suggestedMaterialPath;
        const auto sidecar = texture + ".assetinfo";
        m_presets->callback = [&target](const AZStd::string& source, const AZStd::string&) -> AZ::Outcome<bool, AZStd::string>
        {
            // Opaque boundary fixture, not a fabricated XML preset or a claim about the actual image owner.
            if (!AZ::Utils::WriteFile("owned settings fixture", source + ".assetinfo").IsSuccess() ||
                !AZ::Utils::WriteFile("concurrent material sentinel", target).IsSuccess())
            {
                return AZ::Failure(AZStd::string("fixture write failed"));
            }
            return AZ::Success(true);
        };
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 0);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(sidecar.c_str()));
        auto material = AZ::Utils::ReadFile<AZStd::string>(target);
        ASSERT_TRUE(material.IsSuccess());
        EXPECT_EQ(material.GetValue(), "concurrent material sentinel");
    }

    TEST_F(TextureContextMenuTests, ChangedSidecarIsPreservedWhenLaterPresetPreparationFails)
    {
        const auto texture = WriteTexture("rock_diff.png");
        WriteTexture("rock_rough.png");
        AZStd::string firstSidecar;
        m_presets->callback = [&firstSidecar](const AZStd::string& source, const AZStd::string&)
            -> AZ::Outcome<bool, AZStd::string>
        {
            if (firstSidecar.empty())
            {
                firstSidecar = source + ".assetinfo";
                if (!AZ::Utils::WriteFile("owned settings fixture", firstSidecar).IsSuccess())
                {
                    return AZ::Failure(AZStd::string("fixture write failed"));
                }
                return AZ::Success(true);
            }
            if (!AZ::Utils::WriteFile("manual replacement settings", firstSidecar).IsSuccess())
            {
                return AZ::Failure(AZStd::string("fixture replacement failed"));
            }
            return AZ::Failure(AZStd::string("Second texture requires review."));
        };
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(texture, &pane);
        ASSERT_EQ(m_presets->calls.size(), 2);
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 0);
        auto retained = AZ::Utils::ReadFile<AZStd::string>(firstSidecar);
        ASSERT_TRUE(retained.IsSuccess());
        EXPECT_EQ(retained.GetValue(), "manual replacement settings");
        const auto* status = pane.findChild<QLabel*>("MaterialTextureStatus");
        ASSERT_NE(status, nullptr);
        EXPECT_TRUE(status->text().contains("Se conservaron"));
    }

    TEST_F(TextureContextMenuTests, WritableForeignSourceWithoutAssignedMapsCannotPublishOutsideTheCurrentProject)
    {
        QTemporaryDir foreign;
        ASSERT_TRUE(foreign.isValid());
        const QString imagePath = foreign.filePath("foreign_unrecognized.png");
        QImage image(2, 2, QImage::Format_RGBA8888);
        image.fill(Qt::white);
        ASSERT_TRUE(image.save(imagePath, "PNG"));
        auto set = JE::Editor::AnalyzeMaterialTextureSet(imagePath.toUtf8().constData());
        ASSERT_TRUE(set.IsSuccess());
        ASSERT_TRUE(set.GetValue().selected.empty());
        ON_CALL(m_assetSystem, GetAssetSafeFolders(testing::_)).WillByDefault(
            [this, &foreign](AZStd::vector<AZStd::string>& folders)
            {
                folders.push_back(m_directory.path().toUtf8().constData());
                folders.push_back(foreign.path().toUtf8().constData());
                return true;
            });
        AZStd::string target = set.GetValue().suggestedMaterialPath;
        // The generic source-folder validator permits this location; the authoritative project guard must still reject it.
        ASSERT_TRUE(JE::Editor::Tools::ValidateDocumentPath(target));
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        TriggerGenerate(imagePath.toUtf8().constData(), &pane);
        EXPECT_TRUE(m_presets->calls.empty());
        EXPECT_EQ(opens, 0);
        EXPECT_EQ(notifications.m_created, 0);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(target.c_str()));
        const auto* status = pane.findChild<QLabel*>("MaterialTextureStatus");
        ASSERT_NE(status, nullptr);
        EXPECT_TRUE(status->text().contains("proyecto actual"));
    }

    TEST_F(TextureContextMenuTests, UnassignedMapsCannotCreateMaterialInProtectedProjectDirectories)
    {
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        QWidget pane;
        UnopenedPaneRequests requests;
        CreatedNotifications notifications;
        for (const char* folder : {"Cache", "user", "build", "Gems", "3rdParty", "External"})
        {
            const QString directory = m_directory.filePath(folder);
            ASSERT_TRUE(QDir().mkpath(directory));
            const QString imagePath = QDir(directory).filePath("unclassified_texture.png");
            QImage image(2, 2, QImage::Format_RGBA8888);
            image.fill(Qt::white);
            ASSERT_TRUE(image.save(imagePath, "PNG"));
            auto set = JE::Editor::AnalyzeMaterialTextureSet(imagePath.toUtf8().constData());
            ASSERT_TRUE(set.IsSuccess());
            ASSERT_TRUE(set.GetValue().selected.empty());
            AZStd::string target = set.GetValue().suggestedMaterialPath;
            ASSERT_TRUE(JE::Editor::Tools::ValidateDocumentPath(target));
            TriggerGenerate(imagePath.toUtf8().constData(), &pane);
            EXPECT_FALSE(AZ::IO::SystemFile::Exists(target.c_str())) << folder;
            EXPECT_TRUE(m_presets->calls.empty());
            EXPECT_EQ(opens, 0);
            EXPECT_EQ(notifications.m_created, 0);
            const auto* status = pane.findChild<QLabel*>("MaterialTextureStatus");
            ASSERT_NE(status, nullptr);
            EXPECT_TRUE(status->text().contains("Cache/user/build/Gems/3rdParty/External"));
        }
    }

    TEST_F(TextureContextMenuTests, MaterialSourceAndProductShareInlineInspectorWithoutOpeningAnotherPane)
    {
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        const AZStd::string sourcePath = m_directory.filePath("rock.material").toUtf8().constData();
        const AZStd::string cachePath = m_directory.filePath("inline.azmaterial").toUtf8().constData();
        ASSERT_TRUE(AZ::Utils::WriteFile(
            R"({"materialType":"@engroot@/Engine/Render/Features/Assets/Materials/Types/StandardPBR.materialtype","materialTypeVersion":6})",
            sourcePath).IsSuccess());
        ASSERT_TRUE(AZ::Utils::WriteFile("cache product sentinel", cachePath).IsSuccess());
        CachedMaterialModel browser(m_directory.path().toUtf8().constData());
        ASSERT_TRUE(browser.m_sourceAdded);
        const auto sourceIndex = browser.GetAssetBrowserModel()->findIndex(
            QDir::fromNativeSeparators(QString::fromUtf8(sourcePath.c_str())));
        ASSERT_TRUE(sourceIndex.isValid());
        const auto* source = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>(
            sourceIndex.data(AzToolsFramework::AssetBrowser::AssetBrowserModel::Roles::EntryRole)
                .value<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>());
        ASSERT_NE(source, nullptr);
        AzToolsFramework::AssetBrowser::ProductWithUuid productData;
        productData.first = source->GetSourceUuid();
        productData.second.m_productID = 1;
        productData.second.m_subID = 0;
        productData.second.m_productName = "pc/inline.azmaterial";
        productData.second.m_assetType = azrtti_typeid<JE::Render::RPI::MaterialAsset>();
        ASSERT_TRUE(browser.GetAssetBrowserModel()->GetRootEntry()->AddProduct(productData));
        auto* product = AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry::GetProductByAssetId(
            AZ::Data::AssetId(productData.first, productData.second.m_subID));
        ASSERT_NE(product, nullptr);
        ASSERT_EQ(product->GetParent(), source);
        product->SetFullPath(AZ::IO::Path(cachePath));
        UnopenedPaneRequests panes;
        InlineInspectorProvider provider(session);
        AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget inspector;
        AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequest::PreviewAsset, source);
        inspector.show();
        QApplication::processEvents();
        auto* fromSource = inspector.findChild<QWidget*>("MaterialAssetInspector");
        ASSERT_NE(fromSource, nullptr);
        EXPECT_TRUE(fromSource->isVisibleTo(&inspector));
        const auto id = session.GetActiveDocumentId();
        ASSERT_FALSE(id.IsNull());
        AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequest::PreviewAsset, product);
        auto* fromProduct = inspector.findChild<QWidget*>("MaterialAssetInspector");
        EXPECT_EQ(fromProduct, fromSource);
        EXPECT_EQ(session.GetActiveDocumentId(), id);
        EXPECT_EQ(session.GetDocumentIds().size(), 1);
        EXPECT_EQ(opens, 1);
        EXPECT_EQ(panes.m_showRequests, 0);
        EXPECT_EQ(panes.m_widgetRequests, 0);
        ASSERT_NE(fromSource->findChild<QPushButton*>("MaterialInspectorSave"), nullptr);
        AZStd::string actualPath;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            actualPath, id, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
        EXPECT_EQ(QDir::fromNativeSeparators(QString::fromUtf8(actualPath.c_str())),
            QDir::fromNativeSeparators(QString::fromUtf8(sourcePath.c_str())));
        auto cacheAfter = AZ::Utils::ReadFile<AZStd::string>(cachePath);
        ASSERT_TRUE(cacheAfter.IsSuccess());
        EXPECT_EQ(cacheAfter.GetValue(), "cache product sentinel");
    }

    TEST_F(TextureContextMenuTests, NonMaterialAndClearFallBackWithoutClosingTheInlineDocumentOrWritingItsSource)
    {
        int opens = 0;
        JE::Editor::MaterialWorkspaceSession session(SourceType(opens));
        const AZStd::string path = m_directory.filePath("retained-inline.material").toUtf8().constData();
        const AZStd::string sourceText =
            R"({"materialType":"@engroot@/Engine/Render/Features/Assets/Materials/Types/StandardPBR.materialtype","materialTypeVersion":6})";
        ASSERT_TRUE(AZ::Utils::WriteFile(sourceText, path).IsSuccess());
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry source;
        source.SetFullPath(AZ::IO::Path(path));
        InlineInspectorProvider provider(session);
        AzToolsFramework::AssetBrowser::AssetBrowserEntityInspectorWidget inspector;
        AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequest::PreviewAsset, &source);
        inspector.show();
        QApplication::processEvents();
        auto* view = inspector.findChild<QWidget*>("MaterialAssetInspector");
        ASSERT_NE(view, nullptr);
        const auto id = session.GetActiveDocumentId();
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry other;
        other.SetFullPath(AZ::IO::Path(m_directory.filePath("model.fbx").toUtf8().constData()));
        AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequest::PreviewAsset, &other);
        EXPECT_FALSE(view->isVisibleTo(&inspector));
        const auto* settings = inspector.findChild<QPushButton*>("AssetInspectorSettings");
        ASSERT_NE(settings, nullptr);
        EXPECT_EQ(settings->text(), "Scene Settings");
        AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequest::ClearPreview);
        EXPECT_EQ(session.GetDocumentIds().size(), 1);
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
        auto after = AZ::Utils::ReadFile<AZStd::string>(path);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), sourceText);
    }

    TEST_F(GraphDocumentCreationTests, CreatesAtTheFinalPathBeforeEditingAndRemovesItsTemporary)
    {
        const AZStd::string path = m_directory.filePath("created.workspace").toUtf8().constData();
        JE::Editor::MaterialWorkspaceSession session(EditableDocumentType());
        const auto id = session.CreateDocument({}, path);
        ASSERT_FALSE(id.IsNull());
        EXPECT_EQ(session.GetActiveDocumentId(), id);
        EXPECT_EQ(session.GetDocumentIds().size(), 1);
        AZStd::string actualPath;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(actualPath, id,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
        EXPECT_EQ(QDir::fromNativeSeparators(QString::fromUtf8(actualPath.c_str())),
            QDir::fromNativeSeparators(QString::fromUtf8(path.c_str())));
        auto json = AZ::JsonSerializationUtils::ReadJsonFile(path);
        ASSERT_TRUE(json.IsSuccess());
        EXPECT_FALSE(IsModified(id));
        Edit(id);
        EXPECT_TRUE(IsModified(id));
        EXPECT_EQ(QDir(m_directory.path()).entryList({".je-create-*"}, QDir::Files | QDir::Hidden).size(), 0);
    }

    TEST_F(GraphDocumentCreationTests, MaterialPaneKeepsItsDocumentWhileANodeDocumentOpens)
    {
        JE::Editor::MaterialWorkspaceSession session(EditableDocumentType());
        const auto nodeType = JE::Editor::Tools::AtomToolsAnyDocument::BuildDocumentTypeInfo(
            "Workspace node config", {"materialgraphnode"}, {}, AZStd::any(0.25f), AZ::Uuid::CreateNull());
        session.RegisterGraphDocumentTypes({nodeType});
        const AZStd::string materialPath = m_directory.filePath("material.workspace").toUtf8().constData();
        const auto materialId = session.CreateDocument({}, materialPath);
        ASSERT_FALSE(materialId.IsNull());
        {
            JE::Editor::MaterialWorkspaceWidget pane(session);
            auto* documents = pane.findChild<QComboBox*>();
            ASSERT_NE(documents, nullptr);
            ASSERT_EQ(documents->count(), 1);
            const AZStd::string nodePath = m_directory.filePath("node.materialgraphnode").toUtf8().constData();
            const auto nodeId = session.CreateDocument({}, nodePath);
            ASSERT_FALSE(nodeId.IsNull());
            EXPECT_TRUE(session.IsGraphDocument(nodeId));
            EXPECT_EQ(session.GetActiveDocumentId(), nodeId);
            EXPECT_EQ(documents->count(), 1);
            EXPECT_EQ(AZ::Uuid::CreateString(documents->currentData().toString().toUtf8().constData()), materialId);
            pane.close();
            EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(nodeId));
        }
        JE::Editor::MaterialWorkspaceWidget reopened(session);
        auto* documents = reopened.findChild<QComboBox*>();
        ASSERT_NE(documents, nullptr);
        EXPECT_EQ(documents->count(), 1);
        EXPECT_EQ(AZ::Uuid::CreateString(documents->currentData().toString().toUtf8().constData()), materialId);
        EXPECT_EQ(session.GetDocumentIds().size(), 2);
    }

    TEST_F(GraphDocumentCreationTests, RetainedPaneBecomesInertBeforeItsSessionDestroysDocuments)
    {
        const AZStd::string path = m_directory.filePath("retained.workspace").toUtf8().constData();
        auto session = AZStd::make_unique<JE::Editor::MaterialWorkspaceSession>(EditableDocumentType());
        const auto id = session->CreateDocument({}, path);
        ASSERT_FALSE(id.IsNull());
        auto pane = AZStd::make_unique<JE::Editor::MaterialWorkspaceWidget>(*session);
        ASSERT_TRUE(pane->isEnabled());
        Edit(id);
        ASSERT_TRUE(IsModified(id));
        pane->show();
        QApplication::processEvents();
        pane->hide();
        EXPECT_TRUE(pane->isEnabled());
        EXPECT_EQ(session->GetActiveDocumentId(), id);
        bool canUndo = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            canUndo, id, &JE::Editor::Tools::AtomToolsDocumentRequests::CanUndo);
        EXPECT_TRUE(canUndo);

        class CloseObserver final : public JE::Editor::Tools::AtomToolsDocumentNotificationBus::Handler
        {
        public:
            CloseObserver(QWidget& pane, const AZ::Uuid& id)
                : m_pane(pane), m_id(id)
            {
                BusConnect(JE::Editor::MaterialWorkspaceSession::GetToolId());
            }
            ~CloseObserver() override { BusDisconnect(); }
            void OnDocumentClosed(const AZ::Uuid& id) override
            {
                if (id == m_id)
                {
                    EXPECT_FALSE(m_pane.isEnabled());
                    EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
                    m_closed = true;
                }
            }
            void OnDocumentDestroyed(const AZ::Uuid& id) override
            {
                if (id == m_id)
                {
                    EXPECT_FALSE(m_pane.isEnabled());
                    m_destroyed = true;
                }
            }
            QWidget& m_pane;
            AZ::Uuid m_id;
            bool m_closed = false;
            bool m_destroyed = false;
        } observer(*pane, id);

        // The view manager may retain a hidden pane after its session owner shuts down.
        session.reset();
        EXPECT_TRUE(observer.m_closed);
        EXPECT_TRUE(observer.m_destroyed);
        EXPECT_FALSE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(id));
        EXPECT_FALSE(pane->isEnabled());
        pane->show();
        QApplication::processEvents();
        EXPECT_FALSE(pane->isEnabled());

        const AZStd::string nextPath = m_directory.filePath("next.workspace").toUtf8().constData();
        JE::Editor::MaterialWorkspaceSession nextSession(EditableDocumentType());
        const auto nextId = nextSession.CreateDocument({}, nextPath);
        ASSERT_FALSE(nextId.IsNull());
        QApplication::processEvents();
        EXPECT_FALSE(pane->isEnabled());
        EXPECT_TRUE(pane->OpenDocument(nextPath).IsNull());
        EXPECT_EQ(nextSession.GetActiveDocumentId(), nextId);
        EXPECT_EQ(nextSession.GetDocumentIds().size(), 1);
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(nextId));
        pane->hide();
    }

    TEST_F(GraphDocumentCreationTests, CompactMaterialInspectorSavesItsShownDocumentWithoutSavingAnotherDirtyDocument)
    {
        JE::Editor::MaterialWorkspaceSession session(EditableDocumentType());
        const AZStd::string firstPath = m_directory.filePath("inspector-first.workspace").toUtf8().constData();
        const AZStd::string secondPath = m_directory.filePath("inspector-second.workspace").toUtf8().constData();
        const auto first = session.CreateDocument({}, firstPath);
        const auto second = session.CreateDocument({}, secondPath);
        ASSERT_FALSE(first.IsNull());
        ASSERT_FALSE(second.IsNull());
        JE::Editor::MaterialWorkspaceWidget inspector(session, nullptr, true);
        ASSERT_EQ(inspector.OpenDocument(firstPath), first);
        Edit(first);
        Edit(second);
        ASSERT_TRUE(session.SelectDocument(second));
        auto* save = inspector.findChild<QPushButton*>("MaterialInspectorSave");
        auto* status = inspector.findChild<QLabel*>("MaterialInspectorStatus");
        ASSERT_NE(save, nullptr);
        ASSERT_NE(status, nullptr);
        EXPECT_TRUE(status->text().contains("inspector-first.workspace"));
        EXPECT_TRUE(status->text().contains('*'));
        ASSERT_TRUE(save->isEnabled());
        auto secondBefore = AZ::Utils::ReadFile<AZStd::string>(secondPath);
        ASSERT_TRUE(secondBefore.IsSuccess());
        save->click();
        EXPECT_FALSE(IsModified(first));
        EXPECT_TRUE(IsModified(second));
        EXPECT_FALSE(status->text().contains('*'));
        EXPECT_FALSE(save->isEnabled());
        auto firstSaved = AZ::Utils::ReadFile<AZStd::string>(firstPath);
        auto secondAfter = AZ::Utils::ReadFile<AZStd::string>(secondPath);
        ASSERT_TRUE(firstSaved.IsSuccess());
        ASSERT_TRUE(secondAfter.IsSuccess());
        EXPECT_NE(firstSaved.GetValue().find("0.75"), AZStd::string::npos);
        EXPECT_EQ(secondBefore.GetValue(), secondAfter.GetValue());
    }

    TEST_F(GraphDocumentCreationTests, CompactMaterialInspectorRetainsDirtyDocumentAndHistoryAcrossRebinding)
    {
        JE::Editor::MaterialWorkspaceSession session(EditableDocumentType());
        const AZStd::string firstPath = m_directory.filePath("dirty-inspector.workspace").toUtf8().constData();
        const AZStd::string secondPath = m_directory.filePath("other-inspector.workspace").toUtf8().constData();
        const auto first = session.CreateDocument({}, firstPath);
        const auto second = session.CreateDocument({}, secondPath);
        ASSERT_FALSE(first.IsNull());
        ASSERT_FALSE(second.IsNull());
        JE::Editor::MaterialWorkspaceWidget inspector(session, nullptr, true);
        ASSERT_EQ(inspector.OpenDocument(firstPath), first);
        auto before = AZ::Utils::ReadFile<AZStd::string>(firstPath);
        ASSERT_TRUE(before.IsSuccess());
        Edit(first);
        EXPECT_EQ(inspector.OpenDocument(secondPath), second);
        EXPECT_TRUE(IsModified(first));
        EXPECT_EQ(inspector.OpenDocument(firstPath), first);
        EXPECT_EQ(session.GetDocumentIds().size(), 2);
        bool canUndo = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            canUndo, first, &JE::Editor::Tools::AtomToolsDocumentRequests::CanUndo);
        EXPECT_TRUE(canUndo);
        auto after = AZ::Utils::ReadFile<AZStd::string>(firstPath);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(before.GetValue(), after.GetValue());
        inspector.show();
        QApplication::processEvents();
        EXPECT_TRUE(inspector.findChildren<QSplitter*>(QString(), Qt::FindDirectChildrenOnly).isEmpty());
    }

    TEST_F(GraphDocumentCreationTests, CompactMaterialInspectorRejectsRebindingWhenEndEditFails)
    {
        class CommitBoundary final : public JE::Editor::Tools::AtomToolsAnyDocument
        {
        public:
            AZ_CLASS_ALLOCATOR(CommitBoundary, AZ::SystemAllocator);
            CommitBoundary(const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& type, bool& reject)
                : AtomToolsAnyDocument(toolId, type, AZStd::any(0.25f), AZ::Uuid::CreateNull()), m_reject(reject) {}
            bool EndEdit() override { return !m_reject && AtomToolsAnyDocument::EndEdit(); }
            bool& m_reject;
        };
        bool reject = false;
        auto type = EditableDocumentType();
        type.m_documentFactoryCallback = [&reject](const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& info)
            -> JE::Editor::Tools::AtomToolsDocumentRequests* { return aznew CommitBoundary(toolId, info, reject); };
        JE::Editor::MaterialWorkspaceSession session(type);
        const AZStd::string firstPath = m_directory.filePath("commit-first.workspace").toUtf8().constData();
        const AZStd::string secondPath = m_directory.filePath("commit-second.workspace").toUtf8().constData();
        const auto first = session.CreateDocument({}, firstPath);
        const auto second = session.CreateDocument({}, secondPath);
        ASSERT_FALSE(first.IsNull());
        ASSERT_FALSE(second.IsNull());
        JE::Editor::MaterialWorkspaceWidget inspector(session, nullptr, true);
        ASSERT_EQ(inspector.OpenDocument(firstPath), first);
        Edit(first);
        ASSERT_TRUE(session.SelectDocument(second));
        reject = true;
        EXPECT_TRUE(inspector.OpenDocument(secondPath).IsNull());
        auto* status = inspector.findChild<QLabel*>("MaterialInspectorStatus");
        ASSERT_NE(status, nullptr);
        EXPECT_TRUE(status->text().contains("commit-first.workspace"));
        EXPECT_TRUE(IsModified(first));
        EXPECT_EQ(session.GetActiveDocumentId(), second);
        reject = false;
    }

    TEST_F(GraphDocumentCreationTests, CompactMaterialInspectorDoesNotOfferOrPerformSaveForReadOnlySource)
    {
        JE::Editor::MaterialWorkspaceSession session(EditableDocumentType());
        const QString filePath = m_directory.filePath("readonly-inspector.workspace");
        const AZStd::string path = filePath.toUtf8().constData();
        const auto id = session.CreateDocument({}, path);
        ASSERT_FALSE(id.IsNull());
        auto before = AZ::Utils::ReadFile<AZStd::string>(path);
        ASSERT_TRUE(before.IsSuccess());
        Edit(id);
        const auto permissions = QFile::permissions(filePath);
        const auto restore = qScopeGuard([&]() { QFile::setPermissions(filePath, permissions); });
        ASSERT_TRUE(QFile::setPermissions(filePath, QFileDevice::ReadOwner | QFileDevice::ReadUser |
            QFileDevice::ReadGroup | QFileDevice::ReadOther));
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->IsReadOnly(path.c_str()));
        JE::Editor::MaterialWorkspaceWidget inspector(session, nullptr, true);
        ASSERT_EQ(inspector.OpenDocument(path), id);
        auto* save = inspector.findChild<QPushButton*>("MaterialInspectorSave");
        auto* status = inspector.findChild<QLabel*>("MaterialInspectorStatus");
        auto* properties = inspector.findChild<QWidget*>("MaterialDocumentInspector");
        ASSERT_NE(save, nullptr);
        ASSERT_NE(status, nullptr);
        ASSERT_NE(properties, nullptr);
        EXPECT_FALSE(save->isEnabled());
        EXPECT_FALSE(properties->isEnabled());
        EXPECT_TRUE(status->text().contains("read only"));
        QKeyEvent shortcut(QEvent::KeyPress, Qt::Key_S, Qt::ControlModifier);
        QApplication::sendEvent(status, &shortcut);
        EXPECT_TRUE(IsModified(id));
        auto after = AZ::Utils::ReadFile<AZStd::string>(path);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), before.GetValue());
    }

    TEST_F(GraphDocumentCreationTests, RejectsExistingAndRelativeDestinationsWithoutChangingThem)
    {
        const AZStd::string path = m_directory.filePath("existing.workspace").toUtf8().constData();
        ASSERT_TRUE(AZ::Utils::WriteFile("existing sentinel", path).IsSuccess());
        JE::Editor::MaterialWorkspaceSession session(EditableDocumentType());
        EXPECT_TRUE(session.CreateDocument({}, path).IsNull());
        EXPECT_TRUE(session.CreateDocument({}, "relative.workspace").IsNull());
        EXPECT_TRUE(session.GetDocumentIds().empty());
        auto existing = AZ::Utils::ReadFile<AZStd::string>(path);
        ASSERT_TRUE(existing.IsSuccess());
        EXPECT_EQ(existing.GetValue(), "existing sentinel");
        EXPECT_EQ(QDir(m_directory.path()).entryList({".je-create-*"}, QDir::Files | QDir::Hidden).size(), 0);
    }

    TEST_F(GraphDocumentCreationTests, DestinationAppearingDuringSaveIsPreservedAndTheNewDocumentIsDestroyed)
    {
        class ConcurrentDestinationDocument final : public JE::Editor::Tools::AtomToolsAnyDocument
        {
        public:
            AZ_CLASS_ALLOCATOR(ConcurrentDestinationDocument, AZ::SystemAllocator);
            ConcurrentDestinationDocument(const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& type, const AZStd::string& target)
                : AtomToolsAnyDocument(toolId, type, AZStd::any(0.25f), AZ::Uuid::CreateNull()), m_target(target) {}
            bool SaveAsChild(const AZStd::string& path) override
            {
                if (!AtomToolsAnyDocument::SaveAsChild(path)) { return false; }
                // Introduce the competing file after the native temporary save and before publication.
                return AZ::Utils::WriteFile("concurrent sentinel", m_target).IsSuccess();
            }
        private:
            AZStd::string m_target;
        };
        const AZStd::string path = m_directory.filePath("concurrent.workspace").toUtf8().constData();
        auto type = EditableDocumentType();
        type.m_documentFactoryCallback = [path](const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& info)
            -> JE::Editor::Tools::AtomToolsDocumentRequests*
        {
            return aznew ConcurrentDestinationDocument(toolId, info, path);
        };
        JE::Editor::MaterialWorkspaceSession session(type);
        EXPECT_TRUE(session.CreateDocument({}, path).IsNull());
        EXPECT_TRUE(session.GetDocumentIds().empty());
        auto existing = AZ::Utils::ReadFile<AZStd::string>(path);
        ASSERT_TRUE(existing.IsSuccess());
        EXPECT_EQ(existing.GetValue(), "concurrent sentinel");
        EXPECT_EQ(QDir(m_directory.path()).entryList({".je-create-*"}, QDir::Files | QDir::Hidden).size(), 0);
    }

    TEST_F(MaterialSourceCreationTests, CreatesNativeMaterialSchemaWithTheSelectedTypeVersion)
    {
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        const AZStd::string typePath = directory.filePath("source.materialtype").toUtf8().constData();
        const AZStd::string targetPath = directory.filePath("created.material").toUtf8().constData();
        ASSERT_TRUE(AZ::Utils::WriteFile(R"({"version":7})", typePath).IsSuccess());

        auto created = JE::Editor::MaterialBrowserInteractions::CreateMaterialSourceFile(typePath, targetPath);
        ASSERT_TRUE(created.IsSuccess()) << created.GetError().c_str();
        auto json = AZ::JsonSerializationUtils::ReadJsonFile(targetPath);
        ASSERT_TRUE(json.IsSuccess());
        const auto& document = json.GetValue();
        ASSERT_TRUE(document.HasMember("materialType"));
        ASSERT_TRUE(document.HasMember("materialTypeVersion"));
        EXPECT_STREQ(document["materialType"].GetString(), "source.materialtype");
        EXPECT_EQ(document["materialTypeVersion"].GetUint(), 7);
        EXPECT_FALSE(document.HasMember("ClassData"));
        auto source = AZ::JsonSerializationUtils::ReadJsonFile(typePath);
        ASSERT_TRUE(source.IsSuccess());
        EXPECT_EQ(source.GetValue()["version"].GetUint(), 7);
    }

    TEST_F(MaterialSourceCreationTests, CreatesFromTheAliasedSourceReturnedByTheNativeDialog)
    {
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        const AZStd::string typePath = directory.filePath("source.materialtype").toUtf8().constData();
        ASSERT_TRUE(AZ::Utils::WriteFile(R"({"version":7})", typePath).IsSuccess());
        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        constexpr const char* alias = "@gemroot:JEWorkspaceCreationTest@";
        ASSERT_EQ(fileIO->GetAlias(alias), nullptr);
        const QByteArray folder = directory.path().toUtf8();
        fileIO->SetAlias(alias, folder.constData());
        struct AliasGuard
        {
            AZ::IO::FileIOBase* m_fileIO;
            const char* m_alias;
            ~AliasGuard() { m_fileIO->ClearAlias(m_alias); }
        } aliasGuard{fileIO, alias};

        JE::Editor::Tools::CreateDocumentDialog dialog(
            "Create Material", "Select Material Type", "Select Material Destination", directory.path(), {"material"}, {},
            [](const AZStd::string& path) { return path.ends_with(".materialtype"); });
        auto* selector = dialog.findChild<JE::Editor::Tools::AssetSelectionComboBox*>();
        ASSERT_NE(selector, nullptr);
        selector->AddPath(typePath);
        selector->SelectPath(typePath);
        // Use the real dialog fields: the selected source is aliased, while the destination is absolute.
        const AZStd::string selectedSource = dialog.m_sourcePath.toUtf8().constData();
        const AZStd::string targetPath = dialog.m_targetPath.toUtf8().constData();
        ASSERT_EQ(selectedSource, AZStd::string(alias) + "/source.materialtype");
        ASSERT_FALSE(targetPath.empty());
        ASSERT_TRUE(QFileInfo(dialog.m_targetPath).isAbsolute());
        auto created = JE::Editor::MaterialBrowserInteractions::CreateMaterialSourceFile(selectedSource, targetPath);
        ASSERT_TRUE(created.IsSuccess()) << created.GetError().c_str();
        auto json = AZ::JsonSerializationUtils::ReadJsonFile(targetPath);
        ASSERT_TRUE(json.IsSuccess());
        ASSERT_TRUE(json.GetValue().HasMember("materialType"));
        ASSERT_TRUE(json.GetValue().HasMember("materialTypeVersion"));
        EXPECT_STREQ(json.GetValue()["materialType"].GetString(), "source.materialtype");
        EXPECT_EQ(json.GetValue()["materialTypeVersion"].GetUint(), 7);
        EXPECT_FALSE(json.GetValue().HasMember("ClassData"));
    }

    TEST_F(MaterialSourceCreationTests, ExistingDestinationAndMaterialTypeAreNeverOverwritten)
    {
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        const AZStd::string typePath = directory.filePath("source.materialtype").toUtf8().constData();
        const AZStd::string targetPath = directory.filePath("existing.material").toUtf8().constData();
        ASSERT_TRUE(AZ::Utils::WriteFile(R"({"version":7})", typePath).IsSuccess());
        ASSERT_TRUE(AZ::Utils::WriteFile("existing sentinel", targetPath).IsSuccess());
        EXPECT_FALSE(JE::Editor::MaterialBrowserInteractions::CreateMaterialSourceFile(typePath, targetPath).IsSuccess());
        EXPECT_FALSE(JE::Editor::MaterialBrowserInteractions::CreateMaterialSourceFile(typePath, typePath).IsSuccess());
        auto existing = AZ::Utils::ReadFile<AZStd::string>(targetPath);
        ASSERT_TRUE(existing.IsSuccess());
        EXPECT_EQ(existing.GetValue(), "existing sentinel");
        auto type = AZ::Utils::ReadFile<AZStd::string>(typePath);
        ASSERT_TRUE(type.IsSuccess());
        EXPECT_EQ(type.GetValue(), R"({"version":7})");
    }

    TEST_F(MaterialSourceCreationTests, MissingMaterialTypeFailsWithoutCreatingADestination)
    {
        class LoadErrorRecorder : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            LoadErrorRecorder() { BusConnect(); }
            ~LoadErrorRecorder() override { BusDisconnect(); }
            bool OnPreError(const char* window, const char*, int, const char*, const char* message) override
            {
                if (AZStd::string_view(window) == "MaterialUtils")
                {
                    m_errors.emplace_back(message);
                    return true;
                }
                return false;
            }
            AZStd::vector<AZStd::string> m_errors;
        } recorder;
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        const AZStd::string typePath = directory.filePath("missing.materialtype").toUtf8().constData();
        const AZStd::string targetPath = directory.filePath("created.material").toUtf8().constData();
        const auto created = JE::Editor::MaterialBrowserInteractions::CreateMaterialSourceFile(typePath, targetPath);
        EXPECT_FALSE(created.IsSuccess());
        ASSERT_EQ(recorder.m_errors.size(), 1);
        EXPECT_NE(recorder.m_errors.front().find("missing.materialtype"), AZStd::string::npos);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(targetPath.c_str()));
    }

    TEST_F(MaterialSourceCreationTests, DestinationWriteFailureIsReportedWithoutCreatingAMaterial)
    {
        QTemporaryDir directory;
        ASSERT_TRUE(directory.isValid());
        const AZStd::string typePath = directory.filePath("source.materialtype").toUtf8().constData();
        const AZStd::string targetPath = directory.filePath("absent/created.material").toUtf8().constData();
        ASSERT_TRUE(AZ::Utils::WriteFile(R"({"version":7})", typePath).IsSuccess());
        const auto created = JE::Editor::MaterialBrowserInteractions::CreateMaterialSourceFile(typePath, targetPath);
        ASSERT_FALSE(created.IsSuccess());
        EXPECT_NE(created.GetError().find("Failed to create"), AZStd::string::npos);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(targetPath.c_str()));
    }
}
