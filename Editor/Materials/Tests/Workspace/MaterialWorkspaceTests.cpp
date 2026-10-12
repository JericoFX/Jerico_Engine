/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include "Workspace/MaterialWorkspaceSession.h"
#include "Workspace/MaterialWorkspaceWidget.h"
#include <JE/Editor/Materials/MaterialBrowserInteractions.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>

#include <AtomToolsFramework/Document/AtomToolsAnyDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentInspector.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/AssetSelection/AssetSelectionComboBox.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
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
#include <AzTest/AzTest.h>
#include <QApplication>
#include <QAbstractButton>
#include <QMessageBox>
#include <QTimer>
#include <QTemporaryDir>
#include <QImage>

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
            AtomToolsFramework::InspectorWidget::Reflect(m_application->GetSerializeContext());
        }

        void TearDown() override
        {
            m_application->Destroy();
            m_application.reset();
            m_qtApplication.reset();
            LeakDetectionFixture::TearDown();
        }

        AtomToolsFramework::DocumentTypeInfo EmptyDocumentType()
        {
            AtomToolsFramework::DocumentTypeInfo type;
            type.m_documentTypeName = "Workspace lifecycle";
            type.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& info)
            {
                return aznew AtomToolsFramework::AtomToolsDocument(toolId, info);
            };
            return type;
        }

        AZ::Uuid CreateDocument(const JE::Editor::MaterialWorkspaceSession& session)
        {
            AZ::Uuid id;
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(id, session.GetToolId(),
                &AtomToolsFramework::AtomToolsDocumentSystemRequests::CreateDocumentFromTypeName,
                session.GetDocumentType().m_documentTypeName);
            return id;
        }

        AtomToolsFramework::DocumentTypeInfo EditableDocumentType()
        {
            // Use native document editing/serialization to test session lifetime without an asset/GPU pipeline.
            return AtomToolsFramework::AtomToolsAnyDocument::BuildDocumentTypeInfo(
                "Workspace value", {"workspace"}, {}, AZStd::any(0.25f), AZ::Uuid::CreateNull());
        }

        void Edit(const AZ::Uuid& id)
        {
            bool success = false;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                success, id, &AtomToolsFramework::AtomToolsDocumentRequests::BeginEdit);
            ASSERT_TRUE(success);
            AtomToolsFramework::DocumentObjectInfoVector objects;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                objects, id, &AtomToolsFramework::AtomToolsDocumentRequests::GetObjectInfo);
            ASSERT_EQ(objects.size(), 1);
            ASSERT_EQ(objects.front().m_objectType, azrtti_typeid<float>());
            *static_cast<float*>(objects.front().m_objectPtr) = 0.75f;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                success, id, &AtomToolsFramework::AtomToolsDocumentRequests::EndEdit);
            ASSERT_TRUE(success);
        }

        bool IsModified(const AZ::Uuid& id)
        {
            bool modified = false;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                modified, id, &AtomToolsFramework::AtomToolsDocumentRequests::IsModified);
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
        auto* inspector = new AtomToolsFramework::AtomToolsDocumentInspector(JE::Editor::MaterialWorkspaceSession::GetToolId());
        delete inspector;
    }

    TEST_F(MaterialWorkspaceTests, SelectedInspectorDestruction)
    {
        JE::Editor::MaterialWorkspaceSession session(EmptyDocumentType());
        const auto id = CreateDocument(session);
        ASSERT_FALSE(id.IsNull());
        auto* inspector = new AtomToolsFramework::AtomToolsDocumentInspector(JE::Editor::MaterialWorkspaceSession::GetToolId());
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
        ASSERT_TRUE(AtomToolsFramework::AtomToolsDocumentRequestBus::HasHandlers(id));
        ASSERT_EQ(session.GetDocumentIds().size(), 1);
        EXPECT_EQ(session.GetActiveDocumentId(), id);
        JE::Editor::MaterialWorkspaceWidget reopened(session);
        EXPECT_EQ(session.GetActiveDocumentId(), id);
        EXPECT_TRUE(AtomToolsFramework::AtomToolsDocumentRequestBus::HasHandlers(id));
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
        EXPECT_TRUE(AtomToolsFramework::AtomToolsDocumentRequestBus::HasHandlers(id));
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
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            canUndo, id, &AtomToolsFramework::AtomToolsDocumentRequests::CanUndo);
        EXPECT_TRUE(canUndo);
        AtomToolsFramework::DocumentObjectInfoVector objects;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            objects, id, &AtomToolsFramework::AtomToolsDocumentRequests::GetObjectInfo);
        ASSERT_EQ(objects.size(), 1);
        EXPECT_FLOAT_EQ(*static_cast<const float*>(objects.front().m_objectPtr), 0.75f);
        // A later close attempt can still be cancelled after another editor pane vetoes exit.
        observed = false;
        ChooseCloseDecision(QMessageBox::Cancel, observed);
        EXPECT_FALSE(session.ConfirmApplicationClose(nullptr));
        EXPECT_TRUE(observed);
        EXPECT_TRUE(AtomToolsFramework::AtomToolsDocumentRequestBus::HasHandlers(id));
    }

    TEST_F(MaterialWorkspaceTests, SessionClosesBeforeDestroyingItsDocuments)
    {
        class CloseRecorder : public AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        {
        public:
            CloseRecorder() { BusConnect(JE::Editor::MaterialWorkspaceSession::GetToolId()); }
            ~CloseRecorder() override { BusDisconnect(); }
            void OnDocumentClosed(const AZ::Uuid& id) override
            {
                EXPECT_TRUE(AtomToolsFramework::AtomToolsDocumentRequestBus::HasHandlers(id));
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
        EXPECT_FALSE(AtomToolsFramework::AtomToolsDocumentRequestBus::HasHandlers(id));
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
            reflectionManager->Reflect(azrtti_typeid<AZ::RPI::MaterialTypeSourceData>(), &AZ::RPI::MaterialTypeSourceData::Reflect);
            reflectionManager->Reflect(azrtti_typeid<AZ::RPI::MaterialSourceData>(), &AZ::RPI::MaterialSourceData::Reflect);
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

        AtomToolsFramework::CreateDocumentDialog dialog(
            "Create Material", "Select Material Type", "Select Material Destination", directory.path(), {"material"}, {},
            [](const AZStd::string& path) { return path.ends_with(".materialtype"); });
        auto* selector = dialog.findChild<AtomToolsFramework::AssetSelectionComboBox*>();
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
