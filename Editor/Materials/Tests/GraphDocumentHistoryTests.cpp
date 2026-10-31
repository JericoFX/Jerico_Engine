/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/Materials/Graph/MaterialCanvasDocumentHistory.h>
#if !defined(AZ_RELEASE_BUILD)
#include <JE/Editor/Materials/EditorMaterialSystemComponent.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#endif
#include <JE/Editor/Tools/SettingsDialog/SettingsDialog.h>
#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentSystemRequestBus.h>
#include <JE/Editor/Materials/Graph/MaterialGraphCompiler.h>
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>
#include "Workspace/MaterialWorkspaceSession.h"
#include "Workspace/MaterialGraphWorkspaceWidget.h"
#include <JE/Editor/Tools/Inspector/InspectorWidget.h>
#include <JE/Editor/Tools/Inspector/InspectorPropertyGroupWidget.h>
#include <JE/Render/RPI/Public/RPISystemInterface.h>
#include <JE/Editor/Tools/Graph/GraphDocumentNotificationBus.h>
#include <JE/Editor/Tools/DynamicProperty/DynamicProperty.h>
#include <JE/Editor/Tools/DynamicProperty/DynamicPropertyGroup.h>
#include <JE/Editor/Render/RPI/Edit/Material/LuaMaterialFunctorSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataHolder.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertySourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Render/RPI/Reflect/Material/MaterialFunctor.h>
#include <JE/Render/RPI/Reflect/Material/MaterialPropertyValue.h>
#include <JE/Render/RPI/Reflect/Material/MaterialTypeAssetCreator.h>
#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <AzCore/Asset/AssetManager.h>
#include <JE/Render/RPI/Public/Material/MaterialShaderParameterLayout.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <JE/Editor/Tools/Graph/AssetStatusReporterSystemRequestBus.h>
#include <JE/Editor/Tools/Graph/AssetStatusReporter.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNode.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodeManager.h>
#include <JE/Editor/Tools/Graph/GraphTemplateFileDataCache.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <JE/Editor/Tools/Graph/GraphCompiler.h>
#include <JE/Editor/Tools/Graph/GraphDocument.h>
#include <JE/Editor/Tools/Graph/GraphDocumentView.h>
#include <JE/Editor/Tools/Graph/GraphView.h>
#include <JE/Editor/Tools/Graph/GraphViewConstructPresets.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <GraphCanvas/GraphCanvasModuleRegistration.h>
#include <GraphCanvas/GraphCanvas.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Task/TaskGraphSystemComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <JE/Editor/Tools/GraphCanvas/Components/GeometryBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/SceneBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/ViewBus.h>
#include <JE/Editor/Tools/GraphCanvas/GraphCanvasBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/AssetEditorBus.h>
#include <JE/Editor/Tools/GraphCanvas/Editor/GraphModelBus.h>
#include <JE/Editor/Tools/GraphCanvas/Utils/GraphUtils.h>
#include <JE/Editor/Tools/GraphCanvas/Styling/StyleManager.h>
#include <QDataStream>
#include <JE/Editor/Tools/GraphCanvas/Types/QtMetaTypes.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <GraphModelSystemComponent.h>
#include <JE/Editor/Tools/GraphModel/Model/Connection.h>
#include <JE/Editor/Tools/GraphModel/Model/GraphContext.h>
#include <JE/Editor/Tools/GraphModel/Model/Module/InputOutputNodes.h>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include <QMainWindow>
#include <QWindow>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Buses/ShortcutDispatch.h>
#include <QCoreApplication>
#include <QEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/SlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>
#include <JE/Editor/Tools/GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <QGraphicsLayout>
#include <QDoubleSpinBox>
#include <QKeyEvent>
#include <QPointer>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFont>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QDockWidget>
#include <QRegion>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QSplitterHandle>
#include <QTabWidget>

namespace UnitTest
{
    namespace
    {
        constexpr AZ::Crc32 CanvasHistoryToolId = AZ_CRC_CE("GraphDocumentHistoryTest");

        // Use the application's real module/reflection/entity lifecycle, without Editor startup, AP or networking.
        class CanvasHistoryApplication final : public AzToolsFramework::ToolsApplication
        {
        public:
            AZ_CLASS_ALLOCATOR(CanvasHistoryApplication, AZ::SystemAllocator);
            void InitializeUndo() { m_undoStack = new AzToolsFramework::UndoSystem::UndoStack(10, nullptr); }
            ~CanvasHistoryApplication() override
            {
                delete m_undoStack;
                m_undoStack = nullptr;
            }
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return {azrtti_typeid<AZ::TaskGraphSystemComponent>(), azrtti_typeid<AZ::JobManagerComponent>(),
                    azrtti_typeid<AZ::StreamerComponent>(), azrtti_typeid<AZ::AssetManagerComponent>()};
            }
            void CreateStaticModules(AZStd::vector<AZ::Module*>& modules) override
            {
                AZ::ComponentApplication::CreateStaticModules(modules);
            }
            using ToolsApplication::FlushUndo;
            using ToolsApplication::GetUndoStack;
        };

        // Only the AP scan-folder transport boundary is supplied; document JSON I/O and Canvas remain real.
        class CanvasHistorySourceFolder final : public AzToolsFramework::AssetSystemRequestBus::Handler
        {
        public:
            explicit CanvasHistorySourceFolder(AZStd::string path) : m_path(AZStd::move(path)) { BusConnect(); }
            ~CanvasHistorySourceFolder() override { BusDisconnect(); }
            bool GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string&, AZStd::string&) override { return false; }
            bool GenerateRelativeSourcePath(const AZStd::string&, AZStd::string&, AZStd::string&) override { return false; }
            bool GetFullSourcePathFromRelativeProductPath(const AZStd::string&, AZStd::string&) override { return false; }
            bool GetAssetInfoById(const AZ::Data::AssetId&, const AZ::Data::AssetType&, const AZStd::string&, AZ::Data::AssetInfo&, AZStd::string&) override { return false; }
            bool GetSourceInfoBySourcePath(const char* path, AZ::Data::AssetInfo&, AZStd::string&) override
            {
                ++m_sourceLookupCount;
                m_lastSourceLookup = path;
                return false;
            }
            bool GetSourceInfoBySourceUUID(const AZ::Uuid&, AZ::Data::AssetInfo&, AZStd::string&) override { return false; }
            bool GetScanFolders(AZStd::vector<AZStd::string>& paths) override { paths.push_back(m_path); return true; }
            bool GetAssetSafeFolders(AZStd::vector<AZStd::string>& paths) override { paths.push_back(m_path); return true; }
            bool IsAssetPlatformEnabled(const char*) override { return false; }
            int GetPendingAssetsForPlatform(const char*) override { return 0; }
            bool GetAssetsProducedBySourceUUID(const AZ::Uuid&, AZStd::vector<AZ::Data::AssetInfo>&) override
            {
                ++m_productLookupCount;
                return false;
            }
            size_t m_sourceLookupCount = 0;
            size_t m_productLookupCount = 0;
            AZStd::string m_lastSourceLookup;
            bool ClearFingerprintForAsset(const AZStd::string&) override { return false; }
        private:
            AZStd::string m_path;
        };

        class CanvasHistoryRename final : public AzToolsFramework::UndoSystem::URSequencePoint
        {
        public:
            AZ_CLASS_ALLOCATOR(CanvasHistoryRename, AZ::SystemAllocator);
            explicit CanvasHistoryRename(AZ::Entity& entity) : URSequencePoint("Rename graph test entity"), m_entity(entity) {}
            bool Changed() const override { return true; }
            void Undo() override { m_entity.SetName("before"); }
            void Redo() override { m_entity.SetName("after"); }
        private:
            AZ::Entity& m_entity;
        };

        class ReleaseExtenderNode final : public JE::Editor::Tools::GraphModel::Node
        {
        public:
            AZ_CLASS_ALLOCATOR(ReleaseExtenderNode, AZ::SystemAllocator);
            AZ_RTTI(ReleaseExtenderNode, "{89540F18-02BB-4C85-AA96-B2C92A3ADD29}", JE::Editor::Tools::GraphModel::Node);
            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<ReleaseExtenderNode, JE::Editor::Tools::GraphModel::Node>()->Version(0);
                }
            }
            ReleaseExtenderNode() = default;
            explicit ReleaseExtenderNode(JE::Editor::Tools::GraphModel::GraphPtr graph) : Node(graph)
            {
                RegisterSlots();
                CreateSlotData();
            }
            const char* GetTitle() const override { return "Release extender"; }
        protected:
            void RegisterSlots() override
            {
                const auto type = GetGraphContext()->GetDataType<float>();
                RegisterSlot(AZStd::make_shared<JE::Editor::Tools::GraphModel::SlotDefinition>(
                    JE::Editor::Tools::GraphModel::SlotDirection::Input, JE::Editor::Tools::GraphModel::SlotType::Data,
                    "value", "Value", "", JE::Editor::Tools::GraphModel::DataTypeList{type}, type->GetDefaultValue(),
                    0, 3, "Add value", "Add a real extensible input"));
            }
        };
    }

    class GraphDocumentHistoryTests : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            if (!QApplication::instance())
            {
                m_previousQpa = qgetenv("QT_QPA_PLATFORM");
                qputenv("QT_QPA_PLATFORM", "offscreen");
                m_qtApplication = AZStd::make_unique<QApplication>(m_argc, m_argv);
            }
            ASSERT_NE(qobject_cast<QApplication*>(QCoreApplication::instance()), nullptr);
            const AZStd::string engineRoot = AZ::Test::GetEngineRootPath();
            ASSERT_FALSE(engineRoot.empty());
            m_directory = AZStd::make_unique<AZ::Test::ScopedAutoTempDirectory>();
            m_previousFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            m_fileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
            const auto sourceRoot = m_directory->Resolve("source").Native();
            const auto productsRoot = m_directory->Resolve("products").Native();
            ASSERT_TRUE(m_fileIO->CreatePath(sourceRoot.c_str()));
            ASSERT_TRUE(m_fileIO->CreatePath(productsRoot.c_str()));
            m_previousRegistry = AZ::SettingsRegistry::Get();
            if (m_previousRegistry)
            {
                AZ::SettingsRegistry::Unregister(m_previousRegistry);
            }
            m_application = AZStd::make_unique<CanvasHistoryApplication>();
            auto* registry = AZ::SettingsRegistry::Get();
            ASSERT_NE(registry, nullptr);
            for (const char* key : {"/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen",
                     "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit", "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave"})
            {
                ASSERT_TRUE(registry->Set(key, false));
                bool enabled = true;
                ASSERT_TRUE(registry->Get(enabled, key));
                ASSERT_FALSE(enabled);
            }
            AZ::ComponentApplication::Descriptor descriptor;
            descriptor.m_useExistingAllocator = true;
            AZ::ComponentApplication::StartupParameters startup;
            startup.m_loadDynamicModules = false;
            startup.m_loadStaticModules = true;
            startup.m_loadSettingsRegistry = false;
            startup.m_createEditContext = true;
            AZ::Entity* system = m_application->AZ::ComponentApplication::Create(descriptor, startup);
            ASSERT_NE(system, nullptr);
            // Match the native GraphModel fixture without activating unrelated EditorTools services.
            m_application->RegisterComponentDescriptor(
                JE::Editor::Tools::GraphModel::GraphModelSystemComponent::CreateDescriptor());
            ASSERT_NE(system->CreateComponent<JE::Editor::Tools::GraphModel::GraphModelSystemComponent>(), nullptr);
            m_application->InitializeUndo();
            ASSERT_NE(system->FindComponent<AZ::AssetManagerComponent>(), nullptr);
            ASSERT_NE(system->FindComponent<AZ::JobManagerComponent>(), nullptr);
            ASSERT_NE(system->FindComponent<AZ::TaskGraphSystemComponent>(), nullptr);
            ASSERT_NE(system->FindComponent<AZ::StreamerComponent>(), nullptr);

            // Keep style-enum Qt metatype interfaces in this test DLL across production module reloads.
            qRegisterMetaType<QFont::Weight>();
            qRegisterMetaType<QFont::Style>();
            qRegisterMetaType<QFont::Capitalization>();
            qRegisterMetaType<Qt::PenStyle>();
            qRegisterMetaType<Qt::PenCapStyle>();
            qRegisterMetaType<Qt::Orientation>();
            qRegisterMetaType<Qt::AlignmentFlag>();
            qRegisterMetaType<JE::Editor::Tools::GraphCanvas::Styling::ConnectionCurveType>();
            qRegisterMetaType<JE::Editor::Tools::GraphCanvas::Styling::PaletteStyle>();

            // Register only Canvas descriptors; unrelated EditorTools services remain absent.
            AZStd::list<AZ::ComponentDescriptor*> graphCanvasDescriptors;
            JE::Editor::Tools::GraphCanvas::AppendGraphCanvasDescriptors(graphCanvasDescriptors);
            ASSERT_EQ(graphCanvasDescriptors.size(), 50);
            for (AZ::ComponentDescriptor* componentDescriptor : graphCanvasDescriptors)
            {
                m_application->RegisterComponentDescriptor(componentDescriptor);
            }
            ASSERT_NE(system->CreateComponent<JE::Editor::Tools::GraphCanvas::GraphCanvasSystemComponent>(), nullptr);
            system->Init();
            system->Activate();
            ASSERT_EQ(system->GetState(), AZ::Entity::State::Active);
            // The document job must use this fixture's activated core manager, not an absent/global foreign service.
            auto* jobs = system->FindComponent<AZ::JobManagerComponent>();
            ASSERT_NE(jobs, nullptr);
            ASSERT_NE(jobs->GetManager(), nullptr);
            ASSERT_NE(jobs->GetGlobalContext(), nullptr);
            ASSERT_EQ(AZ::JobContext::GetGlobalContext(), jobs->GetGlobalContext());
            ASSERT_EQ(&AZ::JobContext::GetGlobalContext()->GetJobManager(), jobs->GetManager());
            ASSERT_TRUE(JE::Editor::Tools::GraphCanvas::GraphCanvasRequestBus::HasHandlers());
            ASSERT_TRUE(JE::Editor::Tools::GraphModel::Integration::GraphManagerRequestBus::HasHandlers());

            // Copy the production stylesheet into a private products root; no project cache or authored assets are touched.
            const auto style = AZ::Utils::ReadFile(engineRoot + "/Editor/Materials/Assets/MaterialCanvas/StyleSheet/materialcanvas_style.json");
            ASSERT_TRUE(style.IsSuccess());
            ASSERT_TRUE(AZ::Utils::WriteFile(style.GetValue(), m_directory->Resolve("products/canvas_style.json").Native()).IsSuccess());
            // Seed private products before enabling the real cache-write protection on their alias.
            m_fileIO->SetAlias("@products@", productsRoot.c_str());
            m_styleManager = AZStd::make_unique<JE::Editor::Tools::GraphCanvas::StyleManager>(CanvasHistoryToolId, "canvas_style.json");
            const JE::Editor::Tools::GraphModel::DataTypeList types{
                AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(1, 0.0f, "Float"),
                AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(2, AZStd::string{}, "String")};
            m_graphContext = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphContext>("GraphDocumentHistory", ".materialgraph", types);
            m_policy = JE::Editor::CreateMaterialCanvasDocumentHistoryPolicy();
            m_sourceFolder = AZStd::make_unique<CanvasHistorySourceFolder>(sourceRoot);
            m_path = m_directory->Resolve("source/history.materialgraph").Native();
            JE::Editor::Tools::GraphModel::Graph initial(m_graphContext);
            ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, m_path).IsSuccess());
            const auto type = JE::Editor::Tools::GraphDocument::BuildDocumentTypeInfo(
                "Material Graph", {"materialgraph"}, {}, {}, m_graphContext, {});
            m_document = AZStd::make_unique<JE::Editor::Tools::GraphDocument>(CanvasHistoryToolId, type,
                m_graphContext, AZStd::make_shared<JE::Editor::Tools::GraphCompiler>(CanvasHistoryToolId));
            ASSERT_TRUE(m_document->SetHistoryPolicy(m_policy));
            ASSERT_TRUE(m_document->Open(m_path));
            m_graphId = m_document->GetGraphId();
            ASSERT_TRUE(m_graphId.IsValid());
            ASSERT_TRUE(JE::Editor::Tools::GraphCanvas::SceneRequestBus::HasHandlers(m_graphId));
            ASSERT_TRUE(JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::HasHandlers(m_graphId));
            // Compilation is disabled before construction; no SystemTick or AP connection is needed for replay.
        }

        void TearDown() override
        {
            m_document.reset();
            m_graphContext.reset();
            m_policy = {};
            m_sourceFolder.reset();
            m_styleManager.reset();
            if (m_application)
            {
                m_application->FlushUndo();
                // Destroy deactivates components and releases registered descriptors.
                m_application->Destroy();
                m_application.reset();
            }
            if (m_previousRegistry)
            {
                AZ::SettingsRegistry::Register(m_previousRegistry);
            }
            if (m_fileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
                m_fileIO.reset();
                AZ::IO::FileIOBase::SetInstance(m_previousFileIO);
            }
            m_directory.reset();
            // Workspace views, documents, session and GraphCanvas owners have already been released.
            m_qtStyleManager.reset();
            if (m_qtApplication)
            {
                m_qtApplication.reset();
                if (m_previousQpa.isNull())
                {
                    qunsetenv("QT_QPA_PLATFORM");
                }
                else
                {
                    qputenv("QT_QPA_PLATFORM", m_previousQpa);
                }
            }
            LeakDetectionFixture::TearDown();
        }

        void AddNodes()
        {
            ASSERT_TRUE(m_document->BeginEdit());
            auto graph = m_document->GetGraph();
            auto input = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphInputNode>(graph, m_graphContext->GetDataType<float>());
            auto output = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphOutputNode>(graph, m_graphContext->GetDataType<float>());
            input->GetSlot("name")->SetValue(AZStd::string("input"));
            output->GetSlot("name")->SetValue(AZStd::string("output"));
            input->GetSlot("defaultValue")->SetValue(2.0f);
            AZ::Vector2 inputPosition(24.0f, 48.0f);
            AZ::Vector2 outputPosition(480.0f, 96.0f);
            JE::Editor::Tools::GraphCanvas::NodeId inputCanvasId, outputCanvasId;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(inputCanvasId, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, input, inputPosition);
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(outputCanvasId, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, output, outputPosition);
            ASSERT_TRUE(inputCanvasId.IsValid());
            ASSERT_TRUE(outputCanvasId.IsValid());
            m_inputId = input->GetId();
            m_outputId = output->GetId();
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(m_inputPosition, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetPosition, input);
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(m_outputPosition, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetPosition, output);
            ASSERT_TRUE(m_document->EndEdit());
        }

        enum class ReleaseControl { Normal, ChangedPosition, Cancel };

        void CheckConnectionReleaseGesture(bool moveAfterStart, ReleaseControl control = ReleaseControl::Normal)
        {
            ASSERT_NO_FATAL_FAILURE(AddNodes());
            m_application->FlushUndo();
            JE::Editor::Tools::GraphCanvas::GraphCanvasGraphicsView view(nullptr, false);
            view.resize(1000, 650);
            view.SetEditorId(CanvasHistoryToolId);
            view.SetScene(m_graphId);
            view.show();
            const auto graph = m_document->GetGraph();
            const auto source = graph->GetNode(m_inputId)->GetSlot("value");
            const auto target = graph->GetNode(m_outputId)->GetSlot("value");
            JE::Editor::Tools::GraphCanvas::SlotId sourceSlot, targetSlot;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, source);
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(targetSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, target);
            ASSERT_TRUE(sourceSlot.IsValid());
            ASSERT_TRUE(targetSlot.IsValid());
            JE::Editor::Tools::GraphCanvas::Endpoint sourceEndpoint;
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(sourceEndpoint, sourceSlot, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);
            ASSERT_TRUE(sourceEndpoint.IsValid());
            bool canCreateConnection = false;
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(canCreateConnection, targetSlot,
                &JE::Editor::Tools::GraphCanvas::SlotRequests::CanCreateConnectionTo, sourceEndpoint);
            RecordProperty("release_can_create_before_gesture", static_cast<int>(canCreateConnection));
            RecordProperty("release_source_slot", sourceSlot.ToString().c_str());
            RecordProperty("release_target_slot", targetSlot.ToString().c_str());
            bool canDisplayConnection = false;
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(canDisplayConnection, targetSlot,
                &JE::Editor::Tools::GraphCanvas::SlotRequests::CanDisplayConnectionTo, sourceEndpoint);
            JE::Editor::Tools::GraphCanvas::GraphId targetScene;
            JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(targetScene, targetSlot, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);
            JE::Editor::Tools::GraphCanvas::Endpoint targetEndpoint;
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(targetEndpoint, targetSlot, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);
            bool modelConnectionValid = false;
            if (canDisplayConnection)
            {
                modelConnectionValid = JE::Editor::Tools::GraphCanvas::GraphUtils::IsValidModelConnection(targetScene, sourceEndpoint, targetEndpoint);
            }
            RecordProperty("release_can_display_before_gesture", static_cast<int>(canDisplayConnection));
            RecordProperty("release_target_scene", targetScene.ToString().c_str());
            RecordProperty("release_document_scene", m_graphId.ToString().c_str());
            RecordProperty("release_model_handler_present", static_cast<int>(JE::Editor::Tools::GraphCanvas::GraphModelRequestBus::HasHandlers(targetScene)));
            RecordProperty("release_model_check_evaluated", static_cast<int>(canDisplayConnection));
            RecordProperty("release_model_connection_valid", canDisplayConnection ? static_cast<int>(modelConnectionValid) : -1);
            bool directModelValid = false;
            JE::Editor::Tools::GraphCanvas::GraphModelRequestBus::EventResult(directModelValid, targetScene,
                &JE::Editor::Tools::GraphCanvas::GraphModelRequests::IsValidConnection, sourceEndpoint, targetEndpoint);
            RecordProperty("release_direct_model_valid", static_cast<int>(directModelValid));
            const auto remappedSources = JE::Editor::Tools::GraphCanvas::GraphUtils::RemapEndpointForModel(sourceEndpoint);
            const auto remappedTargets = JE::Editor::Tools::GraphCanvas::GraphUtils::RemapEndpointForModel(targetEndpoint);
            RecordProperty("release_remapped_source_count", static_cast<int>(remappedSources.size()));
            RecordProperty("release_remapped_target_count", static_cast<int>(remappedTargets.size()));
            int remappedIndex = 0;
            for (const auto& endpoint : remappedSources)
            {
                RecordProperty(QString("release_remapped_source_%1").arg(remappedIndex++).toStdString(),
                    (endpoint.GetNodeId().ToString() + "/" + endpoint.GetSlotId().ToString()).c_str());
            }
            remappedIndex = 0;
            for (const auto& endpoint : remappedTargets)
            {
                RecordProperty(QString("release_remapped_target_%1").arg(remappedIndex++).toStdString(),
                    (endpoint.GetNodeId().ToString() + "/" + endpoint.GetSlotId().ToString()).c_str());
            }
            const auto sourceDataType = source->GetDataType();
            const auto targetDataType = target->GetDataType();
            RecordProperty("release_source_datatype", sourceDataType ? sourceDataType->GetTypeUuidString().c_str() : "null");
            RecordProperty("release_target_datatype", targetDataType ? targetDataType->GetTypeUuidString().c_str() : "null");
            RecordProperty("release_datatype_pointer_equal", static_cast<int>(sourceDataType == targetDataType));
            RecordProperty("release_target_supports_source_type", static_cast<int>(target->IsSupportedDataType(sourceDataType)));
            const auto sourceParent = source->GetParentNode();
            const auto targetParent = target->GetParentNode();
            RecordProperty("release_source_parent_valid", static_cast<int>(sourceParent != nullptr));
            RecordProperty("release_target_parent_valid", static_cast<int>(targetParent != nullptr));
            RecordProperty("release_parents_equal", static_cast<int>(sourceParent == targetParent));
            RecordProperty("release_model_connection_count", static_cast<int>(graph->GetConnectionCount()));
            auto recordRoundTrip = [&](const char* label, const JE::Editor::Tools::GraphCanvas::Endpoint& endpoint, const auto& slot)
            {
                JE::Editor::Tools::GraphModel::NodePtr mappedNode;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(mappedNode, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeById, endpoint.GetNodeId());
                const auto mappedSlot = mappedNode ? mappedNode->GetSlot(slot->GetSlotId()) : JE::Editor::Tools::GraphModel::SlotPtr{};
                JE::Editor::Tools::GraphCanvas::SlotId roundTrip;
                if (mappedSlot)
                {
                    JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(roundTrip, m_graphId,
                        &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, mappedSlot);
                }
                RecordProperty(QString("release_%1_node_map_matches").arg(label).toStdString(),
                    static_cast<int>(mappedNode != nullptr && mappedNode == slot->GetParentNode()));
                RecordProperty(QString("release_%1_slot_map_matches").arg(label).toStdString(), static_cast<int>(mappedSlot == slot));
                RecordProperty(QString("release_%1_slot_id_roundtrip").arg(label).toStdString(),
                    static_cast<int>(roundTrip.IsValid() && roundTrip == endpoint.GetSlotId()));
            };
            recordRoundTrip("source", sourceEndpoint, source);
            recordRoundTrip("target", targetEndpoint, target);
            ASSERT_TRUE(canCreateConnection) << "Native target slot rejected the source before the gesture.";
            for (const auto& node : {source->GetParentNode(), target->GetParentNode()})
            {
                JE::Editor::Tools::GraphCanvas::NodeId canvasNode;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasNode, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeIdByNode, node);
                QGraphicsLayout* nodeLayout = nullptr;
                JE::Editor::Tools::GraphCanvas::NodeLayoutRequestBus::EventResult(nodeLayout, canvasNode, &JE::Editor::Tools::GraphCanvas::NodeLayoutRequests::GetLayout);
                ASSERT_NE(nodeLayout, nullptr);
                nodeLayout->activate();
                JE::Editor::Tools::GraphCanvas::NodeUIRequestBus::Event(canvasNode, &JE::Editor::Tools::GraphCanvas::NodeUIRequests::AdjustSize);
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            view.centerOn(350.0, 150.0);
            QPointF sourceScenePoint, targetScenePoint;
            JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(sourceScenePoint, sourceSlot, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetConnectionPoint);
            JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(targetScenePoint, targetSlot, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetConnectionPoint);
            const QPoint sourcePoint = view.mapFromScene(sourceScenePoint);
            const QPoint targetPoint = view.mapFromScene(targetScenePoint);
            ASSERT_NE(sourcePoint, targetPoint);
            ASSERT_TRUE(view.viewport()->rect().contains(sourcePoint));
            ASSERT_TRUE(view.viewport()->rect().contains(targetPoint));
            ASSERT_NE(view.scene(), nullptr);
            ASSERT_FALSE(JE::Editor::Tools::GraphCanvas::AssetEditorRequestBus::HasHandlers(CanvasHistoryToolId));
            auto sendMouse = [&](QEvent::Type type, const QPoint& point, Qt::MouseButton button, Qt::MouseButtons buttons)
            {
                QMouseEvent event(type, QPointF(point), QPointF(view.viewport()->mapToGlobal(point)), button, buttons, Qt::NoModifier);
                QApplication::sendEvent(view.viewport(), &event);
            };
            auto recordPoint = [&](const char* name, const QPointF& point)
            {
                RecordProperty(name, QString("%1,%2").arg(point.x(), 0, 'g', 17).arg(point.y(), 0, 'g', 17).toStdString());
            };
            RecordProperty("release_control_has_extra_move", static_cast<int>(moveAfterStart));
            recordPoint("release_source_scene_before_grab", sourceScenePoint);
            recordPoint("release_target_scene_before_grab", targetScenePoint);
            recordPoint("release_target_pixel_before_grab", targetPoint);
            recordPoint("release_target_pixel_mapped_before_grab", view.mapToScene(targetPoint));
            sendMouse(QEvent::MouseButtonPress, sourcePoint, Qt::LeftButton, Qt::LeftButton);
            auto* pressedGrabber = view.scene()->mouseGrabberItem();
            ASSERT_NE(pressedGrabber, nullptr);
            const QPoint initiatingPoint = moveAfterStart
                ? QPoint((sourcePoint.x() + targetPoint.x()) / 2, (sourcePoint.y() + targetPoint.y()) / 2)
                : targetPoint;
            sendMouse(QEvent::MouseMove, initiatingPoint, Qt::NoButton, Qt::LeftButton);
            ASSERT_NE(view.scene()->mouseGrabberItem(), nullptr);
            ASSERT_NE(view.scene()->mouseGrabberItem(), pressedGrabber) << "The initiating move did not transfer the grab to the connection.";
            QPointF targetAfterGrab;
            JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(targetAfterGrab, targetSlot, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetConnectionPoint);
            recordPoint("release_target_scene_after_grab", targetAfterGrab);
            recordPoint("release_target_pixel_mapped_after_grab", view.mapToScene(targetPoint));
            ASSERT_EQ(graph->GetConnectionCount(), 0);
            if (moveAfterStart)
            {
                const QPoint evaluationPoint = control == ReleaseControl::ChangedPosition ? initiatingPoint : targetPoint;
                if (control == ReleaseControl::ChangedPosition)
                {
                    const QPointF scenePoint = view.mapToScene(evaluationPoint);
                    double snapDistance = 10.0;
                    JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::EventResult(snapDistance, CanvasHistoryToolId,
                        &JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequests::GetSnapDistance);
                    AZStd::vector<JE::Editor::Tools::GraphCanvas::Endpoint> endpoints;
                    JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(endpoints, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetEndpointsInRect,
                        QRectF(scenePoint - QPointF(snapDistance, snapDistance), scenePoint + QPointF(snapDistance, snapDistance)));
                    ASSERT_TRUE(endpoints.empty()) << "The changed-release control requires an empty last-move candidate area.";
                    ASSERT_NE(evaluationPoint, targetPoint);
                }
                sendMouse(QEvent::MouseMove, evaluationPoint, Qt::NoButton, Qt::LeftButton);
            }
            QPointF targetBeforeRelease;
            JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(targetBeforeRelease, targetSlot, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetConnectionPoint);
            recordPoint("release_target_scene_before_release", targetBeforeRelease);
            recordPoint("release_target_pixel_mapped_before_release", view.mapToScene(targetPoint));
            if (control == ReleaseControl::Cancel)
            {
                AZStd::vector<AZ::EntityId> connections;
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(connections, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetConnections);
                ASSERT_EQ(connections.size(), 1);
                JE::Editor::Tools::GraphCanvas::Endpoint proposedTarget;
                JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::EventResult(proposedTarget, connections.front(),
                    &JE::Editor::Tools::GraphCanvas::ConnectionRequests::GetTargetEndpoint);
                ASSERT_EQ(proposedTarget.GetSlotId(), targetSlot) << "Escape must cancel an actual accepted proposal.";
                JE::Editor::Tools::GraphCanvas::ViewNotificationBus::Event(view.GetViewId(), &JE::Editor::Tools::GraphCanvas::ViewNotifications::OnEscape);
                sendMouse(QEvent::MouseButtonRelease, targetPoint, Qt::LeftButton, Qt::NoButton);
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(connections, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetConnections);
                EXPECT_TRUE(connections.empty());
                ASSERT_NO_FATAL_FAILURE(CheckGraph(2.0f, false));
                EXPECT_FALSE(m_application->GetUndoStack()->CanUndo());
                return;
            }
            // The first move starts the connection at the pin; only the control sends a move to the new connection filter.
            sendMouse(QEvent::MouseButtonRelease, targetPoint, Qt::LeftButton, Qt::NoButton);
            ASSERT_EQ(graph->GetConnectionCount(), 1) << "Release must evaluate its actual position even without a post-start move.";
            ASSERT_NO_FATAL_FAILURE(CheckGraph(2.0f, true));
            ASSERT_TRUE(m_document->Undo());
            ASSERT_NO_FATAL_FAILURE(CheckGraph(2.0f, false));
            ASSERT_TRUE(m_document->Redo());
            ASSERT_NO_FATAL_FAILURE(CheckGraph(2.0f, true));
        }

        void CheckExtenderConnectionReleaseGesture(bool changedReleasePoint, bool checkOwnership = false)
        {
            ReleaseExtenderNode::Reflect(m_application->GetSerializeContext());
            ASSERT_TRUE(m_document->BeginEdit());
            const auto graph = m_document->GetGraph();
            const auto source = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphInputNode>(graph, m_graphContext->GetDataType<float>());
            const auto target = AZStd::make_shared<ReleaseExtenderNode>(graph);
            source->GetSlot("defaultValue")->SetValue(2.0f);
            JE::Editor::Tools::GraphCanvas::NodeId sourceCanvasNode, targetCanvasNode;
            AZ::Vector2 sourcePosition(24.0f, 48.0f), targetPosition(480.0f, 96.0f);
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceCanvasNode, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, source, sourcePosition);
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(targetCanvasNode, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, target, targetPosition);
            ASSERT_TRUE(sourceCanvasNode.IsValid());
            ASSERT_TRUE(targetCanvasNode.IsValid());
            ASSERT_TRUE(m_document->EndEdit());
            m_application->FlushUndo();
            ASSERT_TRUE(target->GetExtendableSlots("value").empty());
            JE::Editor::Tools::GraphCanvas::GraphCanvasGraphicsView view(nullptr, false);
            view.resize(1000, 650);
            view.SetEditorId(CanvasHistoryToolId);
            view.SetScene(m_graphId);
            view.show();
            for (const auto& canvasNode : {sourceCanvasNode, targetCanvasNode})
            {
                QGraphicsLayout* layout = nullptr;
                JE::Editor::Tools::GraphCanvas::NodeLayoutRequestBus::EventResult(layout, canvasNode, &JE::Editor::Tools::GraphCanvas::NodeLayoutRequests::GetLayout);
                ASSERT_NE(layout, nullptr);
                layout->activate();
                JE::Editor::Tools::GraphCanvas::NodeUIRequestBus::Event(canvasNode, &JE::Editor::Tools::GraphCanvas::NodeUIRequests::AdjustSize);
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            view.centerOn(350.0, 150.0);
            JE::Editor::Tools::GraphCanvas::SlotId sourceSlot, extenderSlot;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, source->GetSlot("value"));
            AZStd::vector<AZ::EntityId> targetSlots;
            JE::Editor::Tools::GraphCanvas::NodeRequestBus::EventResult(targetSlots, targetCanvasNode, &JE::Editor::Tools::GraphCanvas::NodeRequests::GetSlotIds);
            for (const auto& slot : targetSlots)
            {
                JE::Editor::Tools::GraphCanvas::SlotType type = JE::Editor::Tools::GraphCanvas::SlotTypes::Invalid;
                JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(type, slot, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetSlotType);
                if (type == JE::Editor::Tools::GraphCanvas::SlotTypes::ExtenderSlot)
                {
                    ASSERT_FALSE(extenderSlot.IsValid());
                    extenderSlot = slot;
                }
            }
            ASSERT_TRUE(sourceSlot.IsValid());
            ASSERT_TRUE(extenderSlot.IsValid());
            QPointF sourceScenePoint, extenderScenePoint;
            JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(sourceScenePoint, sourceSlot, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetConnectionPoint);
            JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(extenderScenePoint, extenderSlot, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetConnectionPoint);
            const QPoint sourcePoint = view.mapFromScene(sourceScenePoint);
            const QPoint extenderPoint = view.mapFromScene(extenderScenePoint);
            ASSERT_TRUE(view.viewport()->rect().contains(sourcePoint));
            ASSERT_TRUE(view.viewport()->rect().contains(extenderPoint));
            ASSERT_NE(sourcePoint, extenderPoint);
            ASSERT_FALSE(JE::Editor::Tools::GraphCanvas::AssetEditorRequestBus::HasHandlers(CanvasHistoryToolId));
            auto sendMouse = [&](QEvent::Type type, const QPoint& point, Qt::MouseButton button, Qt::MouseButtons buttons)
            {
                QMouseEvent event(type, QPointF(point), QPointF(view.viewport()->mapToGlobal(point)), button, buttons, Qt::NoModifier);
                QApplication::sendEvent(view.viewport(), &event);
            };
            sendMouse(QEvent::MouseButtonPress, sourcePoint, Qt::LeftButton, Qt::LeftButton);
            auto* pressedGrabber = view.scene()->mouseGrabberItem();
            ASSERT_NE(pressedGrabber, nullptr);
            sendMouse(QEvent::MouseMove, (sourcePoint + extenderPoint) / 2, Qt::NoButton, Qt::LeftButton);
            ASSERT_NE(view.scene()->mouseGrabberItem(), nullptr);
            ASSERT_NE(view.scene()->mouseGrabberItem(), pressedGrabber);
            sendMouse(QEvent::MouseMove, extenderPoint, Qt::NoButton, Qt::LeftButton);
            ASSERT_EQ(target->GetExtendableSlots("value").size(), 1);
            ASSERT_EQ(graph->GetConnectionCount(), 0);
            AZStd::vector<AZ::EntityId> proposals;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(proposals, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetConnections);
            ASSERT_EQ(proposals.size(), 1);
            JE::Editor::Tools::GraphCanvas::SlotId createdSlot;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(createdSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot,
                *target->GetExtendableSlots("value").begin());
            JE::Editor::Tools::GraphCanvas::Endpoint proposedTarget;
            JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::EventResult(proposedTarget, proposals.front(), &JE::Editor::Tools::GraphCanvas::ConnectionRequests::GetTargetEndpoint);
            ASSERT_EQ(proposedTarget.GetSlotId(), createdSlot);
            if (checkOwnership)
            {
                JE::Editor::Tools::GraphCanvas::ScopedGraphUndoBlocker undoBlocker(m_graphId);
                const auto ownerId = proposals.front();
                auto* ownerGrabber = view.scene()->mouseGrabberItem();
                ASSERT_NE(ownerGrabber, nullptr);
                JE::Editor::Tools::GraphCanvas::Endpoint sourceEndpoint;
                JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(sourceEndpoint, sourceSlot, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);
                ASSERT_TRUE(sourceEndpoint.IsValid());
                auto checkOwner = [&]()
                {
                    ASSERT_TRUE(JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::HasHandlers(ownerId));
                    JE::Editor::Tools::GraphCanvas::GraphId ownerScene;
                    JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(ownerScene, ownerId, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);
                    EXPECT_EQ(ownerScene, m_graphId);
                    JE::Editor::Tools::GraphCanvas::Endpoint actualSource, actualTarget;
                    JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::EventResult(actualSource, ownerId, &JE::Editor::Tools::GraphCanvas::ConnectionRequests::GetSourceEndpoint);
                    JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::EventResult(actualTarget, ownerId, &JE::Editor::Tools::GraphCanvas::ConnectionRequests::GetTargetEndpoint);
                    EXPECT_EQ(actualSource, sourceEndpoint);
                    EXPECT_EQ(actualTarget, proposedTarget);
                    EXPECT_EQ(target->GetExtendableSlots("value").size(), 1);
                    EXPECT_EQ(graph->GetConnectionCount(), 0);
                };
                JE::Editor::Tools::GraphCanvas::Endpoint reused;
                JE::Editor::Tools::GraphCanvas::ExtenderSlotRequestBus::EventResult(reused, extenderSlot,
                    &JE::Editor::Tools::GraphCanvas::ExtenderSlotRequests::ExtendForConnectionProposal, ownerId, sourceEndpoint);
                EXPECT_EQ(reused, proposedTarget);
                JE::Editor::Tools::GraphCanvas::ConnectionId foreignId;
                JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(foreignId, sourceSlot,
                    &JE::Editor::Tools::GraphCanvas::SlotRequests::DisplayConnectionWithEndpoint, proposedTarget);
                ASSERT_TRUE(foreignId.IsValid());
                ASSERT_NE(foreignId, ownerId);
                ASSERT_TRUE(JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::HasHandlers(foreignId));
                JE::Editor::Tools::GraphCanvas::GraphId foreignScene;
                JE::Editor::Tools::GraphCanvas::SceneMemberRequestBus::EventResult(foreignScene, foreignId, &JE::Editor::Tools::GraphCanvas::SceneMemberRequests::GetScene);
                ASSERT_EQ(foreignScene, m_graphId);
                ASSERT_NO_FATAL_FAILURE(checkOwner());
                JE::Editor::Tools::GraphCanvas::Endpoint rejected;
                JE::Editor::Tools::GraphCanvas::ExtenderSlotRequestBus::EventResult(rejected, extenderSlot,
                    &JE::Editor::Tools::GraphCanvas::ExtenderSlotRequests::ExtendForConnectionProposal, foreignId, sourceEndpoint);
                EXPECT_FALSE(rejected.IsValid());
                ASSERT_NO_FATAL_FAILURE(checkOwner());
                const AZStd::unordered_set<AZ::EntityId> foreignOnly{foreignId};
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::Event(m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::Delete, foreignOnly);
                EXPECT_FALSE(JE::Editor::Tools::GraphCanvas::ConnectionRequestBus::HasHandlers(foreignId));
                ASSERT_NO_FATAL_FAILURE(checkOwner());
                AZStd::vector<AZ::EntityId> remaining;
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(remaining, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetConnections);
                ASSERT_EQ(remaining.size(), 1);
                ASSERT_EQ(remaining.front(), ownerId);
                ASSERT_EQ(view.scene()->mouseGrabberItem(), ownerGrabber) << "Deleting the foreign connection must restore the owner grab.";
                JE::Editor::Tools::GraphCanvas::ExtenderSlotRequestBus::EventResult(reused, extenderSlot,
                    &JE::Editor::Tools::GraphCanvas::ExtenderSlotRequests::ExtendForConnectionProposal, ownerId, sourceEndpoint);
                EXPECT_EQ(reused, proposedTarget);
                ASSERT_NO_FATAL_FAILURE(checkOwner());
            }
            QPoint releasePoint = extenderPoint;
            if (changedReleasePoint)
            {
                JE::Editor::Tools::GraphCanvas::SlotUIRequestBus::EventResult(extenderScenePoint, extenderSlot, &JE::Editor::Tools::GraphCanvas::SlotUIRequests::GetConnectionPoint);
                releasePoint = view.mapFromScene(extenderScenePoint) + QPoint(1, 0);
                ASSERT_NE(releasePoint, extenderPoint);
                ASSERT_TRUE(view.viewport()->rect().contains(releasePoint));
                ASSERT_TRUE(JE::Editor::Tools::GraphCanvas::GraphUtils::IsSlotVisible(extenderSlot));
                double snapDistance = 10.0;
                JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequestBus::EventResult(snapDistance, CanvasHistoryToolId,
                    &JE::Editor::Tools::GraphCanvas::AssetEditorSettingsRequests::GetSnapDistance);
                const auto releaseScenePoint = view.mapToScene(releasePoint);
                AZStd::vector<JE::Editor::Tools::GraphCanvas::Endpoint> endpoints;
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(endpoints, m_graphId, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetEndpointsInRect,
                    QRectF(releaseScenePoint - QPointF(snapDistance, snapDistance), releaseScenePoint + QPointF(snapDistance, snapDistance)));
                ASSERT_FALSE(endpoints.empty());
                ASSERT_EQ(endpoints.front().GetSlotId(), extenderSlot) << "Changed release must still hit the SAME real extender first.";
            }
            RecordProperty("extender_changed_release_point", static_cast<int>(changedReleasePoint));
            sendMouse(QEvent::MouseButtonRelease, releasePoint, Qt::LeftButton, Qt::NoButton);
            const auto sourceId = source->GetId();
            const auto targetId = target->GetId();
            auto checkState = [&](size_t slotCount, size_t connectionCount)
            {
                const auto current = m_document->GetGraph();
                const auto currentSource = current->GetNode(sourceId);
                const auto currentTarget = current->GetNode(targetId);
                ASSERT_EQ(current->GetNodeCount(), 2);
                ASSERT_NE(currentSource, nullptr);
                ASSERT_NE(currentTarget, nullptr);
                EXPECT_FLOAT_EQ(currentSource->GetSlot("defaultValue")->GetValue<float>(), 2.0f);
                ASSERT_EQ(currentTarget->GetExtendableSlots("value").size(), slotCount);
                ASSERT_EQ(current->GetConnectionCount(), connectionCount);
                if (connectionCount)
                {
                    const auto connection = current->GetConnections().front();
                    EXPECT_EQ(connection->GetSourceSlot(), currentSource->GetSlot("value"));
                    EXPECT_EQ(connection->GetTargetSlot(), *currentTarget->GetExtendableSlots("value").begin());
                }
            };
            ASSERT_NO_FATAL_FAILURE(checkState(1, 1));
            ASSERT_TRUE(m_document->Undo());
            ASSERT_NO_FATAL_FAILURE(checkState(0, 0));
            ASSERT_TRUE(m_document->Redo());
            ASSERT_NO_FATAL_FAILURE(checkState(1, 1));
        }

        void CheckGraph(float value, bool connected)
        {
            ASSERT_EQ(m_document->GetGraphId(), m_graphId);
            EXPECT_FALSE(m_document->IsCompileGraphQueued());
            auto graph = m_document->GetGraph();
            ASSERT_NE(graph, nullptr);
            ASSERT_EQ(graph->GetNodeCount(), 2);
            auto input = graph->GetNode(m_inputId);
            auto output = graph->GetNode(m_outputId);
            ASSERT_NE(input, nullptr);
            ASSERT_NE(output, nullptr);
            EXPECT_EQ(input->GetGraph(), graph);
            EXPECT_EQ(output->GetGraph(), graph);
            ASSERT_NE(input->GetSlot("defaultValue"), nullptr);
            EXPECT_FLOAT_EQ(input->GetSlot("defaultValue")->GetValue<float>(), value);
            auto source = input->GetSlot("value");
            auto target = output->GetSlot("value");
            ASSERT_NE(source, nullptr);
            ASSERT_NE(target, nullptr);
            EXPECT_EQ(source->GetGraph(), graph);
            EXPECT_EQ(source->GetParentNode(), input);
            EXPECT_EQ(target->GetParentNode(), output);
            ASSERT_EQ(graph->GetConnectionCount(), connected ? 1 : 0);
            if (connected)
            {
                const auto connection = graph->GetConnections().front();
                EXPECT_EQ(connection->GetGraph(), graph);
                EXPECT_EQ(connection->GetSourceNode(), input);
                EXPECT_EQ(connection->GetTargetNode(), output);
                EXPECT_EQ(connection->GetSourceSlot(), source);
                EXPECT_EQ(connection->GetTargetSlot(), target);
                EXPECT_EQ(connection->GetSourceEndpoint(), JE::Editor::Tools::GraphModel::Endpoint(m_inputId, source->GetSlotId()));
                EXPECT_EQ(connection->GetTargetEndpoint(), JE::Editor::Tools::GraphModel::Endpoint(m_outputId, target->GetSlotId()));
            }
            for (auto node : {input, output})
            {
                JE::Editor::Tools::GraphCanvas::NodeId canvasId;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasId, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeIdByNode, node);
                ASSERT_TRUE(canvasId.IsValid());
                JE::Editor::Tools::GraphModel::NodePtr mapped;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(mapped, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeById, canvasId);
                EXPECT_EQ(mapped, node);
                JE::Editor::Tools::GraphCanvas::SlotId canvasSlot;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasSlot, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, node->GetSlot("value"));
                EXPECT_TRUE(canvasSlot.IsValid());
                AZ::Vector2 position;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(position, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetPosition, node);
                EXPECT_TRUE(position.IsClose(node == input ? m_inputPosition : m_outputPosition));
            }
        }

        int m_argc = 1;
        char m_program[32] = "GraphDocumentHistoryTests";
        char* m_argv[2] = {m_program, nullptr};
        QByteArray m_previousQpa;
        AZStd::unique_ptr<QApplication> m_qtApplication;
        AZStd::unique_ptr<AzQtComponents::StyleManager> m_qtStyleManager;
        AZStd::unique_ptr<AZ::Test::ScopedAutoTempDirectory> m_directory;
        AZ::IO::FileIOBase* m_previousFileIO = nullptr;
        AZ::SettingsRegistryInterface* m_previousRegistry = nullptr;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_fileIO;
        AZStd::unique_ptr<CanvasHistoryApplication> m_application;
        AZStd::unique_ptr<JE::Editor::Tools::GraphCanvas::StyleManager> m_styleManager;
        JE::Editor::Tools::GraphModel::GraphContextPtr m_graphContext;
        JE::Editor::Tools::DocumentHistoryPolicy m_policy;
        AZStd::unique_ptr<CanvasHistorySourceFolder> m_sourceFolder;
        AZStd::unique_ptr<JE::Editor::Tools::GraphDocument> m_document;
        AZStd::string m_path;
        JE::Editor::Tools::GraphCanvas::GraphId m_graphId;
        JE::Editor::Tools::GraphModel::NodeId m_inputId = 0, m_outputId = 0;
        AZ::Vector2 m_inputPosition, m_outputPosition;
    };

    namespace
    {
        // Only job-status transport is supplied. No AP, shader compile or product success is simulated.
        class GenerationJobStatus final : public JE::Editor::Tools::AssetStatusReporterSystemRequestBus::Handler
        {
        public:
            GenerationJobStatus() { BusConnect(CanvasHistoryToolId); }
            ~GenerationJobStatus() override { BusDisconnect(); }
            void StartReporting(const AZ::Uuid& requestId, const AZStd::vector<AZStd::string>& paths) override
            {
                m_requestId = requestId;
                m_paths = paths;
                m_active = true;
                ++m_startCount;
            }
            void StopReporting(const AZ::Uuid& requestId) override
            {
                if (!m_active)
                {
                    return;
                }
                EXPECT_EQ(requestId, m_requestId);
                if (requestId == m_requestId)
                {
                    m_active = false;
                    ++m_stopCount;
                }
            }
            void StopReportingAll() override { StopReporting(m_requestId); }
            JE::Editor::Tools::AssetStatusReporterState GetStatus(const AZ::Uuid& requestId) const override
            {
                EXPECT_TRUE(m_active);
                EXPECT_EQ(requestId, m_requestId);
                return m_active && requestId == m_requestId ? JE::Editor::Tools::AssetStatusReporterState::Failed
                                                          : JE::Editor::Tools::AssetStatusReporterState::Invalid;
            }
            AZ::Uuid m_requestId;
            AZStd::vector<AZStd::string> m_paths;
            size_t m_startCount = 0;
            size_t m_stopCount = 0;
            bool m_active = false;
        };
    }

    namespace
    {
        // AP transport only: includes are dependencies, while the material sources have actual jobs.
        class MaterialGenerationJobs final : public AzToolsFramework::AssetSystemJobRequestBus::Handler
        {
        public:
            MaterialGenerationJobs() { BusConnect(); }
            ~MaterialGenerationJobs() override { BusDisconnect(); }
            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfo(
                const AZStd::string& sourcePath, const bool escalateJobs) override
            {
                EXPECT_FALSE(escalateJobs);
                m_queries.push_back(sourcePath);
                m_lastQueryHadNoJobs = sourcePath.ends_with(".azsli");
                if (m_lastQueryHadNoJobs)
                {
                    ++m_includeQueries;
                    return AZ::Success(AzToolsFramework::AssetSystem::JobInfoContainer{});
                }
                EXPECT_TRUE(sourcePath.ends_with(".materialtype") || sourcePath.ends_with(".material"));
                AzToolsFramework::AssetSystem::JobInfo job;
                job.m_jobKey = sourcePath.ends_with(".materialtype") ? "Material Type Builder" : "Material Builder";
                job.m_platform = "pc";
                job.m_jobRunKey = 42;
                job.m_status = m_failMaterial && sourcePath.ends_with(".material")
                    ? AzToolsFramework::AssetSystem::JobStatus::Failed : AzToolsFramework::AssetSystem::JobStatus::Completed;
                return AZ::Success(AzToolsFramework::AssetSystem::JobInfoContainer{job});
            }
            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfoByAssetID(
                const AZ::Data::AssetId&, const bool, bool) override { return AZ::Failure(); }
            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfoByJobKey(
                const AZStd::string&, const bool) override { return AZ::Failure(); }
            AZ::Outcome<AzToolsFramework::AssetSystem::JobStatus> GetAssetJobsStatusByJobKey(
                const AZStd::string&, const bool) override { return AZ::Failure(); }
            AZ::Outcome<AZStd::string> GetJobLog(AZ::u64) override
            {
                ADD_FAILURE() << "Status reporting must not retrieve the full AP log.";
                return AZ::Failure();
            }
            AZStd::vector<AZStd::string> m_queries;
            size_t m_includeQueries = 0;
            bool m_lastQueryHadNoJobs = false;
            bool m_failMaterial = false;
        };

        // Exercise the production reporter synchronously. A bounded test cancellation prevents the RED
        // from hanging on an include with no jobs; it never substitutes a successful AP result.
        class MaterialGenerationReport final : public JE::Editor::Tools::AssetStatusReporterSystemRequestBus::Handler
        {
        public:
            explicit MaterialGenerationReport(const MaterialGenerationJobs& jobs) : m_jobs(jobs) { BusConnect(CanvasHistoryToolId); }
            ~MaterialGenerationReport() override { BusDisconnect(); }
            void StartReporting(const AZ::Uuid& id, const AZStd::vector<AZStd::string>& paths) override
            {
                m_requestId = id;
                m_paths = paths;
                m_reporter = AZStd::make_unique<JE::Editor::Tools::AssetStatusReporter>(paths);
                m_pendingObservations = 0;
            }
            void StopReporting(const AZ::Uuid& id) override
            {
                if (m_reporter)
                {
                    EXPECT_EQ(id, m_requestId);
                    if (id == m_requestId) { m_reporter.reset(); }
                }
            }
            void StopReportingAll() override { m_reporter.reset(); }
            JE::Editor::Tools::AssetStatusReporterState GetStatus(const AZ::Uuid& id) const override
            {
                EXPECT_EQ(id, m_requestId);
                if (!m_reporter || id != m_requestId) { return JE::Editor::Tools::AssetStatusReporterState::Invalid; }
                m_lastState = m_reporter->Update();
                if (m_lastState == JE::Editor::Tools::AssetStatusReporterState::Processing && m_jobs.m_lastQueryHadNoJobs
                    && ++m_pendingObservations == 2)
                {
                    m_canceledPendingInclude = true;
                    return JE::Editor::Tools::AssetStatusReporterState::Invalid;
                }
                return m_lastState;
            }
            const MaterialGenerationJobs& m_jobs;
            AZ::Uuid m_requestId;
            AZStd::vector<AZStd::string> m_paths;
            mutable AZStd::unique_ptr<JE::Editor::Tools::AssetStatusReporter> m_reporter;
            mutable JE::Editor::Tools::AssetStatusReporterState m_lastState = JE::Editor::Tools::AssetStatusReporterState::Invalid;
            mutable size_t m_pendingObservations = 0;
            mutable bool m_canceledPendingInclude = false;
        };

        class MaterialGenerationErrors final : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            MaterialGenerationErrors() { BusConnect(); }
            ~MaterialGenerationErrors() override { BusDisconnect(); }
            bool OnPreError(const char* window, const char*, int, const char*, const char* message) override
            {
                if (AZStd::string_view(window) != "AssetStatusReporter") { return false; }
                m_messages.emplace_back(message);
                return true;
            }
            AZStd::vector<AZStd::string> m_messages;
        };
    }
    // Use the existing application's allocator, NameDictionary and SerializeContext. No RHI or AP is
    // initialized for these producer/layout/cache checks, and failed cache assertions precede descriptor dereferences.
    class MaterialShaderParameterBoundsAssertCapture final : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        MaterialShaderParameterBoundsAssertCapture() { BusConnect(); }
        ~MaterialShaderParameterBoundsAssertCapture() override { BusDisconnect(); }
        bool OnPreAssert(const char*, int, const char*, const char* message) override
        {
            if (AZStd::string_view(message) != "AZStd::vector<>::at - position is out of range") { return false; }
            ++m_count;
            return true;
        }
        size_t m_count = 0;
    };
    class MaterialShaderParameterLayoutCacheTests : public GraphDocumentHistoryTests
    {
    protected:
        void SetUp() override
        {
            GraphDocumentHistoryTests::SetUp();
            ASSERT_FALSE(HasFatalFailure());
            auto* context = m_application->GetSerializeContext();
            m_ownsNameReflection = !context->FindClassData(azrtti_typeid<AZ::Name>());
            if (m_ownsNameReflection) { AZ::Name::Reflect(context); }
            m_ownsLayoutReflection = !context->FindClassData(azrtti_typeid<JE::Render::RPI::MaterialShaderParameterLayout>());
            if (m_ownsLayoutReflection) { JE::Render::RPI::MaterialShaderParameterLayout::Reflect(context); }
            auto& assets = AZ::Data::AssetManager::Instance();
            m_previousMaterialTypeHandler = assets.GetHandler(azrtti_typeid<JE::Render::RPI::MaterialTypeAsset>());
            if (!m_previousMaterialTypeHandler)
            {
                m_materialTypeHandler = JE::Render::RPI::MakeAssetHandler<JE::Render::RPI::MaterialTypeAssetHandler>();
                ASSERT_EQ(assets.GetHandler(azrtti_typeid<JE::Render::RPI::MaterialTypeAsset>()), m_materialTypeHandler.get());
            }
        }

        void TearDown() override
        {
            // Creators/assets are local to the test body and are destroyed before teardown. Flush their
            // queued events while the real handler is still alive, before releasing Name references/reflection.
            auto& assets = AZ::Data::AssetManager::Instance();
            assets.DispatchEvents();
            m_materialTypeHandler.reset(); // Its destructor unregisters only the handler this fixture owns.
            EXPECT_EQ(assets.GetHandler(azrtti_typeid<JE::Render::RPI::MaterialTypeAsset>()), m_previousMaterialTypeHandler);
            auto* context = m_application->GetSerializeContext();
            context->EnableRemoveReflection();
            if (m_ownsLayoutReflection) { JE::Render::RPI::MaterialShaderParameterLayout::Reflect(context); }
            if (m_ownsNameReflection) { AZ::Name::Reflect(context); }
            context->DisableRemoveReflection();
            GraphDocumentHistoryTests::TearDown();
        }

        JE::Render::RPI::MaterialShaderParameterLayout ProduceLayout(const AZStd::vector<AZStd::string>& names)
        {
            JE::Render::RPI::MaterialTypeAssetCreator creator;
            creator.Begin(AZ::Uuid::CreateRandom());
            for (const auto& name : names)
            {
                creator.BeginMaterialProperty(AZ::Name(name), JE::Render::RPI::MaterialPropertyDataType::Vector4);
                creator.ConnectMaterialPropertyToShaderParameter(AZ::Name(name));
                creator.EndMaterialProperty();
            }
            auto layout = creator.CreateMaterialShaderParameterLayout();
            EXPECT_EQ(creator.GetErrorCount(), 0);
            return layout;
        }

        bool RoundTrip(JE::Render::RPI::MaterialShaderParameterNameIndex& source, JE::Render::RPI::MaterialShaderParameterNameIndex& restored)
        {
            auto* context = m_application->GetSerializeContext();
            AZStd::vector<AZ::u8> bytes;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> stream(&bytes);
            if (!AZ::Utils::SaveObjectToStream(stream, AZ::ObjectStream::ST_BINARY, &source, context)) { return false; }
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            if (!AZ::Utils::LoadObjectFromStreamInPlace(stream, restored, context)) { return false; }
            EXPECT_EQ(restored.GetName(), source.GetName());
            EXPECT_EQ(restored.GetIndex(), source.GetIndex());
            AZStd::vector<AZ::u8> savedAgain;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> second(&savedAgain);
            if (!AZ::Utils::SaveObjectToStream(second, AZ::ObjectStream::ST_BINARY, &restored, context)) { return false; }
            EXPECT_EQ(savedAgain, bytes);
            return true;
        }

        void CheckFloat4(const JE::Render::RPI::MaterialShaderParameterLayout& layout, const AZStd::string& name)
        {
            const auto index = layout.GetParameterIndex(name);
            ASSERT_TRUE(index.IsValid());
            ASSERT_LT(index.GetIndex(), layout.GetDescriptors().size());
            const auto* descriptor = layout.GetDescriptor(index);
            ASSERT_NE(descriptor, nullptr);
            EXPECT_EQ(descriptor->m_name, name);
            EXPECT_EQ(descriptor->m_typeName, "float4");
            EXPECT_EQ(descriptor->m_structuredBufferBinding.m_elementSize, 16);
            EXPECT_EQ(descriptor->m_structuredBufferBinding.m_elementCount, 1);
        }

        bool m_ownsNameReflection = false;
        bool m_ownsLayoutReflection = false;
        AZ::Data::AssetHandler* m_previousMaterialTypeHandler = nullptr;
        AZStd::unique_ptr<JE::Render::RPI::MaterialTypeAssetHandler> m_materialTypeHandler;
    };

    TEST_F(MaterialShaderParameterLayoutCacheTests, SerializedCacheRebindsWhenLayoutShrinksPastOldIndex)
    {
        AZStd::vector<AZStd::string> originalNames;
        for (int i = 0; i < 58; ++i) { originalNames.push_back(AZStd::string::format("m_filler%d", i)); }
        originalNames.push_back("m_inverseRow2");
        originalNames.push_back("m_sentinel");
        const auto original = ProduceLayout(originalNames);
        JE::Render::RPI::MaterialShaderParameterNameIndex cached(AZ::Name("m_inverseRow2"));
        ASSERT_TRUE(cached.ValidateOrFindIndex(&original));
        ASSERT_EQ(cached.GetIndex().GetIndex(), 60);
        CheckFloat4(original, "m_inverseRow2");
        CheckFloat4(original, "m_sentinel");
        JE::Render::RPI::MaterialShaderParameterNameIndex restored;
        ASSERT_TRUE(RoundTrip(cached, restored));

        AZStd::vector<AZStd::string> finalNames{"m_inverseRow2"};
        for (int i = 0; i < 54; ++i) { finalNames.push_back(AZStd::string::format("m_filler%d", i)); }
        finalNames.push_back("m_sentinel");
        const auto finalLayout = ProduceLayout(finalNames);
        ASSERT_EQ(finalLayout.GetDescriptors().size(), 59);
        ASSERT_GE(restored.GetIndex().GetIndex(), finalLayout.GetDescriptors().size());
        EXPECT_TRUE(restored.ValidateOrFindIndex(&finalLayout));
        // This must fail before touching the out-of-range descriptor in the original implementation.
        ASSERT_EQ(restored.GetIndex(), finalLayout.GetParameterIndex("m_inverseRow2"));
        ASSERT_LT(restored.GetIndex().GetIndex(), finalLayout.GetDescriptors().size());
        CheckFloat4(finalLayout, "m_inverseRow2");
        CheckFloat4(finalLayout, "m_sentinel");
        EXPECT_EQ(finalLayout.GetDescriptor(restored.GetIndex())->m_name, "m_inverseRow2");
    }

    TEST_F(MaterialShaderParameterLayoutCacheTests, SerializedCacheRebindsWhenSameSizeLayoutReorders)
    {
        const auto original = ProduceLayout({"m_sentinelA", "m_inverseRow2", "m_sentinelB"});
        const auto finalLayout = ProduceLayout({"m_inverseRow2", "m_sentinelA", "m_sentinelB"});
        ASSERT_EQ(original.GetDescriptors().size(), finalLayout.GetDescriptors().size());
        JE::Render::RPI::MaterialShaderParameterNameIndex cached(AZ::Name("m_inverseRow2"));
        ASSERT_TRUE(cached.ValidateOrFindIndex(&original));
        CheckFloat4(original, "m_inverseRow2");
        JE::Render::RPI::MaterialShaderParameterNameIndex restored;
        ASSERT_TRUE(RoundTrip(cached, restored));
        ASSERT_NE(restored.GetIndex(), finalLayout.GetParameterIndex("m_inverseRow2"));
        EXPECT_TRUE(restored.ValidateOrFindIndex(&finalLayout));
        ASSERT_EQ(restored.GetIndex(), finalLayout.GetParameterIndex("m_inverseRow2"));
        CheckFloat4(finalLayout, "m_inverseRow2");
        CheckFloat4(finalLayout, "m_sentinelA");
        CheckFloat4(finalLayout, "m_sentinelB");
        EXPECT_EQ(finalLayout.GetDescriptor(restored.GetIndex())->m_name, "m_inverseRow2");
    }

    TEST_F(MaterialShaderParameterLayoutCacheTests, SerializedCacheRejectsNameRemovedFromFinalLayout)
    {
        const auto original = ProduceLayout({"m_sentinelA", "m_inverseRow2", "m_sentinelB"});
        const auto finalLayout = ProduceLayout({"m_sentinelA", "m_sentinelB"});
        JE::Render::RPI::MaterialShaderParameterNameIndex cached(AZ::Name("m_inverseRow2"));
        ASSERT_TRUE(cached.ValidateOrFindIndex(&original));
        JE::Render::RPI::MaterialShaderParameterNameIndex restored;
        ASSERT_TRUE(RoundTrip(cached, restored));
        EXPECT_FALSE(finalLayout.GetParameterIndex("m_inverseRow2").IsValid());
        EXPECT_FALSE(restored.ValidateOrFindIndex(&finalLayout));
        EXPECT_FALSE(restored.GetIndex().IsValid());
        CheckFloat4(finalLayout, "m_sentinelA");
        CheckFloat4(finalLayout, "m_sentinelB");
    }

    TEST_F(MaterialShaderParameterLayoutCacheTests, DescriptorLookupRejectsNullOnePastAndLargeIndices)
    {
        auto layout = ProduceLayout({"m_inverseRow2", "m_sentinel"});
        const auto& constLayout = layout;
        CheckFloat4(constLayout, "m_inverseRow2");
        CheckFloat4(constLayout, "m_sentinel");
        using Index = JE::Render::RPI::MaterialShaderParameterLayout::Index;
        const Index onePast(static_cast<AZ::u32>(layout.GetDescriptors().size()));
        const Index large(0xFFFFFFFEu);
        // Pointer comparisons only: the RED never dereferences an invalid descriptor.
        MaterialShaderParameterBoundsAssertCapture boundsAsserts;
        for (const auto index : {Index{Index::Null}, onePast, large})
        {
            EXPECT_EQ(layout.GetDescriptor(index), nullptr);
            EXPECT_EQ(constLayout.GetDescriptor(index), nullptr);
        }
        RecordProperty("DescriptorBoundsAssertCount", static_cast<int>(boundsAsserts.m_count));
        EXPECT_EQ(boundsAsserts.m_count, 0);
    }
    class MaterialGraphCompilerGenerationTests : public GraphDocumentHistoryTests
    {
    protected:
        static void ReflectGenerationTypes(AZ::ReflectContext* context)
        {
            JE::Editor::Tools::DynamicNode::Reflect(context);
            JE::Render::RPI::MaterialFunctor::Reflect(context);
            JE::Render::RPI::MaterialPropertyValue::Reflect(context);
            JE::Render::RPI::MaterialPropertySourceData::Reflect(context);
            JE::Render::RPI::MaterialTypeSourceData::Reflect(context);
            JE::Render::RPI::MaterialFunctorSourceData::Reflect(context);
            JE::Render::RPI::MaterialFunctorSourceDataHolder::Reflect(context);
            JE::Render::RPI::LuaMaterialFunctorSourceData::Reflect(context);
        }

        void SetUp() override
        {
            GraphDocumentHistoryTests::SetUp();
            ASSERT_FALSE(HasFatalFailure());
            ReflectGenerationTypes(m_application->GetSerializeContext());
            ReflectGenerationTypes(m_application->GetJsonRegistrationContext());
            m_generationReflected = true;
            ASSERT_EQ(JE::Render::RPI::MaterialFunctorSourceDataRegistration::Get(), nullptr);
            m_functorRegistration = AZStd::make_unique<JE::Render::RPI::MaterialFunctorSourceDataRegistration>();
            m_functorRegistration->Init();
            m_functorRegistration->RegisterMaterialFunctor("Lua", azrtti_typeid<JE::Render::RPI::LuaMaterialFunctorSourceData>());
            m_fileIO->SetAlias("@engroot@", AZ::Test::GetEngineRootPath().c_str());
            m_nodeManager = AZStd::make_unique<JE::Editor::Tools::DynamicNodeManager>(CanvasHistoryToolId);
            m_nodeManager->RegisterDataTypes({
                AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(AZ_CRC_CE("bool"), false, "bool"),
                AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(AZ_CRC_CE("float"), 0.0f, "float"),
                AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(AZ_CRC_CE("float3"), AZ::Vector3::CreateZero(), "float3"),
                AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(AZ_CRC_CE("string"), AZStd::string{}, "string")});
            m_templates = AZStd::make_unique<JE::Editor::Tools::GraphTemplateFileDataCache>(CanvasHistoryToolId);
            const auto assets = AZ::Test::GetEngineRootPath() + "/Editor/Materials/Assets/MaterialCanvas/GraphData/Nodes/";
            ASSERT_TRUE(m_constantConfig.Load(assets + "Constants/float.materialgraphnode"));
            ASSERT_TRUE(m_outputConfig.Load(assets + "MaterialOutputs/StandardPBR/StandardPBR.materialgraphnode"));
        }

        void TearDown() override
        {
            m_generationDocument.reset();
            m_compiler.reset();
            m_generationContext.reset();
            m_templates.reset();
            m_nodeManager.reset();
            m_constantConfig = {};
            m_outputConfig = {};
            m_generationPath.clear();
            if (m_functorRegistration)
            {
                m_functorRegistration->Shutdown();
                m_functorRegistration.reset();
            }
            // Compiler status callbacks own strings; drain them before application/allocator teardown.
            AZ::SystemTickBus::ExecuteQueuedEvents();
            if (m_generationReflected)
            {
                for (AZ::ReflectContext* context : {static_cast<AZ::ReflectContext*>(m_application->GetJsonRegistrationContext()),
                         static_cast<AZ::ReflectContext*>(m_application->GetSerializeContext())})
                {
                    context->EnableRemoveReflection();
                    ReflectGenerationTypes(context);
                    context->DisableRemoveReflection();
                }
            }
            GraphDocumentHistoryTests::TearDown();
        }

        void CreateConnectedMaterialGraph(bool missingTemplate)
        {
            const auto outputRoot = m_directory->Resolve(missingTemplate ? "source/missing-template" : "source/generation").Native();
            ASSERT_TRUE(m_fileIO->CreatePath(outputRoot.c_str()));
            m_generationPath = outputRoot + "/Connected.materialgraph";
            if (missingTemplate)
            {
                // Only an in-memory config changes. The native config and templates remain intact.
                m_outputConfig.m_settings["templatePaths"] = {outputRoot + "/absent.azsli"};
            }
            ASSERT_TRUE(m_nodeManager->RegisterConfig(m_constantConfig));
            ASSERT_TRUE(m_nodeManager->RegisterConfig(m_outputConfig));
            m_generationContext = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphContext>(
                "Material Graph", ".materialgraph", m_nodeManager->GetRegisteredDataTypes());
            JE::Editor::Tools::GraphModel::Graph initial(m_generationContext);
            ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, m_generationPath).IsSuccess());
            m_compiler = AZStd::make_shared<JE::Editor::MaterialGraphCompiler>(CanvasHistoryToolId);
            const auto type = JE::Editor::Tools::GraphDocument::BuildDocumentTypeInfo(
                "Material Graph", {"materialgraph"}, {}, {}, m_generationContext, {});
            m_generationDocument = AZStd::make_unique<JE::Editor::Tools::GraphDocument>(
                CanvasHistoryToolId, type, m_generationContext, m_compiler);
            ASSERT_TRUE(m_generationDocument->Open(m_generationPath));
            auto graph = m_generationDocument->GetGraph();
            auto constant = m_nodeManager->CreateNodeById(graph, m_constantConfig.m_id);
            auto output = m_nodeManager->CreateNodeById(graph, m_outputConfig.m_id);
            ASSERT_NE(constant, nullptr);
            ASSERT_NE(output, nullptr);
            ASSERT_NE(constant->GetSlot("inValue"), nullptr);
            ASSERT_NE(constant->GetSlot("outValue"), nullptr);
            ASSERT_NE(output->GetSlot("inDescription"), nullptr);
            ASSERT_NE(output->GetSlot("inRoughness"), nullptr);
            constant->GetSlot("inValue")->SetValue(0.375f);
            output->GetSlot("inDescription")->SetValue(AZStd::string("Connected compiler regression"));
            const auto graphId = m_generationDocument->GetGraphId();
            for (const auto& node : {constant, output})
            {
                JE::Editor::Tools::GraphCanvas::NodeId canvasId;
                AZ::Vector2 position = AZ::Vector2::CreateZero();
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasId, graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, node, position);
                ASSERT_TRUE(canvasId.IsValid());
            }
            JE::Editor::Tools::GraphModel::ConnectionPtr connection;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(connection, graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddConnection,
                constant->GetSlot("outValue"), output->GetSlot("inRoughness"));
            ASSERT_NE(connection, nullptr);
            EXPECT_EQ(connection->GetSourceSlot(), constant->GetSlot("outValue"));
            EXPECT_EQ(connection->GetTargetSlot(), output->GetSlot("inRoughness"));
            m_constantId = constant->GetId();
        }

        bool m_generationReflected = false;
        AZStd::unique_ptr<JE::Render::RPI::MaterialFunctorSourceDataRegistration> m_functorRegistration;
        AZStd::unique_ptr<JE::Editor::Tools::DynamicNodeManager> m_nodeManager;
        AZStd::unique_ptr<JE::Editor::Tools::GraphTemplateFileDataCache> m_templates;
        JE::Editor::Tools::DynamicNodeConfig m_constantConfig, m_outputConfig;
        JE::Editor::Tools::GraphModel::GraphContextPtr m_generationContext;
        AZStd::shared_ptr<JE::Editor::MaterialGraphCompiler> m_compiler;
        AZStd::unique_ptr<JE::Editor::Tools::GraphDocument> m_generationDocument;
        AZStd::string m_generationPath;
        JE::Editor::Tools::GraphModel::NodeId m_constantId = 0;
    };

    TEST_F(MaterialGraphCompilerGenerationTests, ConnectedStandardPBRGeneratesRealSourcesWithoutClaimingAPSuccess)
    {
        CreateConnectedMaterialGraph(false);
        ASSERT_FALSE(HasFatalFailure());
        GenerationJobStatus jobs;
        const auto graph = m_generationDocument->GetGraph();
        EXPECT_FALSE(m_compiler->CompileGraph(graph, "Connected", m_generationPath));
        EXPECT_EQ(m_compiler->GetState(), JE::Editor::Tools::GraphCompiler::State::Failed);
        ASSERT_EQ(jobs.m_startCount, 1);
        EXPECT_EQ(jobs.m_stopCount, 1);
        const auto root = m_directory->Resolve("source/generation").Native();
        EXPECT_EQ(jobs.m_paths, (AZStd::vector<AZStd::string>{(AZ::IO::Path(root) / "Connected.materialtype").Native(), (AZ::IO::Path(root) / "Connected.material").Native()}));
        ASSERT_EQ(jobs.m_paths.size(), 2);
        ASSERT_EQ(m_compiler->GetGeneratedFilePaths().size(), 9);
        for (const char* suffix : {".material", ".materialtype", "_Defines.azsli", "_Common.azsli", "_DepthVertexData.azsli",
                 "_DepthVertexEval.azsli", "_PixelGeometryEval.azsli", "_SurfaceEval.azsli", "_VertexEval.azsli"})
        {
            const auto source = AZ::Utils::ReadFile(root + "/Connected" + suffix);
            ASSERT_TRUE(source.IsSuccess()) << suffix;
            EXPECT_EQ(source.GetValue().find("MaterialGraphName"), AZStd::string::npos) << suffix;
        }
        const auto material = AZ::JsonSerializationUtils::ReadJsonFile(root + "/Connected.material");
        ASSERT_TRUE(material.IsSuccess());
        ASSERT_TRUE(material.GetValue().HasMember("materialType"));
        ASSERT_TRUE(material.GetValue()["materialType"].IsString());
        EXPECT_STREQ(material.GetValue()["materialType"].GetString(), "./Connected.materialtype");
        const auto materialType = AZ::JsonSerializationUtils::ReadJsonFile(root + "/Connected.materialtype");
        ASSERT_TRUE(materialType.IsSuccess());
        for (const char* key : {"materialShaderCode", "materialShaderDefines", "description", "lightingModel"})
        {
            ASSERT_TRUE(materialType.GetValue().HasMember(key));
            ASSERT_TRUE(materialType.GetValue()[key].IsString());
        }
        EXPECT_STREQ(materialType.GetValue()["materialShaderCode"].GetString(), "Connected_Common.azsli");
        EXPECT_STREQ(materialType.GetValue()["materialShaderDefines"].GetString(), "Connected_Defines.azsli");
        EXPECT_STREQ(materialType.GetValue()["description"].GetString(), "Connected compiler regression");
        EXPECT_STREQ(materialType.GetValue()["lightingModel"].GetString(), "Standard");
        const auto surface = AZ::Utils::ReadFile(root + "/Connected_SurfaceEval.azsli");
        ASSERT_TRUE(surface.IsSuccess());
        EXPECT_NE(surface.GetValue().find("0.375"), AZStd::string::npos);
        // Follow the actual connected value on a second generation, rather than the template's default.
        graph->GetNode(m_constantId)->GetSlot("inValue")->SetValue(0.625f);
        EXPECT_FALSE(m_compiler->CompileGraph(graph, "Connected", m_generationPath));
        EXPECT_EQ(m_compiler->GetState(), JE::Editor::Tools::GraphCompiler::State::Failed);
        EXPECT_EQ(jobs.m_startCount, 2);
        EXPECT_EQ(jobs.m_stopCount, 2);
        const auto changed = AZ::Utils::ReadFile(root + "/Connected_SurfaceEval.azsli");
        ASSERT_TRUE(changed.IsSuccess());
        EXPECT_NE(changed.GetValue().find("0.625"), AZStd::string::npos);
        EXPECT_EQ(changed.GetValue().find("0.375"), AZStd::string::npos);
        EXPECT_NE(changed.GetValue(), surface.GetValue());
    }

    TEST_F(MaterialGraphCompilerGenerationTests, StandardPBRCompletesProductJobsWithoutWaitingForIncludeJobs)
    {
        CreateConnectedMaterialGraph(false);
        ASSERT_FALSE(HasFatalFailure());
        MaterialGenerationJobs jobs;
        MaterialGenerationReport reporting(jobs);
        MaterialGenerationErrors errors;
        EXPECT_TRUE(m_compiler->CompileGraph(m_generationDocument->GetGraph(), "Connected", m_generationPath));
        EXPECT_EQ(m_compiler->GetState(), JE::Editor::Tools::GraphCompiler::State::Complete);
        EXPECT_EQ(reporting.m_lastState, JE::Editor::Tools::AssetStatusReporterState::Succeeded);
        EXPECT_FALSE(reporting.m_canceledPendingInclude) << "The real reporter remained pending on an include with no AP jobs.";
        EXPECT_EQ(jobs.m_includeQueries, 0);
        const auto root = m_directory->Resolve("source/generation").Native();
        const AZStd::vector<AZStd::string> products{(AZ::IO::Path(root) / "Connected.materialtype").Native(), (AZ::IO::Path(root) / "Connected.material").Native()};
        EXPECT_EQ(reporting.m_paths, products);
        EXPECT_EQ(jobs.m_queries, products);
        EXPECT_TRUE(errors.m_messages.empty());
        ASSERT_EQ(m_compiler->GetGeneratedFilePaths().size(), 9);
        for (const auto& output : m_compiler->GetGeneratedFilePaths())
        {
            EXPECT_TRUE(m_fileIO->Exists(output.c_str())) << output.c_str();
        }
        const auto include = AZ::Utils::ReadFile(root + "/Connected_SurfaceEval.azsli");
        ASSERT_TRUE(include.IsSuccess());
        EXPECT_NE(include.GetValue().find("0.375"), AZStd::string::npos);
    }

    TEST_F(MaterialGraphCompilerGenerationTests, StandardPBRProductJobFailureRemainsFailureWithAllSourcesPreserved)
    {
        CreateConnectedMaterialGraph(false);
        ASSERT_FALSE(HasFatalFailure());
        MaterialGenerationJobs jobs;
        jobs.m_failMaterial = true;
        MaterialGenerationReport reporting(jobs);
        MaterialGenerationErrors errors;
        EXPECT_FALSE(m_compiler->CompileGraph(m_generationDocument->GetGraph(), "Connected", m_generationPath));
        EXPECT_EQ(m_compiler->GetState(), JE::Editor::Tools::GraphCompiler::State::Failed);
        EXPECT_EQ(reporting.m_lastState, JE::Editor::Tools::AssetStatusReporterState::Failed);
        EXPECT_FALSE(reporting.m_canceledPendingInclude);
        EXPECT_EQ(jobs.m_includeQueries, 0);
        const auto root = m_directory->Resolve("source/generation").Native();
        const AZStd::vector<AZStd::string> products{(AZ::IO::Path(root) / "Connected.materialtype").Native(), (AZ::IO::Path(root) / "Connected.material").Native()};
        EXPECT_EQ(reporting.m_paths, products);
        EXPECT_EQ(jobs.m_queries, products);
        ASSERT_EQ(errors.m_messages.size(), 1);
        EXPECT_NE(errors.m_messages.front().find("Connected.material"), AZStd::string::npos);
        ASSERT_EQ(m_compiler->GetGeneratedFilePaths().size(), 9);
        for (const auto& output : m_compiler->GetGeneratedFilePaths())
        {
            EXPECT_TRUE(m_fileIO->Exists(output.c_str())) << output.c_str();
        }
    }
    TEST_F(MaterialGraphCompilerGenerationTests, MissingTemplateFailsBeforeAPReportingOrSourcePublication)
    {
        CreateConnectedMaterialGraph(true);
        ASSERT_FALSE(HasFatalFailure());
        GenerationJobStatus jobs;
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_compiler->CompileGraph(m_generationDocument->GetGraph(), "Connected", m_generationPath));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(m_compiler->GetState(), JE::Editor::Tools::GraphCompiler::State::Failed);
        EXPECT_TRUE(m_compiler->GetGeneratedFilePaths().empty());
        EXPECT_EQ(jobs.m_startCount, 0);
        EXPECT_EQ(jobs.m_stopCount, 0);
        const auto root = m_directory->Resolve("source/missing-template").Native();
        for (const char* suffix : {".material", ".materialtype", "_SurfaceEval.azsli"})
        {
            EXPECT_FALSE(m_fileIO->Exists((root + "/Connected" + suffix).c_str()));
        }
    }

    class PassGraphWorkspaceTests : public GraphDocumentHistoryTests
    {
    protected:
        virtual bool SeedCollision() const { return false; }
        virtual void ConfigureNodePaletteConfigs(
            JE::Editor::Tools::DynamicNodeConfig&, JE::Editor::Tools::DynamicNodeConfig&) const {}


        void SetUp() override
        {
            GraphDocumentHistoryTests::SetUp();
            ASSERT_FALSE(HasFatalFailure());
            m_localUserSettings = AZStd::make_unique<AZ::UserSettingsProvider>();
            m_localUserSettings->Activate(AZ::UserSettings::CT_LOCAL);
            m_globalUserSettings = AZStd::make_unique<AZ::UserSettingsProvider>();
            m_globalUserSettings->Activate(AZ::UserSettings::CT_GLOBAL);
            if (!AzQtComponents::StyleManager::isInstanced())
            {
                const AZStd::string engineRoot = AZ::Test::GetEngineRootPath();
                m_qtStyleManager = AZStd::make_unique<AzQtComponents::StyleManager>(qApp);
                m_qtStyleManager->initialize(qApp, AZ::IO::PathView(engineRoot.c_str()));
            }
            m_document.reset();
            m_styleManager.reset();
            m_graphContext.reset();
            m_application->RegisterComponentDescriptor(AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor());
            m_propertyManagerEntity = AZStd::make_unique<AZ::Entity>("Workspace inspector property manager");
            ASSERT_NE(m_propertyManagerEntity->CreateComponent<AzToolsFramework::Components::PropertyManagerComponent>(), nullptr);
            m_propertyManagerEntity->Init();
            m_propertyManagerEntity->Activate();
            ASSERT_EQ(m_propertyManagerEntity->GetState(), AZ::Entity::State::Active);
            for (AZ::ReflectContext* context : {static_cast<AZ::ReflectContext*>(m_application->GetSerializeContext()),
                     static_cast<AZ::ReflectContext*>(m_application->GetJsonRegistrationContext())})
            {
                ReflectWorkspaceTypes(context);
            }
            m_workspaceReflected = true;
            m_fileIO->SetAlias("@engroot@", AZ::Test::GetEngineRootPath().c_str());
            const auto products = m_directory->Resolve("products").Native();
            // Seed this fixture's private stylesheet before enabling the products-write protection.
            m_fileIO->ClearAlias("@products@");
            const auto stylePath = m_directory->Resolve("products/MaterialCanvas/StyleSheet/materialcanvas_style.json").Native();
            const AZStd::string styleDirectory(AZ::IO::Path(stylePath).ParentPath().Native());
            ASSERT_TRUE(m_fileIO->CreatePath(styleDirectory.c_str()));
            const auto style = AZ::Utils::ReadFile(m_directory->Resolve("products/canvas_style.json").Native());
            ASSERT_TRUE(style.IsSuccess());
            ASSERT_TRUE(AZ::Utils::WriteFile(style.GetValue(), stylePath).IsSuccess());
            m_fileIO->SetAlias("@products@", products.c_str());

            JE::Editor::Tools::DynamicNodeConfig material, pass;
            material.m_title = "Only Material";
            pass.m_title = "Only Pass";
            ConfigureNodePaletteConfigs(material, pass);
            m_materialConfigId = material.m_id;
            ASSERT_TRUE(material.Save(m_directory->Resolve("source/material.materialgraphnode").Native()));
            ASSERT_TRUE(pass.Save(m_directory->Resolve("source/pass.passgraphnode").Native()));
            if (SeedCollision())
            {
                pass.m_id = material.m_id;
                pass.m_title = "Rejected Collision";
                ASSERT_TRUE(pass.Save(m_directory->Resolve("source/collision.passgraphnode").Native()));
            }
            JE::Editor::Tools::DocumentTypeInfo materialType;
            materialType.m_documentTypeName = "Material";
            materialType.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& info)
            {
                return aznew JE::Editor::Tools::AtomToolsDocument(toolId, info);
            };
            m_session = AZStd::make_unique<JE::Editor::MaterialWorkspaceSession>(materialType);
            if (SeedCollision())
            {
                AZ_TEST_START_TRACE_SUPPRESSION;
                m_session->EnableGraphDocuments();
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            }
            else
            {
                m_session->EnableGraphDocuments();
            }
        }

        void TearDown() override
        {
            m_session.reset();
            m_propertyManagerEntity.reset();
            m_application->FlushUndo();
            AZ::SystemTickBus::ExecuteQueuedEvents();
            if (m_globalUserSettings)
            {
                m_globalUserSettings->Deactivate();
                m_globalUserSettings.reset();
            }
            if (m_localUserSettings)
            {
                m_localUserSettings->Deactivate();
                m_localUserSettings.reset();
            }
            if (m_workspaceReflected)
            {
                for (AZ::ReflectContext* context : {static_cast<AZ::ReflectContext*>(m_application->GetJsonRegistrationContext()),
                         static_cast<AZ::ReflectContext*>(m_application->GetSerializeContext())})
                {
                    context->EnableRemoveReflection();
                    ReflectWorkspaceTypes(context);
                    context->DisableRemoveReflection();
                }
            }
            GraphDocumentHistoryTests::TearDown();
        }

        static void ReflectWorkspaceTypes(AZ::ReflectContext* context)
        {
            JE::Editor::Tools::DynamicNode::Reflect(context);
            JE::Editor::Tools::DynamicProperty::Reflect(context);
            JE::Editor::Tools::DynamicPropertyGroup::Reflect(context);
            JE::Editor::Tools::GraphCompiler::Reflect(context);
            JE::Editor::Tools::GraphViewSettings::Reflect(context);
            JE::Editor::Tools::GraphViewConstructPresets::Reflect(context);
            JE::Editor::Tools::InspectorWidget::Reflect(context);
            JE::Editor::MaterialGraphEnvironment::Reflect(context);
        }

        AZ::Uuid OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind kind)
        {
            auto& environment = m_session->EnableGraphDocuments();
            JE::Editor::Tools::GraphModel::Graph initial(environment.GetGraphContext(kind));
            const auto path = m_directory->Resolve(kind == JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass
                ? "source/test.passgraph" : "source/test.materialgraph").Native();
            EXPECT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, path).IsSuccess());
            return m_session->OpenDocument(path);
        }

        void InspectMaterialSettings(JE::Editor::MaterialGraphWorkspaceWidget& widget,
            const AZStd::function<void(JE::Editor::Tools::SettingsDialog&, JE::Editor::Tools::DynamicPropertyGroup&)>& inspect,
            bool accept = false)
        {
            auto* button = widget.findChild<QPushButton*>(QStringLiteral("MaterialGraphSettings"));
            ASSERT_NE(button, nullptr);
            ASSERT_TRUE(button->isEnabled());
            const int entryLevel = QThread::currentThread()->loopLevel();
            QPointer<JE::Editor::Tools::SettingsDialog> owned;
            bool inspected = false, expired = false;
            const auto findOwned = [&]()
            {
                owned.clear();
                const auto dialogs = widget.findChildren<QDialog*>();
                int verified = 0;
                for (auto* candidate : dialogs)
                {
                    auto* owner = candidate->parent();
                    while (owner && owner != &widget) { owner = owner->parent(); }
                    if (owner != &widget || candidate->objectName() != QStringLiteral("MaterialGraphSettingsDialog") ||
                        !candidate->inherits("JE::Editor::Tools::SettingsDialog")) { continue; }
                    // EditorTools is static: use the verified runtime class across module metaobjects.
                    owned = static_cast<JE::Editor::Tools::SettingsDialog*>(candidate);
                    ++verified;
                }
                if (verified != 1) { owned.clear(); }
            };
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &widget, [&]()
            {
                expired = true;
                findOwned();
                if (owned && QThread::currentThread()->loopLevel() > entryLevel) { owned->reject(); }
                ADD_FAILURE() << "Owned material settings did not finish within5s";
            });
            QTimer poll;
            poll.setInterval(10);
            QObject::connect(&poll, &QTimer::timeout, &widget, [&]()
            {
                findOwned();
                if (!owned || !owned->isVisible() || QThread::currentThread()->loopLevel() <= entryLevel) { return; }
                poll.stop();
                const auto close = qScopeGuard([&]() { if (owned) { owned->done(accept ? QDialog::Accepted : QDialog::Rejected); } });
                if (expired) { return; }
                JE::Editor::Tools::DynamicPropertyGroup* group = nullptr;
                int matches = 0;
                for (auto* candidate : owned->findChildren<QWidget*>())
                {
                    if (!candidate->inherits("JE::Editor::Tools::InspectorPropertyGroupWidget")) { continue; }
                    auto* panel = static_cast<JE::Editor::Tools::InspectorPropertyGroupWidget*>(candidate);
                    panel->GetPropertyEditor()->EnumerateInstances([&](AzToolsFramework::InstanceDataHierarchy& root)
                    {
                        if (root.GetClassMetadata() && root.GetClassMetadata()->m_typeId == azrtti_typeid<JE::Editor::Tools::DynamicPropertyGroup>() &&
                            root.GetNumInstances() == 1)
                        {
                            group = static_cast<JE::Editor::Tools::DynamicPropertyGroup*>(root.GetInstance(0));
                            ++matches;
                        }
                    });
                }
                ASSERT_EQ(matches, 1);
                ASSERT_NE(group, nullptr);
                inspect(*owned, *group);
                inspected = true;
            });
            timeout.start(5000);
            poll.start();
            button->click();
            poll.stop();
            timeout.stop();
            EXPECT_TRUE(inspected);
            EXPECT_FALSE(expired);
            EXPECT_TRUE(owned.isNull()) << "Settings dialog must release its property inspector before the local group";
        }

        JE::Editor::Tools::GraphViewSettings* FindMaterialGraphViewSettings(JE::Editor::Tools::SettingsDialog& dialog)
        {
            JE::Editor::Tools::GraphViewSettings* settings = nullptr;
            int matches = 0;
            for (auto* candidate : dialog.findChildren<QWidget*>())
            {
                if (!candidate->inherits("JE::Editor::Tools::InspectorPropertyGroupWidget")) { continue; }
                auto* panel = static_cast<JE::Editor::Tools::InspectorPropertyGroupWidget*>(candidate);
                panel->GetPropertyEditor()->EnumerateInstances([&](AzToolsFramework::InstanceDataHierarchy& root)
                {
                    if (root.GetClassMetadata() && root.GetClassMetadata()->m_typeId == azrtti_typeid<JE::Editor::Tools::GraphViewSettings>() &&
                        root.GetNumInstances() == 1)
                    {
                        settings = static_cast<JE::Editor::Tools::GraphViewSettings*>(root.GetInstance(0));
                        ++matches;
                    }
                });
            }
            EXPECT_EQ(matches, 1) << "Settings must expose exactly the existing GraphViewSettings instance";
            return settings;
        }

        void RequestDocumentClose(JE::Editor::MaterialGraphWorkspaceWidget& widget, const AZ::Uuid& id,
            QMessageBox::StandardButton decision = QMessageBox::NoButton)
        {
            auto* selector = widget.findChild<QComboBox*>(QStringLiteral("GraphDocuments"));
            ASSERT_NE(selector, nullptr);
            widget.show();
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            ASSERT_EQ(QApplication::activeModalWidget(), nullptr);
            const int entryLoopLevel = QThread::currentThread()->loopLevel();
            int menuLoopLevel = -1;
            bool actionFound = false, answered = false, deadlineExpired = false;
            QPointer<QMenu> ownedMenu;
            QPointer<QMessageBox> ownedQuestion;
            const auto findQuestion = [&widget, &ownedQuestion]()
            {
                auto questions = widget.findChildren<QMessageBox*>(QString(), Qt::FindDirectChildrenOnly);
                for (auto* wrapper : widget.findChildren<AzQtComponents::WindowDecorationWrapper*>(QString(), Qt::FindDirectChildrenOnly))
                {
                    auto* question = qobject_cast<QMessageBox*>(wrapper->guest());
                    if (question && wrapper->parentWidget() == &widget && question->parentWidget() == wrapper)
                    {
                        questions.push_back(question);
                    }
                }
                QMessageBox* candidate = nullptr;
                const auto buttons = QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel;
                for (auto* question : questions)
                {
                    if (question->icon() != QMessageBox::Question || question->standardButtons() != buttons) { continue; }
                    if (candidate) { ownedQuestion.clear(); return; } // Ambiguous ownership must fail the existing watchdog.
                    candidate = question;
                }
                ownedQuestion = candidate;
            };
            const auto cleanup = [&ownedMenu, &ownedQuestion, &findQuestion, entryLoopLevel, &menuLoopLevel]()
            {
                const int level = QThread::currentThread()->loopLevel();
                // Do not send done/close before the corresponding exec loop has entered: that exit can be lost.
                if (menuLoopLevel >= 0 && level > menuLoopLevel)
                {
                    findQuestion();
                    if (ownedQuestion) { ownedQuestion->done(QMessageBox::Cancel); }
                }
                if (level > entryLoopLevel && ownedMenu) { ownedMenu->close(); }
            };
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &widget, [&deadlineExpired, &cleanup, &findQuestion, &ownedQuestion]()
            {
                deadlineExpired = true;
                findQuestion();
                const bool observedQuestion = !ownedQuestion.isNull();
                const bool visible = ownedQuestion && ownedQuestion->isVisible();
                const bool modal = ownedQuestion && ownedQuestion->isModal();
                cleanup();
                ADD_FAILURE() << "Document close did not finish within5s; owned question=" << observedQuestion
                    << " visible=" << visible << " modal=" << modal
                    << " loopLevel=" << QThread::currentThread()->loopLevel();
            });
            QTimer answer;
            answer.setInterval(10);
            QObject::connect(&answer, &QTimer::timeout, &widget,
                [decision, &answer, &answered, &deadlineExpired, &ownedMenu, &ownedQuestion, &findQuestion, &cleanup, &menuLoopLevel]()
            {
                if (deadlineExpired) { cleanup(); return; }
                if (menuLoopLevel < 0 || QThread::currentThread()->loopLevel() <= menuLoopLevel) { return; }
                findQuestion();
                if (!ownedQuestion || !ownedQuestion->isVisible()) { return; }
                answer.stop();
                const auto buttons = QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel;
                EXPECT_EQ(ownedQuestion->icon(), QMessageBox::Question);
                EXPECT_EQ(ownedQuestion->standardButtons(), buttons);
                EXPECT_EQ(ownedQuestion->defaultButton(), ownedQuestion->button(QMessageBox::Cancel));
                EXPECT_FALSE(ownedMenu && ownedMenu->isVisible());
                if (ownedQuestion->icon() != QMessageBox::Question || ownedQuestion->standardButtons() != buttons ||
                    !ownedQuestion->button(decision))
                {
                    ADD_FAILURE() << "Expected owned Save/Discard/Cancel question or button was not found";
                    cleanup();
                    return;
                }
                answered = true;
                ownedQuestion->button(decision)->click();
            });
            QTimer choose;
            choose.setInterval(10);
            QObject::connect(&choose, &QTimer::timeout, &widget,
                [&widget, id, decision, &choose, &actionFound, &answer, &ownedMenu, &menuLoopLevel,
                    entryLoopLevel, &deadlineExpired, &cleanup]()
            {
                if (QThread::currentThread()->loopLevel() <= entryLoopLevel) { return; }
                ownedMenu = widget.findChild<QMenu*>(QStringLiteral("CloseGraphDocumentMenu"));
                if (deadlineExpired) { cleanup(); return; }
                choose.stop();
                EXPECT_NE(ownedMenu, nullptr);
                if (!ownedMenu) { return; }
                menuLoopLevel = QThread::currentThread()->loopLevel();
                const auto data = QString::fromUtf8(id.ToString<AZStd::string>().c_str());
                for (auto* action : ownedMenu->actions())
                {
                    if (action->data().toString() != data) { continue; }
                    actionFound = true;
                    if (decision != QMessageBox::NoButton) { answer.start(); }
                    ownedMenu->setActiveAction(action);
                    QKeyEvent activate(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                    QApplication::sendEvent(ownedMenu, &activate);
                    break;
                }
                if (!actionFound) { cleanup(); }
            });
            timeout.start(5000);
            choose.start();
            selector->customContextMenuRequested(QPoint());
            // Stack-owned timers stop before captured references expire; QPointer guards destroyed popup/dialog objects.
            timeout.stop();
            answer.stop();
            choose.stop();
            ASSERT_FALSE(deadlineExpired);
            EXPECT_TRUE(actionFound);
            if (decision != QMessageBox::NoButton) { EXPECT_TRUE(answered); }
        }

        struct FramingNotifications final : JE::Editor::Tools::GraphCanvas::ViewNotificationBus::Handler
        {
            explicit FramingNotifications(const JE::Editor::Tools::GraphCanvas::ViewId& viewId) { BusConnect(viewId); }
            ~FramingNotifications() { BusDisconnect(); }
            void OnViewCenteredOnArea() override { ++m_centeredCount; }
            int m_centeredCount = 0;
        };

        void PrepareFramingDocument(AZ::Uuid& id, JE::Editor::Tools::GraphCanvas::GraphId& scene)
        {
            id = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
            ASSERT_FALSE(id.IsNull());
            JE::Editor::Tools::GraphModel::GraphPtr graph;
            JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
                graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
            JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
                scene, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
            ASSERT_NE(graph, nullptr);
            ASSERT_TRUE(scene.IsValid());
            bool success = false;
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
            ASSERT_TRUE(success);
            auto node = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphInputNode>(graph, graph->GetContext()->GetDataType<float>());
            AZ::Vector2 position(460.0f, 300.0f);
            JE::Editor::Tools::GraphCanvas::NodeId canvasNode;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(
                canvasNode, scene, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, node, position);
            ASSERT_TRUE(canvasNode.IsValid());
            JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
                success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
            ASSERT_TRUE(success);
            {
                // Activate and lay out the real scene node without fitting a document view.
                JE::Editor::Tools::GraphView preparationView(
                    m_session->GetToolId(), scene, m_session->EnableGraphDocuments().GetViewSettings());
                QGraphicsLayout* nodeLayout = nullptr;
                JE::Editor::Tools::GraphCanvas::NodeLayoutRequestBus::EventResult(
                    nodeLayout, canvasNode, &JE::Editor::Tools::GraphCanvas::NodeLayoutRequests::GetLayout);
                ASSERT_NE(nodeLayout, nullptr);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
                nodeLayout->activate();
                JE::Editor::Tools::GraphCanvas::NodeUIRequestBus::Event(canvasNode, &JE::Editor::Tools::GraphCanvas::NodeUIRequests::AdjustSize);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                nodeLayout->activate();
                JE::Editor::Tools::GraphCanvas::NodeUIRequestBus::Event(canvasNode, &JE::Editor::Tools::GraphCanvas::NodeUIRequests::AdjustSize);
            }
            JE::Editor::Tools::GraphCanvas::ViewId releasedViewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(releasedViewId, scene, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            ASSERT_FALSE(releasedViewId.IsValid());
        }

        AZStd::unique_ptr<AZ::Entity> m_propertyManagerEntity;
        AZStd::unique_ptr<AZ::UserSettingsProvider> m_localUserSettings;
        AZStd::unique_ptr<AZ::UserSettingsProvider> m_globalUserSettings;
        AZStd::unique_ptr<JE::Editor::MaterialWorkspaceSession> m_session;
        AZ::Uuid m_materialConfigId;
        bool m_workspaceReflected = false;
    };

#if !defined(AZ_RELEASE_BUILD)
    TEST_F(PassGraphWorkspaceTests, OpenMaterialGraphDiagnosticRejectsBeforeMutationAndUnregistersWithOwner)
    {
        auto* console = AZ::Interface<AZ::IConsole>::Get();
        ASSERT_NE(console, nullptr);
        constexpr const char* command = "EditorMaterialSystemComponent.JeOpenMaterialGraph";
        ASSERT_FALSE(console->HasCommand(command));
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        const auto pass = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(material.IsNull());
        ASSERT_FALSE(pass.IsNull());
        const auto ids = m_session->GetDocumentIds();
        const auto active = m_session->GetActiveDocumentId();
        const auto state = m_session->GetCompileState(material);
        const auto path = m_directory->Resolve("source/test.materialgraph").Native();
        const auto passPath = m_directory->Resolve("source/test.passgraph").Native();
        const auto missingPath = m_directory->Resolve("source/missing.materialgraph").Native();
        const auto before = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(before.IsSuccess());
        class Output final : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            Output() { BusConnect(); }
            ~Output() override { BusDisconnect(); }
            bool OnPrintf(const char* window, const char* message) override
            {
                if (AZStd::string_view(window) == "JE.MaterialGraphOpen") { messages.emplace_back(message); }
                return false;
            }
            AZStd::vector<AZStd::string> messages;
        } output;
        {
            JE::Editor::EditorMaterialSystemComponent component;
            ASSERT_TRUE(console->HasCommand(command));
            const AZStd::string inputs[] = {
                command,
                AZStd::string::format("%s \"%s\" \"%s\"", command, path.c_str(), path.c_str()),
                AZStd::string::format("%s relative.materialgraph", command),
                AZStd::string::format("%s \"%s\"", command, passPath.c_str()),
                AZStd::string::format("%s \"%s\"", command, missingPath.c_str()),
                AZStd::string::format("%s %s", command, path.c_str()),
                AZStd::string::format("%s \"%s\"", command, path.c_str()),
                AZStd::string::format("%s \"%s", command, path.c_str()),
                AZStd::string::format("%s %s\"", command, path.c_str())};
            const char* reasons[] = {"rejected: usage", "rejected: usage", "rejected: absolute source path required",
                "rejected: materialgraph source extension required", "rejected: existing source file required",
                "rejected: no active material session", "rejected: no active material session",
                "rejected: paired outer double quotes required", "rejected: paired outer double quotes required"};
            for (size_t index = 0; index < AZ_ARRAY_SIZE(inputs); ++index)
            {
                output.messages.clear();
                EXPECT_TRUE(console->PerformCommand(inputs[index].c_str()).IsSuccess());
                ASSERT_EQ(output.messages.size(), 1) << "case=" << index << " input=" << inputs[index].c_str();
                EXPECT_NE(output.messages.front().find(reasons[index]), AZStd::string::npos)
                    << "case=" << index << " input=" << inputs[index].c_str() << " message=" << output.messages.front().c_str();
                EXPECT_EQ(m_session->GetDocumentIds(), ids);
                EXPECT_EQ(m_session->GetActiveDocumentId(), active);
                EXPECT_EQ(m_session->GetCompileState(material), state);
                bool queued = true;
                JE::Editor::Tools::GraphDocumentRequestBus::EventResult(queued, material,
                    &JE::Editor::Tools::GraphDocumentRequests::IsCompileGraphQueued);
                EXPECT_FALSE(queued);
            }
        }
        EXPECT_FALSE(console->HasCommand(command));
        const auto after = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), before.GetValue());
        // Positive opening uses the real host pane/session; native compilation and preview remain separate GUI gates.
    }

    TEST_F(PassGraphWorkspaceTests, CompileSelectDiagnosticRejectsBeforeMutationAndUnregistersWithOwner)
    {
        auto* console = AZ::Interface<AZ::IConsole>::Get();
        ASSERT_NE(console, nullptr);
        constexpr const char* command = "EditorMaterialSystemComponent.JeCompileGraphAndSelect";
        ASSERT_FALSE(console->HasCommand(command));
        const auto first = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        const auto second = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(first.IsNull());
        ASSERT_FALSE(second.IsNull());
        const auto ids = m_session->GetDocumentIds();
        const auto active = m_session->GetActiveDocumentId();
        const auto state = m_session->GetCompileState(first);
        class Output final : public AZ::Debug::TraceMessageBus::Handler
        {
        public:
            Output() { BusConnect(); }
            ~Output() override { BusDisconnect(); }
            bool OnPrintf(const char* window, const char* message) override
            {
                if (AZStd::string_view(window) == "JE.MaterialCompileSelect") { messages.emplace_back(message); }
                return false;
            }
            AZStd::vector<AZStd::string> messages;
        } output;
        {
            // Construction registers the existing member-console mechanism. No renderer or component activation.
            JE::Editor::EditorMaterialSystemComponent component;
            ASSERT_TRUE(console->HasCommand(command));
            const auto firstText = first.ToString<AZStd::string>();
            const auto secondText = second.ToString<AZStd::string>();
            const AZStd::string inputs[] = {
                command,
                AZStd::string::format("%s %s %s", command, firstText.c_str(), firstText.c_str()),
                AZStd::string::format("%s %s %s", command, firstText.c_str(), secondText.c_str())};
            const char* reasons[] = {"rejected: usage", "rejected: distinct non-null UUIDs required", "rejected: no active material session"};
            for (size_t index = 0; index < AZ_ARRAY_SIZE(inputs); ++index)
            {
                output.messages.clear();
                // Console success means dispatch only; the command explicitly reports its rejection.
                EXPECT_TRUE(console->PerformCommand(inputs[index].c_str()).IsSuccess());
                ASSERT_EQ(output.messages.size(), 1);
                EXPECT_NE(output.messages.front().find(reasons[index]), AZStd::string::npos);
                EXPECT_EQ(m_session->GetDocumentIds(), ids);
                EXPECT_EQ(m_session->GetActiveDocumentId(), active);
                EXPECT_EQ(m_session->GetCompileState(first), state);
                bool queued = true;
                JE::Editor::Tools::GraphDocumentRequestBus::EventResult(queued, first,
                    &JE::Editor::Tools::GraphDocumentRequests::IsCompileGraphQueued);
                EXPECT_FALSE(queued);
            }
        }
        EXPECT_FALSE(console->HasCommand(command));
        // Valid dispatch in the real Editor/provider and natural Completed(A) require the existing Vulkan lease.
    }
#endif

    TEST_F(PassGraphWorkspaceTests, DefaultGraphWorkspaceCreatesOneUntitledDocumentPerSession)
    {
        ASSERT_TRUE(m_session->GetDocumentIds().empty());
        AZ::SettingsRegistry::Get()->Remove("/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart");
        m_session->PrepareGraphWorkspace({});
        ASSERT_EQ(m_session->GetDocumentIds().size(), 1);
        const auto id = m_session->GetActiveDocumentId();
        ASSERT_FALSE(id.IsNull());
        EXPECT_EQ(m_session->GetDocumentIds().front(), id);
        JE::Editor::Tools::DocumentTypeInfo type;
        AZStd::string path;
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(type, id,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetDocumentTypeInfo);
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(path, id,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graph, id,
            &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        EXPECT_EQ(type.m_documentTypeName, "Material Graph");
        EXPECT_TRUE(path.empty());
        EXPECT_NE(graph, nullptr);
        for (int pane = 0; pane != 2; ++pane)
        {
            JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
            widget.hide();
            m_session->PrepareGraphWorkspace({});
            EXPECT_EQ(m_session->GetDocumentIds().size(), 1);
            EXPECT_EQ(m_session->GetActiveDocumentId(), id);
            auto* selector = widget.findChild<QComboBox*>(QStringLiteral("GraphDocuments"));
            ASSERT_NE(selector, nullptr);
            ASSERT_EQ(selector->count(), 1);
            EXPECT_EQ(selector->currentText(), QStringLiteral("Material Graph"));
            EXPECT_EQ(selector->currentData().toString(), QString::fromUtf8(id.ToString<AZStd::string>().c_str()));
        }
        bool closed = false;
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(closed, m_session->GetToolId(),
            &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CloseAllDocuments);
        ASSERT_TRUE(closed);
        ASSERT_TRUE(m_session->GetDocumentIds().empty());
        m_session->PrepareGraphWorkspace({});
        EXPECT_TRUE(m_session->GetDocumentIds().empty());
    }

    TEST_F(PassGraphWorkspaceTests, DefaultGraphWorkspaceDisabledDecisionLastsUntilNewSession)
    {
        auto* registry = AZ::SettingsRegistry::Get();
        ASSERT_TRUE(registry->Set("/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart", false));
        m_session->PrepareGraphWorkspace({});
        EXPECT_TRUE(m_session->GetDocumentIds().empty());
        ASSERT_TRUE(registry->Set("/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart", true));
        m_session->PrepareGraphWorkspace({});
        EXPECT_TRUE(m_session->GetDocumentIds().empty());
        const auto type = m_session->GetDocumentType();
        m_session.reset();
        m_session = AZStd::make_unique<JE::Editor::MaterialWorkspaceSession>(type);
        m_session->PrepareGraphWorkspace({});
        ASSERT_EQ(m_session->GetDocumentIds().size(), 1);
        EXPECT_FALSE(m_session->GetActiveDocumentId().IsNull());
    }

    TEST_F(PassGraphWorkspaceTests, DefaultGraphWorkspacePreservesExistingDirtyGraphAndActivePass)
    {
        AZ::Uuid material;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(material, scene));
        const auto pass = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(pass.IsNull());
        const auto ids = m_session->GetDocumentIds();
        bool modified = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(modified, material,
            &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
        ASSERT_TRUE(modified);
        m_session->PrepareGraphWorkspace({});
        EXPECT_EQ(m_session->GetDocumentIds(), ids);
        EXPECT_EQ(m_session->GetActiveDocumentId(), pass);
        modified = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(modified, material,
            &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
        EXPECT_TRUE(modified);
    }

    TEST_F(PassGraphWorkspaceTests, DefaultGraphWorkspacePreservesExistingMaterialDocument)
    {
        AZ::Uuid material;
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(material, m_session->GetToolId(),
            &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CreateDocumentFromTypeName, "Material");
        ASSERT_FALSE(material.IsNull());
        JE::Editor::Tools::AtomToolsDocumentNotificationBus::Event(m_session->GetToolId(),
            &JE::Editor::Tools::AtomToolsDocumentNotifications::OnDocumentOpened, material);
        m_session->PrepareGraphWorkspace({});
        ASSERT_EQ(m_session->GetDocumentIds().size(), 1);
        EXPECT_EQ(m_session->GetDocumentIds().front(), material);
        EXPECT_EQ(m_session->GetActiveDocumentId(), material);
    }

    TEST_F(PassGraphWorkspaceTests, DefaultGraphWorkspaceExplicitSourceWinsWithoutExtraDocument)
    {
        auto& environment = m_session->EnableGraphDocuments();
        JE::Editor::Tools::GraphModel::Graph initial(environment.GetGraphContext(
            JE::Editor::MaterialGraphEnvironment::GraphKind::Material));
        const auto path = m_directory->Resolve("source/explicit.materialgraph").Native();
        ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, path).IsSuccess());
        m_session->PrepareGraphWorkspace(path);
        ASSERT_TRUE(m_session->GetDocumentIds().empty());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        const auto id = widget.OpenDocument(path);
        ASSERT_FALSE(id.IsNull());
        m_session->PrepareGraphWorkspace({});
        ASSERT_EQ(m_session->GetDocumentIds().size(), 1);
        EXPECT_EQ(m_session->GetDocumentIds().front(), id);
        EXPECT_EQ(m_session->GetActiveDocumentId(), id);
    }

    TEST_F(PassGraphWorkspaceTests, DefaultGraphWorkspaceExplicitIntentNeverFallsBackToUntitled)
    {
        // Preparation does not open paths. Even an unavailable explicit source consumes the default decision.
        m_session->PrepareGraphWorkspace(m_directory->Resolve("source/missing.materialgraph").Native());
        EXPECT_TRUE(m_session->GetDocumentIds().empty());
        m_session->PrepareGraphWorkspace({});
        EXPECT_TRUE(m_session->GetDocumentIds().empty());
    }

    TEST_F(PassGraphWorkspaceTests, DynamicEnumInspectorRefreshAfterSaveConfigKeepsLiveComparisonAttributes)
    {
        JE::Editor::Tools::DynamicPropertyConfig config;
        config.m_name = "blendMode";
        config.m_displayName = "Vertex Color Blend Mode";
        config.m_originalValue = AZ::u32{0};
        config.m_enumValues = {"Disabled", "Enabled"};
        JE::Editor::Tools::DynamicPropertyGroup group;
        group.m_properties.emplace_back(config);

        // Use the production panel: its comparison instance is the same live group.
        // Declare the group first so both RPE hierarchies die before their property owner.
        JE::Editor::Tools::InspectorPropertyGroupWidget panel(
            &group, &group, azrtti_typeid<JE::Editor::Tools::DynamicPropertyGroup>(), nullptr, nullptr);
        auto* editor = panel.GetPropertyEditor();
        ASSERT_NE(editor, nullptr);
        editor->ForceQueuedInvalidation();
        QApplication::processEvents();
        const auto combos = panel.findChildren<QComboBox*>();
        ASSERT_EQ(combos.size(), 1);
        auto* combo = combos.front();
        ASSERT_EQ(combo->count(), 2);
        EXPECT_EQ(combo->itemText(0), QStringLiteral("Disabled"));
        EXPECT_EQ(combo->itemText(1), QStringLiteral("Enabled"));
        EXPECT_EQ(combo->currentIndex(), 0);

        auto& property = group.m_properties.front();
        property.SetValue(AZ::u32{1});
        auto savedConfig = property.GetConfig();
        savedConfig.m_originalValue = property.GetValue();
        property.SetConfig(savedConfig); // Same clean-baseline update as MaterialDocument::Save.
        panel.Refresh();
        // DoRefresh must rebuild comparison data and consume the existing main hierarchy.
        // Do not rebuild or clear the main tree first: that hides the lifetime defect.
        QApplication::processEvents();

        const auto refreshed = panel.findChildren<QComboBox*>();
        ASSERT_EQ(refreshed.size(), 1);
        EXPECT_EQ(refreshed.front(), combo);
        EXPECT_EQ(combo->count(), 2);
        EXPECT_EQ(combo->itemText(0), QStringLiteral("Disabled"));
        EXPECT_EQ(combo->itemText(1), QStringLiteral("Enabled"));
        EXPECT_EQ(combo->currentIndex(), 1);
        EXPECT_EQ(AZStd::any_cast<AZ::u32>(property.GetConfig().m_originalValue), 1);
    }
    TEST_F(PassGraphWorkspaceTests, KeyboardUndoRedoKeepsFocusAfterInspectorRebuild)
    {
        AZ::Uuid id;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(id, scene));
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        widget.resize(1200, 800);
        widget.show();
        widget.activateWindow();
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph, nullptr);
        ASSERT_EQ(graph->GetNodeCount(), 1);
        const auto node = graph->GetNodes().begin()->second;
        const auto nodeId = node->GetId();
        bool success = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
        ASSERT_TRUE(success);
        const auto defaultValue = node->GetSlot("defaultValue");
        ASSERT_NE(defaultValue, nullptr);
        defaultValue->SetValue(0.25f);
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        ASSERT_TRUE(success);
        JE::Editor::Tools::GraphCanvas::NodeId canvasNode;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasNode, scene,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeIdByNode, node);
        ASSERT_TRUE(canvasNode.IsValid());
        JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::Event(
            canvasNode, &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        auto processRebuild = []()
        {
            AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
            QApplication::processEvents(QEventLoop::AllEvents);
        };
        processRebuild();
        QWidget* inspector = nullptr;
        for (auto* child : widget.findChildren<QWidget*>())
        {
            if (child->qt_metacast("JE::Editor::Tools::AtomToolsDocumentInspector"))
            {
                inspector = child;
                break;
            }
        }
        ASSERT_NE(inspector, nullptr);
        QElapsedTimer layoutDeadline;
        layoutDeadline.start();
        while (inspector->findChildren<QDoubleSpinBox*>().isEmpty() && layoutDeadline.elapsed() < 1000)
        {
            QApplication::processEvents(QEventLoop::AllEvents);
            widget.layout()->activate();
        }
        const auto editors = inspector->findChildren<QDoubleSpinBox*>();
        ASSERT_EQ(editors.size(), 1);
        auto* editor = editors.front();
        ASSERT_TRUE(editor->isVisible());
        ASSERT_TRUE(editor->isEnabled());
        ASSERT_DOUBLE_EQ(editor->value(), 0.25);
        m_application->FlushUndo();
        editor->setFocus(Qt::OtherFocusReason);
        QApplication::processEvents(QEventLoop::AllEvents);
        auto* initialFocus = QApplication::focusWidget();
        ASSERT_NE(initialFocus, nullptr);
        ASSERT_TRUE(initialFocus == editor || editor->isAncestorOf(initialFocus));
        QPointer<QDoubleSpinBox> originalEditor(editor);
        editor->setValue(5.0);
        QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
        auto* tabFocus = QApplication::focusWidget();
        ASSERT_NE(tabFocus, nullptr);
        QApplication::sendEvent(tabFocus, &tab);
        QApplication::processEvents(QEventLoop::AllEvents);
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        auto checkValue = [&](float expected)
        {
            JE::Editor::Tools::GraphModel::GraphPtr currentGraph;
            JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
                currentGraph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
            ASSERT_NE(currentGraph, nullptr);
            const auto currentNode = currentGraph->GetNode(nodeId);
            ASSERT_NE(currentNode, nullptr);
            const auto currentValue = currentNode->GetSlot("defaultValue");
            ASSERT_NE(currentValue, nullptr);
            EXPECT_FLOAT_EQ(currentValue->GetValue<float>(), expected);
        };
        ASSERT_NO_FATAL_FAILURE(checkValue(5.0f));
        auto sendHistoryKey = [&](Qt::Key key)
        {
            auto* focus = QApplication::focusWidget();
            ASSERT_NE(focus, nullptr);
            ASSERT_TRUE(focus == &widget || widget.isAncestorOf(focus));
            QKeyEvent shortcut(QEvent::ShortcutOverride, key, Qt::ControlModifier);
            QApplication::sendEvent(focus, &shortcut);
            focus = QApplication::focusWidget();
            ASSERT_NE(focus, nullptr);
            QKeyEvent press(QEvent::KeyPress, key, Qt::ControlModifier);
            QApplication::sendEvent(focus, &press);
            processRebuild();
        };
        ASSERT_NO_FATAL_FAILURE(sendHistoryKey(Qt::Key_Z));
        ASSERT_NO_FATAL_FAILURE(checkValue(0.25f));
        ASSERT_TRUE(m_application->GetUndoStack()->CanRedo());
        ASSERT_TRUE(originalEditor.isNull()) << "Undo must rebuild the real Inspector before the next shortcut.";
        auto* afterUndoFocus = QApplication::focusWidget();
        ASSERT_NE(afterUndoFocus, nullptr);
        ASSERT_TRUE(afterUndoFocus == &widget || widget.isAncestorOf(afterUndoFocus));
        ASSERT_NO_FATAL_FAILURE(sendHistoryKey(Qt::Key_Y));
        ASSERT_NO_FATAL_FAILURE(checkValue(5.0f));
        ASSERT_FALSE(m_application->GetUndoStack()->CanRedo());
        ASSERT_NO_FATAL_FAILURE(sendHistoryKey(Qt::Key_Z));
        ASSERT_NO_FATAL_FAILURE(checkValue(0.25f));
    }
    TEST_F(PassGraphWorkspaceTests, DockedWorkspaceKeyboardHistoryUsesHostWindowRoute)
    {
        AZ::Uuid id;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(id, scene));
        QMainWindow host;
        host.setObjectName("HistoryHost");
        host.setFocusPolicy(Qt::StrongFocus);
        AzQtComponents::StyledDockWidget dock("Material Graph Workspace", &host);
        dock.setObjectName("HistoryDock");
        dock.setFocusPolicy(Qt::StrongFocus);
        AzQtComponents::MarkAsShortcutSearchBreak(&dock);
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        widget.setObjectName("HistoryWorkspace");
        dock.setWidget(&widget);
        host.addDockWidget(Qt::BottomDockWidgetArea, &dock);
        host.resize(1200, 800);
        host.show();
        QWidget* const rootWidget = host.window();
        ASSERT_NE(rootWidget, nullptr);
        QWindow* const rootWindow = rootWidget->windowHandle();
        ASSERT_NE(rootWindow, nullptr);
        RecordProperty("host_root_class", rootWidget->metaObject()->className());
        RecordProperty("host_is_root_widget", rootWidget == &host ? 1 : 0);
        host.activateWindow();
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph, nullptr);
        ASSERT_EQ(graph->GetNodeCount(), 1);
        const auto node = graph->GetNodes().begin()->second;
        const auto nodeId = node->GetId();
        bool success = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
        ASSERT_TRUE(success);
        const auto defaultValue = node->GetSlot("defaultValue");
        ASSERT_NE(defaultValue, nullptr);
        defaultValue->SetValue(0.25f);
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        ASSERT_TRUE(success);
        JE::Editor::Tools::GraphCanvas::NodeId canvasNode;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasNode, scene,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeIdByNode, node);
        ASSERT_TRUE(canvasNode.IsValid());
        JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::Event(
            canvasNode, &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        auto processRebuild = []()
        {
            AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
            QApplication::processEvents(QEventLoop::AllEvents);
        };
        processRebuild();
        QWidget* inspector = nullptr;
        for (auto* child : widget.findChildren<QWidget*>())
        {
            if (child->qt_metacast("JE::Editor::Tools::AtomToolsDocumentInspector"))
            {
                inspector = child;
                break;
            }
        }
        ASSERT_NE(inspector, nullptr);
        QElapsedTimer layoutDeadline;
        layoutDeadline.start();
        while (inspector->findChildren<QDoubleSpinBox*>().isEmpty() && layoutDeadline.elapsed() < 1000)
        {
            QApplication::processEvents(QEventLoop::AllEvents);
            widget.layout()->activate();
        }
        const auto editors = inspector->findChildren<QDoubleSpinBox*>();
        ASSERT_EQ(editors.size(), 1);
        auto* editor = editors.front();
        ASSERT_TRUE(editor->isVisible());
        ASSERT_TRUE(editor->isEnabled());
        ASSERT_DOUBLE_EQ(editor->value(), 0.25);
        m_application->FlushUndo();
        editor->setFocus(Qt::OtherFocusReason);
        QApplication::processEvents(QEventLoop::AllEvents);
        auto* initialFocus = QApplication::focusWidget();
        ASSERT_NE(initialFocus, nullptr);
        ASSERT_TRUE(initialFocus == editor || editor->isAncestorOf(initialFocus));
        QPointer<QDoubleSpinBox> originalEditor(editor);
        editor->setValue(5.0);
        // Commit without depending on the host's next Tab stop.
        auto* commitFocus = QApplication::focusWidget();
        ASSERT_NE(commitFocus, nullptr);
        QKeyEvent commitPress(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(rootWindow, &commitPress);
        QKeyEvent commitRelease(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(rootWindow, &commitRelease);
        QApplication::processEvents(QEventLoop::AllEvents);
        auto* committedFocus = QApplication::focusWidget();
        ASSERT_NE(committedFocus, nullptr);
        ASSERT_TRUE(committedFocus == &widget || widget.isAncestorOf(committedFocus));
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        auto checkValue = [&](float expected)
        {
            JE::Editor::Tools::GraphModel::GraphPtr currentGraph;
            JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
                currentGraph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
            ASSERT_NE(currentGraph, nullptr);
            const auto currentNode = currentGraph->GetNode(nodeId);
            ASSERT_NE(currentNode, nullptr);
            const auto currentValue = currentNode->GetSlot("defaultValue");
            ASSERT_NE(currentValue, nullptr);
            EXPECT_FLOAT_EQ(currentValue->GetValue<float>(), expected);
        };
        ASSERT_NO_FATAL_FAILURE(checkValue(5.0f));
        auto recordHostFocus = [&](const char* phase)
        {
            auto* focus = QApplication::focusWidget();
            auto* object = rootWindow->focusObject();
            RecordProperty(AZStd::string::format("%s_widget_class", phase).c_str(),
                focus ? focus->metaObject()->className() : "null");
            RecordProperty(AZStd::string::format("%s_widget_name", phase).c_str(),
                focus ? focus->objectName().toUtf8().constData() : "null");
            RecordProperty(AZStd::string::format("%s_object_class", phase).c_str(),
                object ? object->metaObject()->className() : "null");
            RecordProperty(AZStd::string::format("%s_workspace_scope", phase).c_str(),
                focus && (focus == &widget || widget.isAncestorOf(focus)) ? 1 : 0);
            RecordProperty(AZStd::string::format("%s_host_focus_window", phase).c_str(),
                QApplication::focusWindow() == rootWindow ? 1 : 0);
        };
        int historyCommand = 0;
        auto sendHistoryKey = [&](Qt::Key key)
        {
            const auto prefix = AZStd::string::format("command%d_%s", ++historyCommand, key == Qt::Key_Y ? "redo" : "undo");
            const auto before = prefix + "_before_activation";
            const auto activated = prefix + "_after_activation";
            const auto dispatched = prefix + "_after_dispatch";
            recordHostFocus(before.c_str());
            // The GUI input API activates its target top-level window on every key command.
            host.activateWindow();
            QApplication::processEvents(QEventLoop::AllEvents);
            recordHostFocus(activated.c_str());
            ASSERT_EQ(QApplication::focusWindow(), rootWindow);
            QKeyEvent shortcut(QEvent::ShortcutOverride, key, Qt::ControlModifier);
            QApplication::sendEvent(rootWindow, &shortcut);
            QKeyEvent press(QEvent::KeyPress, key, Qt::ControlModifier);
            QApplication::sendEvent(rootWindow, &press);
            QKeyEvent release(QEvent::KeyRelease, key, Qt::ControlModifier);
            QApplication::sendEvent(rootWindow, &release);
            processRebuild();
            recordHostFocus(dispatched.c_str());
        };
        ASSERT_NO_FATAL_FAILURE(sendHistoryKey(Qt::Key_Z));
        ASSERT_NO_FATAL_FAILURE(checkValue(0.25f));
        ASSERT_TRUE(m_application->GetUndoStack()->CanRedo());
        ASSERT_TRUE(originalEditor.isNull()) << "Undo must rebuild the real Inspector before the next shortcut.";
        auto* afterUndoFocus = QApplication::focusWidget();
        EXPECT_NE(afterUndoFocus, nullptr);
        EXPECT_TRUE(afterUndoFocus && (afterUndoFocus == &widget || widget.isAncestorOf(afterUndoFocus)));
        ASSERT_NO_FATAL_FAILURE(sendHistoryKey(Qt::Key_Y));
        ASSERT_NO_FATAL_FAILURE(checkValue(5.0f));
        ASSERT_FALSE(m_application->GetUndoStack()->CanRedo());
        ASSERT_NO_FATAL_FAILURE(sendHistoryKey(Qt::Key_Z));
        ASSERT_NO_FATAL_FAILURE(checkValue(0.25f));
    }
    TEST_F(PassGraphWorkspaceTests, BookmarksReducedTabReportsScrollContainment)
    {
        const auto id = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(id.IsNull());
        QMainWindow host;
        AzQtComponents::StyledDockWidget dock("Material Graph Workspace", &host);
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        dock.setWidget(&widget);
        host.addDockWidget(Qt::BottomDockWidgetArea, &dock);
        // Approximate R15 content bounds at 150%: 1000x549 physical -> 667x366 logical.
        const QSize requestedWorkspaceSize(667, 366);
        const int requestedNavigationHeight = 109; // Approximately 164 physical pixels in R15.
        host.resize(667, 396);
        host.show();
        auto settleLayout = [&]()
        {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
            host.layout()->activate();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        };
        settleLayout();
        host.resize(host.size() + requestedWorkspaceSize - widget.size());
        settleLayout();
        auto* bookmarks = widget.findChild<QDockWidget*>("BookmarkDockWidget");
        ASSERT_NE(bookmarks, nullptr);
        QWidget* ancestor = bookmarks->parentWidget();
        while (ancestor && !qobject_cast<QTabWidget*>(ancestor)) { ancestor = ancestor->parentWidget(); }
        auto* navigation = qobject_cast<QTabWidget*>(ancestor);
        ASSERT_NE(navigation, nullptr);
        auto* column = qobject_cast<QSplitter*>(navigation->parentWidget());
        ASSERT_NE(column, nullptr);
        navigation->setCurrentWidget(bookmarks);
        column->setSizes({column->height() - column->handleWidth() - requestedNavigationHeight, requestedNavigationHeight});
        settleLayout();
        auto* outer = bookmarks->findChild<QScrollArea*>("scrollArea");
        auto* create = bookmarks->findChild<QPushButton*>("createBookmarkButton");
        auto* remove = bookmarks->findChild<QPushButton*>("deleteBookmarkButton");
        ASSERT_NE(outer, nullptr);
        ASSERT_NE(create, nullptr);
        ASSERT_NE(remove, nullptr);
        ASSERT_NE(outer->widget(), nullptr);
        auto* bar = outer->verticalScrollBar();
        ASSERT_NE(bar, nullptr);
        bar->setValue(bar->maximum());
        settleLayout();
        auto recordRect = [&](const AZStd::string& key, const QRect& rect)
        {
            RecordProperty(key.c_str(), AZStd::string::format("%d,%d,%d,%d", rect.x(), rect.y(), rect.width(), rect.height()).c_str());
        };
        auto recordWidget = [&](const char* prefix, QWidget* child)
        {
            const AZStd::string key(prefix);
            recordRect(key + "_global_rect", QRect(child->mapToGlobal(QPoint()), child->size()));
            recordRect(key + "_visible_global_bounds", child->visibleRegion().boundingRect().translated(child->mapToGlobal(QPoint())));
            recordRect(key + "_minimum", QRect(QPoint(), child->minimumSize()));
            recordRect(key + "_minimum_hint", QRect(QPoint(), child->minimumSizeHint()));
            RecordProperty((key + "_vertical_policy").c_str(), static_cast<int>(child->sizePolicy().verticalPolicy()));
        };
        RecordProperty("platform", QGuiApplication::platformName().toUtf8().constData());
        RecordProperty("device_pixel_ratio_milli", qRound(widget.devicePixelRatioF() * 1000.0));
        recordRect("requested_workspace", QRect(QPoint(), requestedWorkspaceSize));
        RecordProperty("requested_navigation_height", requestedNavigationHeight);
        RecordProperty("outer_scroll_value", bar->value());
        RecordProperty("outer_scroll_minimum", bar->minimum());
        RecordProperty("outer_scroll_maximum", bar->maximum());
        RecordProperty("outer_scroll_page_step", bar->pageStep());
        RecordProperty("dock_vertical_constraint", static_cast<int>(bookmarks->layout()->verticalSizeConstraint()));
        RecordProperty("dock_horizontal_constraint", static_cast<int>(bookmarks->layout()->horizontalSizeConstraint()));
        RecordProperty("content_vertical_constraint", static_cast<int>(bookmarks->widget()->layout()->verticalSizeConstraint()));
        RecordProperty("scroll_content_vertical_constraint", static_cast<int>(outer->widget()->layout()->verticalSizeConstraint()));
        recordWidget("workspace", &widget);
        recordWidget("navigation", navigation);
        recordWidget("stack", bookmarks->parentWidget());
        recordWidget("dock", bookmarks);
        recordWidget("dock_content", bookmarks->widget());
        recordWidget("outer_scroll", outer);
        recordWidget("outer_viewport", outer->viewport());
        recordWidget("scroll_content", outer->widget());
        recordWidget("create", create);
        recordWidget("delete", remove);
        // Emit every diagnostic before checking containment; this case must not mask a clipped ancestor.
        ASSERT_EQ(widget.size(), requestedWorkspaceSize);
        ASSERT_EQ(navigation->height(), requestedNavigationHeight);
        ASSERT_EQ(bar->value(), bar->maximum());
        const QRect viewportGlobal(outer->viewport()->mapToGlobal(QPoint()), outer->viewport()->size());
        for (auto* button : {create, remove})
        {
            const QRect buttonGlobal(button->mapToGlobal(QPoint()), button->size());
            EXPECT_TRUE(button->isVisible()) << button->objectName().toUtf8().constData();
            EXPECT_TRUE(viewportGlobal.contains(buttonGlobal)) << button->objectName().toUtf8().constData();
            EXPECT_TRUE((QRegion(button->rect()) - button->visibleRegion()).isEmpty())
                << button->objectName().toUtf8().constData() << " must be fully visible through every ancestor.";
        }
    }

    TEST_F(PassGraphWorkspaceTests, InitialGraphFramingMatchesLayoutReadyControl)
    {
        const auto id = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(id.IsNull());
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            scene, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
        ASSERT_NE(graph, nullptr);
        ASSERT_TRUE(scene.IsValid());
        bool success = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
        ASSERT_TRUE(success);
        auto node = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphInputNode>(graph, graph->GetContext()->GetDataType<float>());
        AZ::Vector2 position(460.0f, 300.0f);
        JE::Editor::Tools::GraphCanvas::NodeId canvasNode;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(
            canvasNode, scene, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, node, position);
        ASSERT_TRUE(canvasNode.IsValid());
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        ASSERT_TRUE(success);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        {
            struct ViewParamChanges final : JE::Editor::Tools::GraphCanvas::ViewNotificationBus::Router
            {
                ViewParamChanges() { BusRouterConnect(); }
                ~ViewParamChanges() { BusRouterDisconnect(); }
                void OnViewParamsChanged(const JE::Editor::Tools::GraphCanvas::ViewParams&) override { ++changes; }
                int changes = 0;
            } viewParamChanges;
            JE::Editor::Tools::GraphCanvas::ViewId preparationViewId;
            {
                // Registering the first real view activates scene items/styles, but GraphView does not fit the graph.
                JE::Editor::Tools::GraphView preparationView(
                    m_session->GetToolId(), scene, m_session->EnableGraphDocuments().GetViewSettings());
                JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(
                    preparationViewId, scene, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
                ASSERT_TRUE(preparationViewId.IsValid());
                qreal preparationZoom = 0.0;
                JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(
                    preparationZoom, preparationViewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetZoomLevel);
                ASSERT_DOUBLE_EQ(preparationZoom, 1.0);
                RecordProperty("preparation_zoom", QString::number(preparationZoom, 'g', 17).toStdString());

                QGraphicsLayout* nodeLayout = nullptr;
                JE::Editor::Tools::GraphCanvas::NodeLayoutRequestBus::EventResult(
                    nodeLayout, canvasNode, &JE::Editor::Tools::GraphCanvas::NodeLayoutRequests::GetLayout);
                ASSERT_NE(nodeLayout, nullptr);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
                nodeLayout->activate();
                JE::Editor::Tools::GraphCanvas::NodeUIRequestBus::Event(canvasNode, &JE::Editor::Tools::GraphCanvas::NodeUIRequests::AdjustSize);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                nodeLayout->activate();
                JE::Editor::Tools::GraphCanvas::NodeUIRequestBus::Event(canvasNode, &JE::Editor::Tools::GraphCanvas::NodeUIRequests::AdjustSize);

                QRectF preparedArea;
                JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(
                    preparedArea, preparationViewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetCompleteArea);
                ASSERT_FALSE(preparedArea.isEmpty());
                RecordProperty("prepared_node_width", QString::number(preparedArea.width(), 'g', 17).toStdString());
                RecordProperty("prepared_node_height", QString::number(preparedArea.height(), 'g', 17).toStdString());
            }
            JE::Editor::Tools::GraphCanvas::ViewId releasedViewId;
            JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(releasedViewId, scene, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
            ASSERT_FALSE(releasedViewId.IsValid());
            ASSERT_FALSE(JE::Editor::Tools::GraphCanvas::ViewRequestBus::HasHandlers(preparationViewId));
            ASSERT_EQ(viewParamChanges.changes, 0) << "Preparation persisted zoom/anchor; camera isolation is incomplete.";
        }
        ASSERT_EQ(QApplication::mouseButtons(), Qt::NoButton);

        QWidget host;
        host.resize(900, 650);
        auto* layout = new QVBoxLayout(&host);
        auto view = AZStd::make_unique<JE::Editor::Tools::GraphDocumentView>(
            m_session->GetToolId(), id, m_session->EnableGraphDocuments().GetViewSettings(), &host);
        const auto graphicsViews = view->findChildren<QGraphicsView*>();
        ASSERT_EQ(graphicsViews.size(), 1);
        auto* graphicsView = graphicsViews.front();
        JE::Editor::Tools::GraphCanvas::ViewId viewId;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, scene, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
        ASSERT_TRUE(viewId.IsValid());

        const QSize hiddenViewport = graphicsView->viewport()->size();
        const QTransform hiddenTransform = graphicsView->transform();
        QRectF initialArea;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(initialArea, viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetCompleteArea);
        ASSERT_FALSE(initialArea.isEmpty());
        RecordProperty("hidden_viewport_width", hiddenViewport.width());
        RecordProperty("hidden_viewport_height", hiddenViewport.height());
        RecordProperty("hidden_zoom", QString::number(hiddenTransform.m11(), 'g', 17).toStdString());

        layout->addWidget(view.get());
        host.ensurePolished();
        host.show();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        layout->activate();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        layout->activate();
        ASSERT_TRUE(graphicsView->isVisible());
        const QSize laidOutViewport = graphicsView->viewport()->size();
        const QTransform initialAfterLayout = graphicsView->transform();
        QRectF laidOutArea;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(laidOutArea, viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetCompleteArea);
        RecordProperty("laid_out_viewport_width", laidOutViewport.width());
        RecordProperty("laid_out_viewport_height", laidOutViewport.height());
        RecordProperty("initial_zoom_after_layout", QString::number(initialAfterLayout.m11(), 'g', 17).toStdString());
        ASSERT_GT(laidOutViewport.width(), hiddenViewport.width());
        ASSERT_GT(laidOutViewport.height(), hiddenViewport.height());
        ASSERT_EQ(laidOutArea, initialArea) << "Node bounds changed; viewport timing alone is not isolated.";

        // Re-run the existing fit from the source default only after the real viewport geometry exists.
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::SetViewParams, JE::Editor::Tools::GraphCanvas::ViewParams{});
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::ShowEntireGraph);
        const QTransform layoutReadyControl = graphicsView->transform();
        RecordProperty("layout_ready_control_zoom", QString::number(layoutReadyControl.m11(), 'g', 17).toStdString());
        RecordProperty("initial_transform_dx", QString::number(initialAfterLayout.dx(), 'g', 17).toStdString());
        RecordProperty("initial_transform_dy", QString::number(initialAfterLayout.dy(), 'g', 17).toStdString());
        RecordProperty("control_transform_dx", QString::number(layoutReadyControl.dx(), 'g', 17).toStdString());
        RecordProperty("control_transform_dy", QString::number(layoutReadyControl.dy(), 'g', 17).toStdString());
        ASSERT_GT(layoutReadyControl.m11(), 0.0);
        EXPECT_DOUBLE_EQ(initialAfterLayout.m11(), layoutReadyControl.m11())
            << "Constructor-time framing differs from fitting the same bounds with the final viewport.";
    }

    TEST_F(PassGraphWorkspaceTests, InitialGraphFramingDoesNotRepeatAfterManualCameraAndHideShow)
    {
        AZ::Uuid id;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(id, scene));
        QWidget host;
        host.resize(900, 650);
        auto* layout = new QVBoxLayout(&host);
        auto view = AZStd::make_unique<JE::Editor::Tools::GraphDocumentView>(
            m_session->GetToolId(), id, m_session->EnableGraphDocuments().GetViewSettings(), &host);
        const auto graphicsViews = view->findChildren<QGraphicsView*>();
        ASSERT_EQ(graphicsViews.size(), 1);
        auto* graphicsView = graphicsViews.front();
        JE::Editor::Tools::GraphCanvas::ViewId viewId;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, scene, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
        ASSERT_TRUE(viewId.IsValid());
        FramingNotifications notifications(viewId);
        layout->addWidget(view.get());
        host.show();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        layout->activate();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        ASSERT_GT(notifications.m_centeredCount, 0);

        JE::Editor::Tools::GraphCanvas::ViewParams manualParams;
        manualParams.m_scale = 0.5;
        manualParams.m_anchorPointX = 4000.0f;
        manualParams.m_anchorPointY = 5000.0f;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::SetViewParams, manualParams);
        // SetViewParams can replay the queued first fit; pan after that native restoration.
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::CenterOn, QPointF(4000.0, 5000.0));
        const QSize manualViewport = graphicsView->viewport()->size();
        const QPointF manualCenter = graphicsView->mapToScene(graphicsView->viewport()->rect().center());
        QRectF graphArea;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(graphArea, viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetCompleteArea);
        ASSERT_FALSE(graphArea.contains(manualCenter)) << "Manual camera was not displaced before hide/show.";
        const qreal manualZoom = graphicsView->transform().m11();
        const int centeredBeforeShow = notifications.m_centeredCount;
        host.hide();
        host.show();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        layout->activate();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        ASSERT_EQ(graphicsView->viewport()->size(), manualViewport);
        EXPECT_EQ(notifications.m_centeredCount, centeredBeforeShow);
        EXPECT_EQ(graphicsView->mapToScene(graphicsView->viewport()->rect().center()), manualCenter);
        EXPECT_DOUBLE_EQ(graphicsView->transform().m11(), manualZoom);

        // Prove that the camera oracle detects a fit even when min(fit, original) preserves zoom.
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::ShowEntireGraph);
        EXPECT_GT(notifications.m_centeredCount, centeredBeforeShow);
        EXPECT_NE(graphicsView->mapToScene(graphicsView->viewport()->rect().center()), manualCenter);
    }

    TEST_F(PassGraphWorkspaceTests, InputBeforeInitialFramingPreservesManualCamera)
    {
        AZ::Uuid id;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(id, scene));
        QWidget host;
        host.resize(900, 650);
        auto* layout = new QVBoxLayout(&host);
        auto view = AZStd::make_unique<JE::Editor::Tools::GraphDocumentView>(
            m_session->GetToolId(), id, m_session->EnableGraphDocuments().GetViewSettings(), &host);
        const auto graphicsViews = view->findChildren<QGraphicsView*>();
        ASSERT_EQ(graphicsViews.size(), 1);
        auto* graphicsView = graphicsViews.front();
        JE::Editor::Tools::GraphCanvas::ViewId viewId;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(viewId, scene, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
        ASSERT_TRUE(viewId.IsValid());
        FramingNotifications notifications(viewId);
        layout->addWidget(view.get());
        host.show();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        layout->activate();
        ASSERT_EQ(notifications.m_centeredCount, 0) << "The queued first-fit ran before the input precondition.";
        ASSERT_TRUE(graphicsView->viewport()->isVisible());
        QKeyEvent press(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_Shift, Qt::NoModifier);
        QApplication::sendEvent(graphicsView->viewport(), &press);
        QApplication::sendEvent(graphicsView->viewport(), &release);
        JE::Editor::Tools::GraphCanvas::ViewParams manualParams;
        manualParams.m_scale = 0.5;
        manualParams.m_anchorPointX = 4000.0f;
        manualParams.m_anchorPointY = 5000.0f;
        JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::SetViewParams, manualParams);
        const QSize manualViewport = graphicsView->viewport()->size();
        const QPointF manualCenter = graphicsView->mapToScene(graphicsView->viewport()->rect().center());
        const qreal manualZoom = graphicsView->transform().m11();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        ASSERT_EQ(graphicsView->viewport()->size(), manualViewport);
        EXPECT_EQ(notifications.m_centeredCount, 0);
        EXPECT_EQ(graphicsView->mapToScene(graphicsView->viewport()->rect().center()), manualCenter);
        EXPECT_DOUBLE_EQ(graphicsView->transform().m11(), manualZoom);

        JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(viewId, &JE::Editor::Tools::GraphCanvas::ViewRequests::ShowEntireGraph);
        EXPECT_GT(notifications.m_centeredCount, 0);
        EXPECT_NE(graphicsView->mapToScene(graphicsView->viewport()->rect().center()), manualCenter);
    }

    TEST_F(PassGraphWorkspaceTests, PassEditingSaveReopenAndGlobalUndoNeverQueueAutomaticCompilation)
    {
        auto* registry = AZ::SettingsRegistry::Get();
        ASSERT_NE(registry, nullptr);
        for (const char* key : {"/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen",
                 "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit", "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave"})
        {
            ASSERT_TRUE(registry->Set(key, true));
        }
        const auto id = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(id.IsNull());
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(scene, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
        ASSERT_NE(graph, nullptr);
        ASSERT_TRUE(scene.IsValid());
        bool success = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
        ASSERT_TRUE(success);
        auto node = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphInputNode>(graph, graph->GetContext()->GetDataType<float>());
        node->GetSlot("defaultValue")->SetValue(0.375f);
        AZ::Vector2 position(24.0f, 48.0f);
        JE::Editor::Tools::GraphCanvas::NodeId canvasNode;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasNode, scene,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, node, position);
        ASSERT_TRUE(canvasNode.IsValid());
        const auto logicalId = node->GetId();
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        ASSERT_TRUE(success);
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        m_application->GetUndoStack()->Undo();
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        EXPECT_EQ(graph->GetNodeCount(), 0);
        m_application->GetUndoStack()->Redo();
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph->GetNode(logicalId), nullptr);
        EXPECT_FLOAT_EQ(graph->GetNode(logicalId)->GetSlot("defaultValue")->GetValue<float>(), 0.375f);
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::Save);
        ASSERT_TRUE(success);
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::Reopen);
        ASSERT_TRUE(success);
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph->GetNode(logicalId), nullptr);
        EXPECT_FLOAT_EQ(graph->GetNode(logicalId)->GetSlot("defaultValue")->GetValue<float>(), 0.375f);

        class CompilationNotifications final : public JE::Editor::Tools::GraphDocumentNotificationBus::Handler
        {
        public:
            CompilationNotifications() { BusConnect(JE::Editor::MaterialWorkspaceSession::GetToolId()); }
            ~CompilationNotifications() override { BusDisconnect(); }
            void OnCompileGraphStarted(const AZ::Uuid&) override { ++m_count; }
            void OnCompileGraphCompleted(const AZ::Uuid&) override { ++m_count; }
            void OnCompileGraphFailed(const AZ::Uuid&) override { ++m_count; }
            int m_count = 0;
        } notifications;
        JE::Editor::Tools::GraphDocumentRequestBus::Event(id, &JE::Editor::Tools::GraphDocumentRequests::QueueCompileGraph);
        bool queued = true;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(queued, id, &JE::Editor::Tools::GraphDocumentRequests::IsCompileGraphQueued);
        EXPECT_FALSE(queued);
        const auto state = m_session->GetCompileState(id);
        // Explicit requests for an owned Pass graph are covered below; unowned requests still cannot submit a job.
        EXPECT_FALSE(m_session->RequestCompile(AZ::Uuid::CreateNull()));
        EXPECT_EQ(m_session->GetCompileState(id), state);
        for (int frame = 0; frame < 3; ++frame)
        {
            AZ::SystemTickBus::Broadcast(&AZ::SystemTickBus::Events::OnSystemTick);
            AZ::SystemTickBus::ExecuteQueuedEvents();
        }
        EXPECT_EQ(notifications.m_count, 0);
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(material.IsNull());
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(queued, material, &JE::Editor::Tools::GraphDocumentRequests::IsCompileGraphQueued);
        EXPECT_TRUE(queued); // Control: material auto-compilation remains enabled; do not submit its job in this fixture.
    }

    TEST_F(PassGraphWorkspaceTests, CloseMenuClosesInactiveCleanDocumentByUuidWithoutSelectingIt)
    {
        const auto primary = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        const auto secondary = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(primary.IsNull());
        ASSERT_FALSE(secondary.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        ASSERT_EQ(m_session->GetActiveDocumentId(), secondary);
        ASSERT_NO_FATAL_FAILURE(RequestDocumentClose(widget, primary));
        EXPECT_FALSE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(primary));
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(secondary));
        EXPECT_EQ(m_session->GetActiveDocumentId(), secondary);
        auto* selector = widget.findChild<QComboBox*>(QStringLiteral("GraphDocuments"));
        ASSERT_NE(selector, nullptr);
        EXPECT_EQ(selector->count(), 1);
        EXPECT_EQ(selector->currentData().toString(), QString::fromUtf8(secondary.ToString<AZStd::string>().c_str()));
    }

    TEST_F(PassGraphWorkspaceTests, CloseMenuCancelPreservesInactiveDirtyDocumentAndDiscardLeavesFileUnchanged)
    {
        AZ::Uuid primary;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(primary, scene));
        AZStd::string path;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            path, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
        const auto before = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(before.IsSuccess());
        const auto secondary = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(secondary.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        ASSERT_NO_FATAL_FAILURE(RequestDocumentClose(widget, primary, QMessageBox::Cancel));
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(primary));
        bool modified = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            modified, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
        EXPECT_TRUE(modified);
        EXPECT_EQ(m_session->GetActiveDocumentId(), secondary);
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, primary, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph, nullptr);
        EXPECT_EQ(graph->GetNodeCount(), 1);
        ASSERT_NO_FATAL_FAILURE(RequestDocumentClose(widget, primary, QMessageBox::Discard));
        EXPECT_FALSE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(primary));
        EXPECT_EQ(m_session->GetActiveDocumentId(), secondary);
        const auto after = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), before.GetValue());
    }

    TEST_F(PassGraphWorkspaceTests, CloseMenuSaveWritesTheInactiveTargetBeforeClosingIt)
    {
        AZ::Uuid primary;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(primary, scene));
        AZStd::string path;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            path, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
        const auto before = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(before.IsSuccess());
        const auto secondary = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(secondary.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        ASSERT_NO_FATAL_FAILURE(RequestDocumentClose(widget, primary, QMessageBox::Save));
        EXPECT_FALSE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(primary));
        EXPECT_EQ(m_session->GetActiveDocumentId(), secondary);
        const auto after = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_NE(after.GetValue(), before.GetValue());
        const auto reopened = m_session->OpenDocument(path);
        ASSERT_FALSE(reopened.IsNull());
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, reopened, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph, nullptr);
        EXPECT_EQ(graph->GetNodeCount(), 1);
    }

    TEST_F(PassGraphWorkspaceTests, CloseMenuFailedSavePreservesInactiveDocumentAndHistory)
    {
        AZ::Uuid primary;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(primary, scene));
        AZStd::string path;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            path, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::GetAbsolutePath);
        const auto before = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(before.IsSuccess());
        QFile file(QString::fromUtf8(path.c_str()));
        const auto permissions = file.permissions();
        const auto restorePermissions = qScopeGuard([&file, permissions]()
        {
            EXPECT_TRUE(file.setPermissions(permissions));
        });
        // This is only the test fixture's temporary source file, never a checkout/SDK path or ACL.
        ASSERT_TRUE(file.setPermissions(permissions & ~(QFileDevice::WriteOwner | QFileDevice::WriteUser |
            QFileDevice::WriteGroup | QFileDevice::WriteOther)));
        // The real DocumentSystem rejects this before GraphDocument's writer/atomic-save behavior.
        ASSERT_FALSE(QFileInfo(file).isWritable());
        auto* registry = AZ::SettingsRegistry::Get();
        ASSERT_NE(registry, nullptr);
        ASSERT_TRUE(registry->Set("/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/DisplayErrorMessageDialogs", false));
        const auto secondary = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(secondary.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        const bool couldUndo = m_application->GetUndoStack()->CanUndo();
        AZ_TEST_START_TRACE_SUPPRESSION;
        RequestDocumentClose(widget, primary, QMessageBox::Save);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // DocumentSystem's real "could not be overwritten" rejection.
        ASSERT_FALSE(HasFatalFailure());
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(primary));
        EXPECT_EQ(m_session->GetActiveDocumentId(), secondary);
        EXPECT_EQ(m_application->GetUndoStack()->CanUndo(), couldUndo);
        bool modified = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            modified, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::IsModified);
        EXPECT_TRUE(modified);
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, primary, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph, nullptr);
        EXPECT_EQ(graph->GetNodeCount(), 1);
        const auto after = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), before.GetValue());
    }

    TEST_F(PassGraphWorkspaceTests, CloseMenuRejectedHistoryCommitKeepsInactiveDocumentOpen)
    {
        auto& environment = m_session->EnableGraphDocuments();
        const auto context = environment.GetGraphContext();
        auto rejectCommit = AZStd::make_shared<bool>(false);
        auto rejectedCommits = AZStd::make_shared<int>(0);
        auto policy = JE::Editor::CreateMaterialCanvasDocumentHistoryPolicy();
        const auto commit = policy.m_commit;
        policy.m_commit = [rejectCommit, rejectedCommits, commit](const AZ::Uuid& id, const AZStd::string& path,
            const JE::Editor::Tools::DocumentHistoryState& before, const JE::Editor::Tools::DocumentHistoryState& after)
        {
            if (*rejectCommit)
            {
                ++*rejectedCommits;
                return false;
            }
            return commit(id, path, before, after);
        };
        // The existing document factory/policy seam supplies a real GraphDocument, not a fake Close/EndEdit bus.
        auto type = JE::Editor::Tools::GraphDocument::BuildDocumentTypeInfo(
            "Commit Failure Material Graph", {"materialgraph"}, {}, {}, context,
            [toolId = m_session->GetToolId()]()
            {
                return AZStd::make_shared<JE::Editor::Tools::GraphCompiler>(toolId);
            });
        const auto factory = type.m_documentFactoryCallback;
        type.m_documentFactoryCallback = [factory, policy](const AZ::Crc32& toolId,
            const JE::Editor::Tools::DocumentTypeInfo& info) -> JE::Editor::Tools::AtomToolsDocumentRequests*
        {
            auto* document = factory(toolId, info);
            if (document && !document->SetHistoryPolicy(policy))
            {
                delete document;
                return nullptr;
            }
            return document;
        };
        m_session->RegisterGraphDocumentTypes({type});
        const auto path = m_directory->Resolve("source/commit-rejected.materialgraph").Native();
        JE::Editor::Tools::GraphModel::Graph initial(context);
        ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, path).IsSuccess());
        AZ::Uuid primary;
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(
            primary, m_session->GetToolId(), &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CreateDocumentFromType, type);
        ASSERT_FALSE(primary.IsNull());
        bool result = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            result, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::Open, path);
        ASSERT_TRUE(result);
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, primary, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            scene, primary, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
        ASSERT_NE(graph, nullptr);
        ASSERT_TRUE(scene.IsValid());
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            result, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
        ASSERT_TRUE(result);
        auto node = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphInputNode>(graph, context->GetDataType<float>());
        node->GetSlot("defaultValue")->SetValue(0.25f);
        JE::Editor::Tools::GraphCanvas::NodeId canvasNode;
        AZ::Vector2 position(24.0f, 48.0f);
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(
            canvasNode, scene, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, node, position);
        ASSERT_TRUE(canvasNode.IsValid());
        const auto nodeId = node->GetId();
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            result, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        ASSERT_TRUE(result);
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(
            result, m_session->GetToolId(), &JE::Editor::Tools::AtomToolsDocumentSystemRequests::SaveDocument, primary);
        ASSERT_TRUE(result);
        const auto before = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(before.IsSuccess());
        const auto secondary = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(secondary.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        ASSERT_EQ(m_session->GetActiveDocumentId(), secondary);
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            result, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
        ASSERT_TRUE(result);
        node->GetSlot("defaultValue")->SetValue(0.75f);
        *rejectCommit = true;
        ASSERT_NO_FATAL_FAILURE(RequestDocumentClose(widget, primary)); // No confirmation is permitted after EndEdit fails.
        EXPECT_EQ(*rejectedCommits, 1);
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(primary));
        bool open = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            open, primary, &JE::Editor::Tools::AtomToolsDocumentRequests::IsOpen);
        EXPECT_TRUE(open);
        EXPECT_EQ(m_session->GetActiveDocumentId(), secondary);
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(secondary));
        // Reacquire after real AddUndoRedoHistory rollback; the old graph/node can have been replaced.
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, primary, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph, nullptr);
        ASSERT_NE(graph->GetNode(nodeId), nullptr);
        EXPECT_FLOAT_EQ(graph->GetNode(nodeId)->GetSlot("defaultValue")->GetValue<float>(), 0.25f);
        const auto after = AZ::Utils::ReadFile(path);
        ASSERT_TRUE(after.IsSuccess());
        EXPECT_EQ(after.GetValue(), before.GetValue());
    }

    TEST_F(PassGraphWorkspaceTests, InspectorPreviewSplitterReportsConstraintsAndKeepsManualPartitionAfterFirstShow)
    {
        ASSERT_EQ(JE::Render::RPI::RPISystemInterface::Get(), nullptr);
        AZ::Uuid id;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(id, scene));
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        widget.resize(1920, 1032);
        widget.show();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        auto* splitter = widget.findChild<QSplitter*>(QStringLiteral("MaterialGraphPropertiesSplitter"));
        auto* preview = widget.findChild<QWidget*>(QStringLiteral("MaterialGraphPreviewHost"));
        ASSERT_NE(splitter, nullptr);
        ASSERT_NE(preview, nullptr);
        ASSERT_EQ(splitter->count(), 2);
        auto* inspector = splitter->widget(0);
        ASSERT_NE(inspector, nullptr);
        // Expose the existing lazy host only to measure the real Inspector/splitter constraints.
        // No viewport is fabricated: this is not the Vulkan/RPI resize acceptance gate.
        preview->show();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        const auto initial = splitter->sizes();
        ASSERT_EQ(initial.size(), 2);
        int minimum = 0, maximum = 0;
        splitter->getRange(1, &minimum, &maximum);
        auto* handle = splitter->handle(1);
        ASSERT_NE(handle, nullptr);
        RecordProperty("SplitterInitialInspectorHeight", initial[0]);
        RecordProperty("SplitterInitialPreviewHeight", initial[1]);
        RecordProperty("SplitterRangeMinimum", minimum);
        RecordProperty("SplitterRangeMaximum", maximum);
        RecordProperty("SplitterHandleY", handle->geometry().y());
        RecordProperty("SplitterHandleHeight", handle->height());
        RecordProperty("SplitterHandleVisible", handle->isVisible() ? 1 : 0);
        RecordProperty("SplitterHandleEnabled", handle->isEnabled() ? 1 : 0);
        RecordProperty("InspectorMinimumHeight", inspector->minimumHeight());
        RecordProperty("InspectorMinimumHintHeight", inspector->minimumSizeHint().height());
        RecordProperty("InspectorHintHeight", inspector->sizeHint().height());
        RecordProperty("PreviewMinimumHintHeight", preview->minimumSizeHint().height());
        EXPECT_EQ(widget.findChild<QWidget*>(QStringLiteral("MaterialGraphPreviewViewport")), nullptr);
        EXPECT_GT(initial[0], 0);
        EXPECT_GT(initial[1], 0);
        EXPECT_TRUE(handle->isVisible());
        EXPECT_TRUE(handle->isEnabled());
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(
            graph, id, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph, nullptr);
        ASSERT_EQ(graph->GetNodeCount(), 1);
        JE::Editor::Tools::GraphCanvas::NodeId nodeId;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(
            nodeId, scene, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeIdByNode,
            graph->GetNodes().begin()->second);
        ASSERT_TRUE(nodeId.IsValid());
        JE::Editor::Tools::GraphCanvas::SceneMemberUIRequestBus::Event(
            nodeId, &JE::Editor::Tools::GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        splitter->getRange(1, &minimum, &maximum);
        RecordProperty("PopulatedInspectorMinimumHintHeight", inspector->minimumSizeHint().height());
        RecordProperty("PopulatedSplitterRangeMinimum", minimum);
        RecordProperty("PopulatedSplitterRangeMaximum", maximum);
        const auto populated = splitter->sizes();
        RecordProperty("PopulatedInspectorHeight", populated[0]);
        RecordProperty("PopulatedPreviewHeight", populated[1]);
        const int total = populated[0] + populated[1];
        splitter->setSizes({total * 2 / 3, total - total * 2 / 3});
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        const auto manual = splitter->sizes();
        RecordProperty("SplitterRequestedInspectorHeight", total * 2 / 3);
        RecordProperty("SplitterActualInspectorHeight", manual[0]);
        preview->hide();
        preview->show();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        EXPECT_EQ(splitter->sizes(), manual);
    }

    TEST_F(PassGraphWorkspaceTests, MaterialSettingsOpeningDoesNotPersistAbsentDefaults)
    {
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(material.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        auto* registry = AZ::SettingsRegistry::Get();
        ASSERT_NE(registry, nullptr);
        const char* keys[] = {
            "/O3DE/Atom/MaterialCanvas/ForceDeleteGeneratedFiles",
            "/O3DE/Atom/MaterialCanvas/ForceClearAssetFingerprints",
            "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen",
            "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave",
            "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit",
            "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphStarted",
            "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphFailed",
            "/O3DE/AtomToolsFramework/GraphCompiler/EnableLogging",
            "/O3DE/AtomToolsFramework/DynamicNode/EnablePropertyEditingOnNodeUI",
            "/O3DE/AtomToolsFramework/GraphCompiler/QueueGraphCompileIntervalMs",
            "/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart",
            "/O3DE/Preferences/JE/Materials/EnableFasterShaderBuilds"};
        for (const char* key : keys) { registry->Remove(key); }
        for (bool accept : {false, true})
        {
            InspectMaterialSettings(widget, [&](auto& dialog, auto& group)
            {
                EXPECT_EQ(dialog.size(), QSize(800, 400));
                EXPECT_LT(dialog.minimumWidth(), dialog.maximumWidth());
                EXPECT_LT(dialog.minimumHeight(), dialog.maximumHeight());
                ASSERT_EQ(group.m_properties.size(), 12);
                for (size_t i = 0; i < AZ_ARRAY_SIZE(keys); ++i)
                {
                    const auto& property = group.m_properties[i];
                    EXPECT_STREQ(property.GetId().GetCStr(), keys[i]);
                    if (i == 9)
                    {
                        EXPECT_EQ(AZStd::any_cast<AZ::s32>(property.GetValue()), 500);
                        EXPECT_EQ(AZStd::any_cast<AZ::s32>(property.GetConfig().m_min), 0);
                        EXPECT_EQ(AZStd::any_cast<AZ::s32>(property.GetConfig().m_max), 1000);
                    }
                    else { EXPECT_EQ(AZStd::any_cast<bool>(property.GetValue()), i != 0 && i != 1 && i != 7 && i != 11); }
                    EXPECT_EQ(registry->GetType(keys[i]), AZ::SettingsRegistryInterface::Type::NoType);
                }
            }, accept);
            for (const char* key : keys) { EXPECT_EQ(registry->GetType(key), AZ::SettingsRegistryInterface::Type::NoType); }
        }
    }

    TEST_F(PassGraphWorkspaceTests, FasterShaderBuildPreferenceRequestsProjectSaveOnlyAfterAnActualEdit)
    {
        class SaveRequests final : public AzToolsFramework::EditorSettingsAPIBus::Handler
        {
        public:
            ~SaveRequests() override { BusDisconnect(); }
            AZStd::vector<AZStd::string> BuildSettingsList() override { return {}; }
            SettingOutcome GetValue(AZStd::string_view) override { return AZ::Failure(AZStd::string("Unused")); }
            SettingOutcome SetValue(AZStd::string_view, const AZStd::any&) override { return AZ::Failure(AZStd::string("Unused")); }
            AzToolsFramework::ConsoleColorTheme GetConsoleColorTheme() const override { return AzToolsFramework::ConsoleColorTheme::Light; }
            AZ::u64 GetMaxNumberOfItemsShownInSearchView() const override { return 0; }
            void SaveSettingsRegistryFile() override
            {
                bool requested = false;
                EXPECT_TRUE(AZ::SettingsRegistry::Get()->Get(requested, "/O3DE/Preferences/JE/Materials/EnableFasterShaderBuilds"));
                values.push_back(requested);
            }
            AZStd::vector<bool> values;
        } saveRequests;
        ASSERT_FALSE(AzToolsFramework::EditorSettingsAPIBus::HasHandlers());
        saveRequests.BusConnect();
        // Handler destructor disconnects; this fake observes the void save request, never claims a disk write.
        auto* registry = AZ::SettingsRegistry::Get();
        ASSERT_NE(registry, nullptr);
        constexpr const char* requestedKey = "/O3DE/Preferences/JE/Materials/EnableFasterShaderBuilds";
        constexpr const char* buildVariantsKey = "/O3DE/Atom/Shaders/BuildVariants";
        constexpr const char* unrelatedKey = "/O3DE/Preferences/Existing/Unrelated";
        registry->Remove(requestedKey);
        registry->Set(buildVariantsKey, false);
        registry->Set(unrelatedKey, AZ::s64{73});
        ASSERT_FALSE(OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material).IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        for (bool requested : {false, true, false})
        {
            InspectMaterialSettings(widget, [&](auto&, auto& group)
            {
                auto property = AZStd::find_if(group.m_properties.begin(), group.m_properties.end(), [](const auto& entry)
                {
                    return entry.GetId() == AZ::Name("/O3DE/Preferences/JE/Materials/EnableFasterShaderBuilds");
                });
                ASSERT_NE(property, group.m_properties.end());
                ASSERT_TRUE(property->GetConfig().m_dataChangeCallback);
                property->SetValue(requested);
                property->GetConfig().m_dataChangeCallback(property->GetValue());
                EXPECT_EQ(AZStd::any_cast<bool>(property->GetValue()), requested);
            });
            if (saveRequests.values.empty())
            {
                EXPECT_EQ(registry->GetType(requestedKey), AZ::SettingsRegistryInterface::Type::NoType);
            }
            else
            {
                bool stored = !requested;
                ASSERT_TRUE(registry->Get(stored, requestedKey));
                EXPECT_EQ(stored, requested);
            }
            bool buildVariants = true;
            ASSERT_TRUE(registry->Get(buildVariants, buildVariantsKey));
            EXPECT_FALSE(buildVariants);
            AZ::s64 unrelated = 0;
            ASSERT_TRUE(registry->Get(unrelated, unrelatedKey));
            EXPECT_EQ(unrelated, 73);
        }
        ASSERT_EQ(saveRequests.values.size(), 2);
        EXPECT_TRUE(saveRequests.values[0]);
        EXPECT_FALSE(saveRequests.values[1]);
    }

    TEST_F(PassGraphWorkspaceTests, DefaultGraphWorkspacePreferenceUsesLiveSettingsBinding)
    {
        ASSERT_FALSE(OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material).IsNull());
        {
            JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
            InspectMaterialSettings(widget, [](auto&, auto& group)
            {
                auto found = AZStd::find_if(group.m_properties.begin(), group.m_properties.end(), [](const auto& property)
                {
                    return property.GetId() == AZ::Name("/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart");
                });
                ASSERT_NE(found, group.m_properties.end());
                ASSERT_TRUE(found->GetConfig().m_dataChangeCallback);
                found->SetValue(false);
                found->GetConfig().m_dataChangeCallback(found->GetValue());
            });
        }
        bool enabled = true;
        ASSERT_TRUE(AZ::SettingsRegistry::Get()->Get(enabled, "/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart"));
        ASSERT_FALSE(enabled);
        bool closed = false;
        JE::Editor::Tools::AtomToolsDocumentSystemRequestBus::EventResult(closed, m_session->GetToolId(),
            &JE::Editor::Tools::AtomToolsDocumentSystemRequests::CloseAllDocuments);
        ASSERT_TRUE(closed);
        m_session->PrepareGraphWorkspace({});
        EXPECT_TRUE(m_session->GetDocumentIds().empty());
    }

    TEST_F(PassGraphWorkspaceTests, MaterialSettingsLiveCompileOptionSurvivesDialogDismissal)
    {
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(material.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        InspectMaterialSettings(widget, [](auto&, auto& group)
        {
            auto found = AZStd::find_if(group.m_properties.begin(), group.m_properties.end(), [](const auto& property)
            {
                return property.GetId() == AZ::Name("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit");
            });
            ASSERT_NE(found, group.m_properties.end());
            ASSERT_TRUE(found->GetConfig().m_dataChangeCallback);
            found->SetValue(true);
            found->GetConfig().m_dataChangeCallback(found->GetValue());
        });
        bool enabled = false;
        ASSERT_TRUE(AZ::SettingsRegistry::Get()->Get(enabled, "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit"));
        EXPECT_TRUE(enabled) << "Legacy settings edits apply live, including when Esc/X rejects the dialog";
        bool queued = true;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(queued, material,
            &JE::Editor::Tools::GraphDocumentRequests::IsCompileGraphQueued);
        ASSERT_FALSE(queued);
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(graph, material, &JE::Editor::Tools::GraphDocumentRequests::GetGraph);
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(scene, material, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
        ASSERT_NE(graph, nullptr);
        bool edited = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(edited, material,
            &JE::Editor::Tools::AtomToolsDocumentRequests::BeginEdit);
        ASSERT_TRUE(edited);
        JE::Editor::Tools::GraphModel::NodePtr node =
            AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphInputNode>(graph, graph->GetContext()->GetDataType<float>());
        AZ::Vector2 position(24.0f, 48.0f);
        JE::Editor::Tools::GraphCanvas::NodeId canvasNode;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasNode, scene,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, node, position);
        ASSERT_TRUE(canvasNode.IsValid());
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(edited, material,
            &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        ASSERT_TRUE(edited);
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(queued, material,
            &JE::Editor::Tools::GraphDocumentRequests::IsCompileGraphQueued);
        EXPECT_TRUE(queued) << "The exposed option must reach the existing GraphDocument consumer";
    }

    TEST_F(PassGraphWorkspaceTests, MaterialSettingsAreHiddenAndDisabledForPassDocuments)
    {
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(material.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        auto* settings = widget.findChild<QPushButton*>(QStringLiteral("MaterialGraphSettings"));
        ASSERT_NE(settings, nullptr);
        EXPECT_FALSE(settings->isHidden());
        EXPECT_TRUE(settings->isEnabled());
        const auto pass = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(pass.IsNull());
        EXPECT_TRUE(settings->isHidden());
        EXPECT_FALSE(settings->isEnabled());
        const auto node = m_session->OpenDocument(m_directory->Resolve("source/pass.passgraphnode").Native());
        ASSERT_FALSE(node.IsNull());
        EXPECT_TRUE(settings->isHidden());
        EXPECT_FALSE(settings->isEnabled());
        ASSERT_TRUE(m_session->SelectDocument(material));
        EXPECT_FALSE(settings->isHidden());
        EXPECT_TRUE(settings->isEnabled());
    }

    TEST_F(PassGraphWorkspaceTests, MaterialSettingsDialogClosesBeforeSessionDestruction)
    {
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(material.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        QPointer<JE::Editor::Tools::SettingsDialog> observed;
        InspectMaterialSettings(widget, [&](auto& dialog, auto&)
        {
            observed = &dialog;
            EXPECT_TRUE(dialog.isVisible());
            m_session.reset();
            EXPECT_FALSE(dialog.isVisible()) << "AboutToClose must reject the dialog before helper cleanup";
        });
        EXPECT_TRUE(observed.isNull());
        EXPECT_EQ(JE::Render::RPI::RPISystemInterface::Get(), nullptr);
    }

    TEST_F(PassGraphWorkspaceTests, GraphViewSettingsShareEnvironmentAndRefreshCachedZoomAfterDialogDismissal)
    {
        AZ::Uuid material;
        JE::Editor::Tools::GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(material, scene));
        const auto settings = m_session->EnableGraphDocuments().GetViewSettings();
        settings->m_maxZoom = 2.0f;
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        JE::Editor::Tools::GraphCanvas::ViewId view;
        JE::Editor::Tools::GraphCanvas::SceneRequestBus::EventResult(
            view, scene, &JE::Editor::Tools::GraphCanvas::SceneRequests::GetViewId);
        ASSERT_TRUE(view.IsValid());
        auto* registry = AZ::SettingsRegistry::Get();
        ASSERT_NE(registry, nullptr);
        constexpr const char* settingsPath = "/O3DE/Atom/GraphView/ViewSettings";
        double previousLimit = 2.0;
        for (bool accept : {false, true})
        {
            // Use the real view's cached maximum before changing the shared settings object.
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::Event(
                view, &JE::Editor::Tools::GraphCanvas::ViewRequests::DisplayArea, QRectF(0.0, 0.0, 1.0, 1.0));
            qreal zoom = 0.0;
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(
                zoom, view, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetZoomLevel);
            ASSERT_DOUBLE_EQ(zoom, previousLimit);
            const float nextLimit = accept ? 1.0f : 1.25f;
            bool edited = false;
            InspectMaterialSettings(widget, [&](auto& dialog, auto&)
            {
                auto* exposed = FindMaterialGraphViewSettings(dialog);
                ASSERT_EQ(exposed, settings.get()) << "Do not edit a detached settings copy";
                exposed->m_maxZoom = nextLimit;
                EXPECT_FLOAT_EQ(settings->m_maxZoom, nextLimit);
                JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(
                    zoom, view, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetZoomLevel);
                EXPECT_DOUBLE_EQ(zoom, previousLimit) << "The view cache must change through the production close/publication path";
                edited = true;
            }, accept);
            ASSERT_TRUE(edited);
            JE::Editor::Tools::GraphCanvas::ViewRequestBus::EventResult(
                zoom, view, &JE::Editor::Tools::GraphCanvas::ViewRequests::GetZoomLevel);
            EXPECT_DOUBLE_EQ(zoom, nextLimit) << "Closing Settings must publish and refresh the existing view's cached maximum";
            JE::Editor::Tools::GraphViewSettingsPtr stored;
            ASSERT_TRUE(registry->GetObject(stored, settingsPath));
            ASSERT_NE(stored, nullptr);
            EXPECT_FLOAT_EQ(stored->m_maxZoom, nextLimit) << "Esc/X preserves the same live edits as OK";
            bool reopened = false;
            InspectMaterialSettings(widget, [&](auto& dialog, auto&)
            {
                auto* exposed = FindMaterialGraphViewSettings(dialog);
                ASSERT_EQ(exposed, settings.get());
                EXPECT_FLOAT_EQ(exposed->m_maxZoom, nextLimit);
                reopened = true;
            });
            ASSERT_TRUE(reopened);
            previousLimit = nextLimit;
        }
    }

    TEST_F(PassGraphWorkspaceTests, GraphViewSettingsSessionClosePublishesBeforeModalAndWorkspaceCleanup)
    {
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(material.IsNull());
        auto* settings = m_session->EnableGraphDocuments().GetViewSettings().get();
        auto widget = AZStd::make_unique<JE::Editor::MaterialGraphWorkspaceWidget>(*m_session);
        auto* registry = AZ::SettingsRegistry::Get();
        ASSERT_NE(registry, nullptr);
        constexpr const char* settingsPath = "/O3DE/Atom/GraphView/ViewSettings";
        registry->Remove(settingsPath);
        bool closing = false, closed = false, publishedDuringClose = false, publishedAfterClose = false;
        [[maybe_unused]] auto notifier = registry->RegisterNotifier([&](const AZ::SettingsRegistryInterface::NotifyEventArgs& args)
        {
            if (args.m_jsonKeyPath == settingsPath)
            {
                publishedDuringClose |= closing;
                publishedAfterClose |= closed;
            }
        });
        bool inspected = false;
        QPointer<JE::Editor::Tools::SettingsDialog> observed;
        InspectMaterialSettings(*widget, [&](auto& dialog, auto&)
        {
            observed = &dialog;
            auto* exposed = FindMaterialGraphViewSettings(dialog);
            ASSERT_EQ(exposed, settings);
            exposed->m_maxZoom = 1.25f;
            EXPECT_TRUE(dialog.isVisible());
            closing = true;
            m_session.reset();
            closing = false;
            closed = true;
            EXPECT_FALSE(dialog.isVisible()) << "Session closure must reject before helper cleanup";
            widget.reset();
            EXPECT_TRUE(observed.isNull()) << "Destroying the Workspace must release its owned modal safely";
            inspected = true;
        });
        ASSERT_TRUE(inspected);
        EXPECT_TRUE(publishedDuringClose) << "The existing Environment destructor must preserve the edited settings";
        EXPECT_FALSE(publishedAfterClose) << "The modal must not publish again after its session was destroyed";
        JE::Editor::Tools::GraphViewSettingsPtr stored;
        ASSERT_TRUE(registry->GetObject(stored, settingsPath));
        ASSERT_NE(stored, nullptr);
        EXPECT_FLOAT_EQ(stored->m_maxZoom, 1.25f);
        EXPECT_TRUE(observed.isNull());
    }

    TEST_F(PassGraphWorkspaceTests, MaterialPreviewRemainsLazyAcrossGraphSwitchAndCloseWithoutRpi)
    {
        ASSERT_EQ(JE::Render::RPI::RPISystemInterface::Get(), nullptr);
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(material.IsNull());
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        widget.show();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        auto* preview = widget.findChild<QWidget*>(QStringLiteral("MaterialGraphPreviewHost"));
        ASSERT_NE(preview, nullptr);
        EXPECT_TRUE(preview->isHidden());
        EXPECT_EQ(widget.findChild<QWidget*>(QStringLiteral("MaterialGraphPreviewViewport")), nullptr);

        const auto pass = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(pass.IsNull());
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(m_session->GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphCompleted, material);
        EXPECT_TRUE(preview->isHidden());
        EXPECT_EQ(widget.findChild<QWidget*>(QStringLiteral("MaterialGraphPreviewViewport")), nullptr);
        widget.close();
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(m_session->GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphCompleted, pass);
        bool open = false;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(open, material,
            &JE::Editor::Tools::AtomToolsDocumentRequests::IsOpen);
        EXPECT_TRUE(open);
        EXPECT_TRUE(preview->isHidden());
        EXPECT_EQ(widget.findChild<QWidget*>(QStringLiteral("MaterialGraphPreviewViewport")), nullptr);
        m_session.reset();
        EXPECT_EQ(widget.findChild<QWidget*>(QStringLiteral("MaterialGraphPreviewHost")), nullptr);
        JE::Editor::Tools::GraphDocumentNotificationBus::Event(JE::Editor::MaterialWorkspaceSession::GetToolId(),
            &JE::Editor::Tools::GraphDocumentNotifications::OnCompileGraphCompleted, material);
        EXPECT_EQ(widget.findChild<QWidget*>(QStringLiteral("MaterialGraphPreviewViewport")), nullptr);
        EXPECT_EQ(JE::Render::RPI::RPISystemInterface::Get(), nullptr);
    }
    TEST_F(PassGraphWorkspaceTests, SharedPaneSwitchesPaletteAndActionsWithoutClosingEitherDocument)
    {
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        const auto pass = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(pass.IsNull());
        auto* paletteWidget = widget.findChild<QWidget*>(QStringLiteral("GraphNodePalette"));
        ASSERT_NE(paletteWidget, nullptr);
        auto* palette = static_cast<JE::Editor::Tools::GraphCanvas::NodePaletteWidget*>(paletteWidget->qt_metacast(JE::Editor::Tools::GraphCanvas::NodePaletteWidget::staticMetaObject.className()));
        ASSERT_NE(palette, nullptr);
        EXPECT_NE(palette->FindItemWithName(QStringLiteral("Only Pass")), nullptr);
        EXPECT_EQ(palette->FindItemWithName(QStringLiteral("Only Material")), nullptr);
        JE::Editor::Tools::GraphCanvas::NodePaletteWidget contextPalette(nullptr);
        JE::Editor::Tools::GraphCanvas::NodePaletteConfig contextSettings;
        contextSettings.m_editorId = m_session->GetToolId();
        contextSettings.m_saveIdentifier = "PassGraphContextTest";
        contextSettings.m_rootTreeItem = m_session->EnableGraphDocuments().GetViewSettings()->m_createNodeTreeItemsFn(m_session->GetToolId());
        contextPalette.SetupNodePalette(contextSettings);
        EXPECT_NE(contextPalette.FindItemWithName(QStringLiteral("Only Pass")), nullptr);
        EXPECT_EQ(contextPalette.FindItemWithName(QStringLiteral("Only Material")), nullptr);
        auto* compile = widget.findChild<QPushButton*>(QStringLiteral("CompileGraph"));
        auto* openMaterial = widget.findChild<QPushButton*>(QStringLiteral("OpenGeneratedMaterial"));
        ASSERT_NE(compile, nullptr);
        ASSERT_NE(openMaterial, nullptr);
        EXPECT_TRUE(compile->isHidden());
        EXPECT_FALSE(compile->isEnabled());
        EXPECT_TRUE(openMaterial->isHidden());
        const auto material = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(material.IsNull());
        EXPECT_NE(palette->FindItemWithName(QStringLiteral("Only Material")), nullptr);
        EXPECT_EQ(palette->FindItemWithName(QStringLiteral("Only Pass")), nullptr);
        contextPalette.ResetModel(m_session->EnableGraphDocuments().GetViewSettings()->m_createNodeTreeItemsFn(m_session->GetToolId()));
        EXPECT_NE(contextPalette.FindItemWithName(QStringLiteral("Only Material")), nullptr);
        EXPECT_EQ(contextPalette.FindItemWithName(QStringLiteral("Only Pass")), nullptr);
        EXPECT_FALSE(compile->isHidden());
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(pass));
        EXPECT_TRUE(JE::Editor::Tools::AtomToolsDocumentRequestBus::HasHandlers(material));
        EXPECT_EQ(m_session->GetGraphDocumentTypes().size(), 5);
        const auto node = m_session->OpenDocument(m_directory->Resolve("source/pass.passgraphnode").Native());
        ASSERT_FALSE(node.IsNull());
        JE::Editor::Tools::DocumentObjectInfoVector objects;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(objects, node,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetObjectInfo);
        ASSERT_EQ(objects.size(), 1);
        EXPECT_EQ(objects.front().m_objectType, azrtti_typeid<JE::Editor::Tools::DynamicNodeConfig>());
        JE::Editor::Tools::GraphCanvas::GraphId nodeScene;
        JE::Editor::Tools::GraphDocumentRequestBus::EventResult(nodeScene, node, &JE::Editor::Tools::GraphDocumentRequests::GetGraphId);
        EXPECT_FALSE(nodeScene.IsValid());
        EXPECT_TRUE(compile->isHidden());
        EXPECT_NE(palette->FindItemWithName(QStringLiteral("Only Pass")), nullptr);
    }

    TEST_F(PassGraphWorkspaceTests, PassNodeConfigurationKeepsNoticeCompactAndInspectorUsable)
    {
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        const auto node = m_session->OpenDocument(m_directory->Resolve("source/pass.passgraphnode").Native());
        ASSERT_FALSE(node.IsNull());
        JE::Editor::Tools::DocumentObjectInfoVector objects;
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(objects, node,
            &JE::Editor::Tools::AtomToolsDocumentRequests::GetObjectInfo);
        ASSERT_EQ(objects.size(), 1);
        EXPECT_EQ(objects.front().m_objectType, azrtti_typeid<JE::Editor::Tools::DynamicNodeConfig>());

        auto* notice = widget.findChild<QLabel*>(QStringLiteral("GraphCompileStatus"));
        ASSERT_NE(notice, nullptr);
        QWidget* inspector = nullptr;
        for (auto* child : widget.findChildren<QWidget*>())
        {
            if (child->qt_metacast("JE::Editor::Tools::AtomToolsDocumentInspector"))
            {
                inspector = child;
                break;
            }
        }
        ASSERT_NE(inspector, nullptr);
        QWidget* propertyEditor = nullptr;
        for (auto* child : inspector->findChildren<QWidget*>())
        {
            if (child->qt_metacast("AzToolsFramework::ReflectedPropertyEditor"))
            {
                propertyEditor = child;
                break;
            }
        }
        ASSERT_NE(propertyEditor, nullptr);
        auto* reflectedEditor = static_cast<AzToolsFramework::ReflectedPropertyEditor*>(
            propertyEditor->qt_metacast("AzToolsFramework::ReflectedPropertyEditor"));
        ASSERT_NE(reflectedEditor, nullptr);

        for (const QSize size : {QSize(900, 650), QSize(1400, 900)})
        {
            SCOPED_TRACE(testing::Message() << "workspace " << size.width() << "x" << size.height());
            widget.resize(size);
            widget.show();
            // Property population and ancestor layout requests span multiple queued event turns.
            QElapsedTimer layoutDeadline;
            layoutDeadline.start();
            do
            {
                QApplication::processEvents(QEventLoop::AllEvents);
                widget.layout()->activate();
            } while (layoutDeadline.elapsed() < 1000 &&
                (reflectedEditor->GetWidgets().empty() || reflectedEditor->GetContentHeight() <= 0 ||
                    propertyEditor->height() < propertyEditor->sizeHint().height()));
            EXPECT_FALSE(reflectedEditor->GetWidgets().empty());
            EXPECT_GT(reflectedEditor->GetContentHeight(), 0);
            EXPECT_TRUE(notice->isVisible());
            EXPECT_FALSE(notice->text().isEmpty());
            EXPECT_LE(notice->height(), 4 * notice->fontMetrics().height());
            EXPECT_TRUE(inspector->isVisible());
            EXPECT_GE(inspector->height(), widget.height() / 2);
            EXPECT_GE(inspector->width(), 20 * inspector->fontMetrics().averageCharWidth());
            EXPECT_TRUE(propertyEditor->isVisible());
            EXPECT_TRUE(propertyEditor->isEnabled());
            const QRect propertyBounds(propertyEditor->mapTo(inspector, QPoint()), propertyEditor->size());
            EXPECT_GE(propertyBounds.intersected(inspector->rect()).height(), 6 * propertyEditor->fontMetrics().height());
        }
    }

    TEST_F(PassGraphWorkspaceTests, LegacyDefaultIsCanonicalizedButCustomTemplateIsPreserved)
    {
        auto* registry = AZ::SettingsRegistry::Get();
        const char* key = "/O3DE/Atom/PassCanvas/DefaultPassGraphTemplate";
        ASSERT_TRUE(registry->Set(key, "@gemroot:PassCanvas@/Assets/PassCanvas/GraphData/blank_graph.passgraphtemplate"));
        EXPECT_EQ(JE::Editor::MaterialGraphEnvironment::GetDefaultPassGraphTemplatePath(),
            AZ::IO::Path(AZ::Test::GetEngineRootPath() +
                "/Editor/Materials/RenderPass/Assets/PassCanvas/GraphData/blank_graph.passgraphtemplate").StringAsPosix());
        const auto custom = m_directory->Resolve("source/custom.passgraphtemplate").Native();
        ASSERT_TRUE(registry->Set(key, custom));
        EXPECT_EQ(JE::Editor::MaterialGraphEnvironment::GetDefaultPassGraphTemplatePath(), AZ::IO::Path(custom).StringAsPosix());
    }

    class PassGraphCategoryPaletteTests : public PassGraphWorkspaceTests
    {
    protected:
        void ConfigureNodePaletteConfigs(
            JE::Editor::Tools::DynamicNodeConfig& material, JE::Editor::Tools::DynamicNodeConfig& pass) const override
        {
            material.m_category = "Explicit Material Category";
            material.m_titlePaletteName = "MathNodeTitlePalette";
            pass.m_category = "Default Pass Category";
            pass.m_titlePaletteName.clear();
        }
    };

    TEST_F(PassGraphCategoryPaletteTests, CategoryPaletteDefaultsWhenUnspecified)
    {
        auto& environment = m_session->EnableGraphDocuments();
        JE::Editor::Tools::GraphCanvas::NodePaletteWidget palette(nullptr);
        JE::Editor::Tools::GraphCanvas::NodePaletteConfig settings;
        settings.m_editorId = m_session->GetToolId();
        settings.m_saveIdentifier = "CategoryPaletteTest";
        settings.m_rootTreeItem = environment.CreateNodePaletteTree(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        palette.SetupNodePalette(settings);

        auto* passCategory = azrtti_cast<JE::Editor::Tools::GraphCanvas::NodePaletteTreeItem*>(
            palette.FindItemWithName(QStringLiteral("Default Pass Category")));
        ASSERT_NE(passCategory, nullptr);
        EXPECT_EQ(passCategory->GetTitlePalette(), "DefaultNodeTitlePalette");
        auto* passNode = azrtti_cast<JE::Editor::Tools::GraphCanvas::NodePaletteTreeItem*>(palette.FindItemWithName(QStringLiteral("Only Pass")));
        ASSERT_NE(passNode, nullptr);
        EXPECT_EQ(passNode->GetTitlePalette(), "DefaultNodeTitlePalette");
        EXPECT_EQ(palette.FindItemWithName(QStringLiteral("Only Material")), nullptr);

        // An explicit, existing stylesheet palette remains selected when switching graph contexts.
        palette.ResetModel(environment.CreateNodePaletteTree(JE::Editor::MaterialGraphEnvironment::GraphKind::Material));
        auto* materialCategory = azrtti_cast<JE::Editor::Tools::GraphCanvas::NodePaletteTreeItem*>(
            palette.FindItemWithName(QStringLiteral("Explicit Material Category")));
        ASSERT_NE(materialCategory, nullptr);
        EXPECT_EQ(materialCategory->GetTitlePalette(), "MathNodeTitlePalette");
        auto* materialNode = azrtti_cast<JE::Editor::Tools::GraphCanvas::NodePaletteTreeItem*>(palette.FindItemWithName(QStringLiteral("Only Material")));
        ASSERT_NE(materialNode, nullptr);
        EXPECT_EQ(materialNode->GetTitlePalette(), "MathNodeTitlePalette");
        EXPECT_EQ(palette.FindItemWithName(QStringLiteral("Only Pass")), nullptr);
    }

    class PassGraphCollisionTests : public PassGraphWorkspaceTests
    {
    protected:
        bool SeedCollision() const override { return true; }
    };

    TEST_F(PassGraphCollisionTests, DuplicateUuidDoesNotReplaceMaterialConfigOrEnterPassPalette)
    {
        auto& environment = m_session->EnableGraphDocuments();
        JE::Editor::Tools::DynamicNodeConfig config;
        JE::Editor::Tools::DynamicNodeManagerRequestBus::EventResult(config, m_session->GetToolId(),
            &JE::Editor::Tools::DynamicNodeManagerRequests::GetConfigById, m_materialConfigId);
        EXPECT_EQ(config.m_id, m_materialConfigId);
        EXPECT_EQ(config.m_title, "Only Material");
        JE::Editor::Tools::GraphCanvas::NodePaletteWidget palette(nullptr);
        JE::Editor::Tools::GraphCanvas::NodePaletteConfig settings;
        settings.m_editorId = m_session->GetToolId();
        settings.m_saveIdentifier = "PassGraphCollisionTest";
        settings.m_rootTreeItem = environment.CreateNodePaletteTree(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        palette.SetupNodePalette(settings);
        EXPECT_NE(palette.FindItemWithName(QStringLiteral("Only Pass")), nullptr);
        EXPECT_EQ(palette.FindItemWithName(QStringLiteral("Rejected Collision")), nullptr);
        palette.ResetModel(environment.CreateNodePaletteTree(JE::Editor::MaterialGraphEnvironment::GraphKind::Material));
        EXPECT_NE(palette.FindItemWithName(QStringLiteral("Only Material")), nullptr);
        EXPECT_EQ(palette.FindItemWithName(QStringLiteral("Only Pass")), nullptr);
    }

    TEST_F(GraphDocumentHistoryTests, ConnectionReleaseWithoutPostStartMoveUsesReleasePosition)
    {
        ASSERT_NO_FATAL_FAILURE(CheckConnectionReleaseGesture(false));
    }

    TEST_F(GraphDocumentHistoryTests, ConnectionReleaseAfterMouseMovePreservesConnectionAndUndo)
    {
        ASSERT_NO_FATAL_FAILURE(CheckConnectionReleaseGesture(true));
    }

    TEST_F(GraphDocumentHistoryTests, ConnectionReleaseAfterDifferentLastMoveUsesReleasePosition)
    {
        ASSERT_NO_FATAL_FAILURE(CheckConnectionReleaseGesture(true, ReleaseControl::ChangedPosition));
    }

    TEST_F(GraphDocumentHistoryTests, ConnectionEscapeWithActiveProposalPreservesGraph)
    {
        ASSERT_NO_FATAL_FAILURE(CheckConnectionReleaseGesture(true, ReleaseControl::Cancel));
    }

    TEST_F(GraphDocumentHistoryTests, ExtenderReleaseAtEvaluatedPointCreatesOneSlotAndConnection)
    {
        ASSERT_NO_FATAL_FAILURE(CheckExtenderConnectionReleaseGesture(false));
    }

    TEST_F(GraphDocumentHistoryTests, ExtenderReleaseAtChangedPointPreservesSingleProposal)
    {
        ASSERT_NO_FATAL_FAILURE(CheckExtenderConnectionReleaseGesture(true));
    }

    TEST_F(GraphDocumentHistoryTests, ExtenderProposalReuseRejectsForeignConnectionAndPreservesOwner)
    {
        ASSERT_NO_FATAL_FAILURE(CheckExtenderConnectionReleaseGesture(false, true));
    }

    TEST_F(GraphDocumentHistoryTests, ModuleSlotTypesRemainCanonicalAcrossSerializationAndHistory)
    {
        ASSERT_NO_FATAL_FAILURE(AddNodes());
        auto checkCanonicalTypes = [&]()
        {
            const auto graph = m_document->GetGraph();
            const auto contextType = graph->GetContext()->GetDataType<float>();
            const auto input = azrtti_cast<JE::Editor::Tools::GraphModel::GraphInputNode*>(graph->GetNode(m_inputId));
            const auto output = azrtti_cast<JE::Editor::Tools::GraphModel::GraphOutputNode*>(graph->GetNode(m_outputId));
            ASSERT_NE(contextType, nullptr);
            ASSERT_NE(input, nullptr);
            ASSERT_NE(output, nullptr);
            EXPECT_EQ(input->GetNodeDataType(), contextType);
            EXPECT_EQ(output->GetNodeDataType(), contextType);
            EXPECT_EQ(input->GetSlot("value")->GetDataType(), contextType);
            EXPECT_EQ(output->GetSlot("value")->GetDataType(), contextType);
            EXPECT_TRUE(output->GetSlot("value")->IsSupportedDataType(input->GetSlot("value")->GetDataType()));
        };
        ASSERT_NO_FATAL_FAILURE(checkCanonicalTypes());
        ASSERT_NO_FATAL_FAILURE(CheckGraph(2.0f, false));
        {
            const auto input = azrtti_cast<JE::Editor::Tools::GraphModel::GraphInputNode*>(m_document->GetGraph()->GetNode(m_inputId));
            AZStd::unique_ptr<JE::Editor::Tools::GraphModel::GraphInputNode> detached(
                m_application->GetSerializeContext()->CloneObject(input.get()));
            ASSERT_NE(detached, nullptr);
            EXPECT_EQ(detached->GetGraph(), nullptr);
            ASSERT_NE(detached->GetNodeDataType(), nullptr);
            EXPECT_EQ(detached->GetNodeDataType()->GetTypeEnum(), input->GetNodeDataType()->GetTypeEnum());
            EXPECT_EQ(detached->GetNodeDataType()->GetTypeUuid(), input->GetNodeDataType()->GetTypeUuid());
            EXPECT_FLOAT_EQ(AZStd::any_cast<float>(detached->GetNodeDataType()->GetDefaultValue()), 0.0f);
        }
        ASSERT_TRUE(m_document->BeginEdit());
        const auto graph = m_document->GetGraph();
        const auto source = graph->GetNode(m_inputId)->GetSlot("value");
        const auto target = graph->GetNode(m_outputId)->GetSlot("value");
        JE::Editor::Tools::GraphCanvas::SlotId sourceSlot, targetSlot;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceSlot, m_graphId,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, source);
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(targetSlot, m_graphId,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, target);
        JE::Editor::Tools::GraphCanvas::Endpoint sourceEndpoint;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(sourceEndpoint, sourceSlot, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);
        bool canCreate = false;
        JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(canCreate, targetSlot, &JE::Editor::Tools::GraphCanvas::SlotRequests::CanCreateConnectionTo, sourceEndpoint);
        ASSERT_TRUE(canCreate);
        graph->GetNode(m_inputId)->GetSlot("defaultValue")->SetValue(7.0f);
        JE::Editor::Tools::GraphModel::ConnectionPtr connection;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(connection, m_graphId,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddConnection, source, target);
        ASSERT_NE(connection, nullptr);
        ASSERT_TRUE(m_document->EndEdit());
        ASSERT_TRUE(m_document->Save());
        ASSERT_TRUE(m_document->Reopen());
        ASSERT_NO_FATAL_FAILURE(checkCanonicalTypes());
        ASSERT_NO_FATAL_FAILURE(CheckGraph(7.0f, true));
        ASSERT_TRUE(m_document->Undo());
        ASSERT_NO_FATAL_FAILURE(checkCanonicalTypes());
        ASSERT_NO_FATAL_FAILURE(CheckGraph(2.0f, false));
        ASSERT_TRUE(m_document->Redo());
        ASSERT_NO_FATAL_FAILURE(checkCanonicalTypes());
        ASSERT_NO_FATAL_FAILURE(CheckGraph(7.0f, true));
    }

    TEST_F(GraphDocumentHistoryTests, ModuleSlotTypesRejectDifferentUuidAndUnregisteredEnum)
    {
        ASSERT_NO_FATAL_FAILURE(AddNodes());
        const auto graph = m_document->GetGraph();
        const auto target = graph->GetNode(m_outputId)->GetSlot("value");
        JE::Editor::Tools::GraphCanvas::SlotId targetSlot;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(targetSlot, m_graphId,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, target);
        ASSERT_TRUE(targetSlot.IsValid());
        const JE::Editor::Tools::GraphModel::DataTypeList rejectedTypes{
            m_graphContext->GetDataType<AZStd::string>(),
            AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(1, AZStd::string{}, "Enum collision"),
            AZStd::make_shared<JE::Editor::Tools::GraphModel::DataType>(99, 0.0f, "Unregistered enum")};
        for (const auto& dataType : rejectedTypes)
        {
            const auto input = AZStd::make_shared<JE::Editor::Tools::GraphModel::GraphInputNode>(graph, dataType);
            ASSERT_NE(input->GetNodeDataType(), nullptr);
            EXPECT_EQ(input->GetNodeDataType()->GetTypeEnum(), dataType->GetTypeEnum());
            EXPECT_EQ(input->GetNodeDataType()->GetTypeUuid(), dataType->GetTypeUuid());
            EXPECT_FALSE(target->IsSupportedDataType(input->GetSlot("value")->GetDataType()));
            AZ::Vector2 position(24.0f, 400.0f);
            JE::Editor::Tools::GraphCanvas::NodeId nodeId;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, input, position);
            ASSERT_TRUE(nodeId.IsValid());
            JE::Editor::Tools::GraphCanvas::SlotId sourceSlot;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, input->GetSlot("value"));
            ASSERT_TRUE(sourceSlot.IsValid());
            JE::Editor::Tools::GraphCanvas::Endpoint sourceEndpoint;
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(sourceEndpoint, sourceSlot, &JE::Editor::Tools::GraphCanvas::SlotRequests::GetEndpoint);
            ASSERT_TRUE(sourceEndpoint.IsValid());
            bool canCreate = true;
            JE::Editor::Tools::GraphCanvas::SlotRequestBus::EventResult(canCreate, targetSlot, &JE::Editor::Tools::GraphCanvas::SlotRequests::CanCreateConnectionTo, sourceEndpoint);
            EXPECT_FALSE(canCreate);
            EXPECT_EQ(graph->GetConnectionCount(), 0);
        }
    }

    TEST_F(GraphDocumentHistoryTests, RealCanvasReplayAndReopenPreserveLogicalReferencesAndGlobalChronology)
    {
        AddNodes();
        ASSERT_FALSE(HasFatalFailure());
        CheckGraph(2.0f, false);
        ASSERT_FALSE(HasFatalFailure());
        AZ::Entity entity("after");
        {
            AzToolsFramework::ScopedUndoBatch batch("Rename graph test entity");
            (aznew CanvasHistoryRename(entity))->SetParent(batch.GetUndoBatch());
        }
        ASSERT_TRUE(m_document->BeginEdit());
        auto graph = m_document->GetGraph();
        graph->GetNode(m_inputId)->GetSlot("defaultValue")->SetValue(7.0f);
        JE::Editor::Tools::GraphModel::ConnectionPtr connection;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(connection, m_graphId,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddConnection,
            graph->GetNode(m_inputId)->GetSlot("value"), graph->GetNode(m_outputId)->GetSlot("value"));
        ASSERT_NE(connection, nullptr);
        ASSERT_TRUE(m_document->EndEdit());
        CheckGraph(7.0f, true);
        ASSERT_TRUE(m_document->Undo());
        CheckGraph(2.0f, false);
        EXPECT_EQ(entity.GetName(), "after");
        ASSERT_TRUE(m_document->Undo());
        EXPECT_EQ(entity.GetName(), "before");
        CheckGraph(2.0f, false);
        ASSERT_TRUE(m_document->Redo());
        ASSERT_TRUE(m_document->Redo());
        CheckGraph(7.0f, true);
        EXPECT_EQ(entity.GetName(), "after");
        ASSERT_TRUE(m_document->Save());
        ASSERT_TRUE(m_document->Reopen());
        CheckGraph(7.0f, true);
        ASSERT_TRUE(m_document->Undo());
        CheckGraph(2.0f, false);
        ASSERT_TRUE(m_document->Redo());
        CheckGraph(7.0f, true);
        // The reference-based entity command must be removed before this stack entity is destroyed.
        m_application->FlushUndo();
    }

    TEST_F(GraphDocumentHistoryTests, RejectedReplayPreservesLiveGraphSceneAndController)
    {
        AddNodes();
        ASSERT_FALSE(HasFatalFailure());
        const auto state = m_document->GetHistoryState();
        ASSERT_FALSE(state.empty());
        const auto liveGraph = m_document->GetGraph();
        const auto invalidTarget = JE::Editor::Tools::DocumentHistoryState{0, 1, 2, 3};
        // ObjectStream reports the malformed data in addition to GraphDocument's explicit rejection.
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_document->RestoreHistoryState(state, invalidTarget));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_EQ(m_document->GetGraph(), liveGraph);
        CheckGraph(2.0f, false);
        liveGraph->GetNode(m_inputId)->GetSlot("defaultValue")->SetValue(9.0f);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_document->RestoreHistoryState(state, state));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(m_document->GetGraph(), liveGraph);
        CheckGraph(9.0f, false);
    }
}
