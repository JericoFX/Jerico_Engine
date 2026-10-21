/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/Materials/Graph/MaterialCanvasDocumentHistory.h>
#include <JE/Editor/Materials/Graph/MaterialGraphCompiler.h>
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>
#include "Workspace/MaterialWorkspaceSession.h"
#include "Workspace/MaterialGraphWorkspaceWidget.h"
#include <JE/Editor/Tools/Inspector/InspectorWidget.h>
#include <JE/Editor/Render/RPI/Edit/Material/LuaMaterialFunctorSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataHolder.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertySourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Render/RPI/Reflect/Material/MaterialFunctor.h>
#include <JE/Render/RPI/Reflect/Material/MaterialPropertyValue.h>
#include <JE/Editor/Tools/Graph/AssetStatusReporterSystemRequestBus.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNode.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodeManager.h>
#include <JE/Editor/Tools/Graph/GraphTemplateFileDataCache.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <JE/Editor/Tools/Graph/GraphCompiler.h>
#include <JE/Editor/Tools/Graph/GraphDocument.h>
#include <JE/Editor/Tools/Graph/GraphDocumentView.h>
#include <JE/Editor/Tools/Graph/GraphView.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Module/ModuleManagerBus.h>
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
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <QDataStream>
#include <GraphCanvas/Types/QtMetaTypes.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <JE/Editor/Tools/GraphModel/GraphModelBus.h>
#include <GraphModelSystemComponent.h>
#include <JE/Editor/Tools/GraphModel/Model/Connection.h>
#include <JE/Editor/Tools/GraphModel/Model/GraphContext.h>
#include <JE/Editor/Tools/GraphModel/Model/Module/InputOutputNodes.h>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <QGraphicsLayout>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFont>
#include <QLabel>
#include <QLayout>
#include <QPushButton>

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
            bool GetSourceInfoBySourcePath(const char*, AZ::Data::AssetInfo&, AZStd::string&) override { return false; }
            bool GetSourceInfoBySourceUUID(const AZ::Uuid&, AZ::Data::AssetInfo&, AZStd::string&) override { return false; }
            bool GetScanFolders(AZStd::vector<AZStd::string>& paths) override { paths.push_back(m_path); return true; }
            bool GetAssetSafeFolders(AZStd::vector<AZStd::string>& paths) override { paths.push_back(m_path); return true; }
            bool IsAssetPlatformEnabled(const char*) override { return false; }
            int GetPendingAssetsForPlatform(const char*) override { return 0; }
            bool GetAssetsProducedBySourceUUID(const AZ::Uuid&, AZStd::vector<AZ::Data::AssetInfo>&) override { return false; }
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
            qRegisterMetaType<GraphCanvas::Styling::ConnectionCurveType>();
            qRegisterMetaType<GraphCanvas::Styling::PaletteStyle>();

            // Register the production GraphCanvas module before activating the system and native GraphModel component.
            for (const char* name : {"GraphCanvas.Editor"})
            {
                AZ::ModuleManagerRequests::LoadModuleOutcome result = AZ::Failure(AZStd::string("Module manager unavailable"));
                AZ::ModuleManagerRequestBus::BroadcastResult(result, &AZ::ModuleManagerRequests::LoadDynamicModule,
                    name, AZ::ModuleInitializationSteps::RegisterComponentDescriptors, true);
                ASSERT_TRUE(result.IsSuccess()) << (result.IsSuccess() ? "" : result.GetError().c_str());
                m_modules.push_back(result.GetValue());
            }
            system->Init();
            system->Activate();
            ASSERT_EQ(system->GetState(), AZ::Entity::State::Active);
            for (const auto& module : m_modules)
            {
                ASSERT_NE(module->GetEntity(), nullptr);
                ASSERT_EQ(module->GetEntity()->GetState(), AZ::Entity::State::Active);
            }
            ASSERT_TRUE(GraphCanvas::GraphCanvasRequestBus::HasHandlers());
            ASSERT_TRUE(JE::Editor::Tools::GraphModel::Integration::GraphManagerRequestBus::HasHandlers());

            // Copy the production stylesheet into a private products root; no project cache or authored assets are touched.
            const auto style = AZ::Utils::ReadFile(engineRoot + "/Editor/Materials/Assets/MaterialCanvas/StyleSheet/materialcanvas_style.json");
            ASSERT_TRUE(style.IsSuccess());
            ASSERT_TRUE(AZ::Utils::WriteFile(style.GetValue(), m_directory->Resolve("products/canvas_style.json").Native()).IsSuccess());
            // Seed private products before enabling the real cache-write protection on their alias.
            m_fileIO->SetAlias("@products@", productsRoot.c_str());
            m_styleManager = AZStd::make_unique<GraphCanvas::StyleManager>(CanvasHistoryToolId, "canvas_style.json");
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
            ASSERT_TRUE(GraphCanvas::SceneRequestBus::HasHandlers(m_graphId));
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
                // Drop caller references while the application still owns the loaded modules and their reflection data.
                m_modules.clear();
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
            GraphCanvas::NodeId inputCanvasId, outputCanvasId;
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
            GraphCanvas::GraphCanvasGraphicsView view(nullptr, false);
            view.resize(1000, 650);
            view.SetEditorId(CanvasHistoryToolId);
            view.SetScene(m_graphId);
            view.show();
            const auto graph = m_document->GetGraph();
            const auto source = graph->GetNode(m_inputId)->GetSlot("value");
            const auto target = graph->GetNode(m_outputId)->GetSlot("value");
            GraphCanvas::SlotId sourceSlot, targetSlot;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, source);
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(targetSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, target);
            ASSERT_TRUE(sourceSlot.IsValid());
            ASSERT_TRUE(targetSlot.IsValid());
            GraphCanvas::Endpoint sourceEndpoint;
            GraphCanvas::SlotRequestBus::EventResult(sourceEndpoint, sourceSlot, &GraphCanvas::SlotRequests::GetEndpoint);
            ASSERT_TRUE(sourceEndpoint.IsValid());
            bool canCreateConnection = false;
            GraphCanvas::SlotRequestBus::EventResult(canCreateConnection, targetSlot,
                &GraphCanvas::SlotRequests::CanCreateConnectionTo, sourceEndpoint);
            RecordProperty("release_can_create_before_gesture", static_cast<int>(canCreateConnection));
            RecordProperty("release_source_slot", sourceSlot.ToString().c_str());
            RecordProperty("release_target_slot", targetSlot.ToString().c_str());
            bool canDisplayConnection = false;
            GraphCanvas::SlotRequestBus::EventResult(canDisplayConnection, targetSlot,
                &GraphCanvas::SlotRequests::CanDisplayConnectionTo, sourceEndpoint);
            GraphCanvas::GraphId targetScene;
            GraphCanvas::SceneMemberRequestBus::EventResult(targetScene, targetSlot, &GraphCanvas::SceneMemberRequests::GetScene);
            GraphCanvas::Endpoint targetEndpoint;
            GraphCanvas::SlotRequestBus::EventResult(targetEndpoint, targetSlot, &GraphCanvas::SlotRequests::GetEndpoint);
            bool modelConnectionValid = false;
            if (canDisplayConnection)
            {
                modelConnectionValid = GraphCanvas::GraphUtils::IsValidModelConnection(targetScene, sourceEndpoint, targetEndpoint);
            }
            RecordProperty("release_can_display_before_gesture", static_cast<int>(canDisplayConnection));
            RecordProperty("release_target_scene", targetScene.ToString().c_str());
            RecordProperty("release_document_scene", m_graphId.ToString().c_str());
            RecordProperty("release_model_handler_present", static_cast<int>(GraphCanvas::GraphModelRequestBus::HasHandlers(targetScene)));
            RecordProperty("release_model_check_evaluated", static_cast<int>(canDisplayConnection));
            RecordProperty("release_model_connection_valid", canDisplayConnection ? static_cast<int>(modelConnectionValid) : -1);
            bool directModelValid = false;
            GraphCanvas::GraphModelRequestBus::EventResult(directModelValid, targetScene,
                &GraphCanvas::GraphModelRequests::IsValidConnection, sourceEndpoint, targetEndpoint);
            RecordProperty("release_direct_model_valid", static_cast<int>(directModelValid));
            const auto remappedSources = GraphCanvas::GraphUtils::RemapEndpointForModel(sourceEndpoint);
            const auto remappedTargets = GraphCanvas::GraphUtils::RemapEndpointForModel(targetEndpoint);
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
            auto recordRoundTrip = [&](const char* label, const GraphCanvas::Endpoint& endpoint, const auto& slot)
            {
                JE::Editor::Tools::GraphModel::NodePtr mappedNode;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(mappedNode, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeById, endpoint.GetNodeId());
                const auto mappedSlot = mappedNode ? mappedNode->GetSlot(slot->GetSlotId()) : JE::Editor::Tools::GraphModel::SlotPtr{};
                GraphCanvas::SlotId roundTrip;
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
                GraphCanvas::NodeId canvasNode;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasNode, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeIdByNode, node);
                QGraphicsLayout* nodeLayout = nullptr;
                GraphCanvas::NodeLayoutRequestBus::EventResult(nodeLayout, canvasNode, &GraphCanvas::NodeLayoutRequests::GetLayout);
                ASSERT_NE(nodeLayout, nullptr);
                nodeLayout->activate();
                GraphCanvas::NodeUIRequestBus::Event(canvasNode, &GraphCanvas::NodeUIRequests::AdjustSize);
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            view.centerOn(350.0, 150.0);
            QPointF sourceScenePoint, targetScenePoint;
            GraphCanvas::SlotUIRequestBus::EventResult(sourceScenePoint, sourceSlot, &GraphCanvas::SlotUIRequests::GetConnectionPoint);
            GraphCanvas::SlotUIRequestBus::EventResult(targetScenePoint, targetSlot, &GraphCanvas::SlotUIRequests::GetConnectionPoint);
            const QPoint sourcePoint = view.mapFromScene(sourceScenePoint);
            const QPoint targetPoint = view.mapFromScene(targetScenePoint);
            ASSERT_NE(sourcePoint, targetPoint);
            ASSERT_TRUE(view.viewport()->rect().contains(sourcePoint));
            ASSERT_TRUE(view.viewport()->rect().contains(targetPoint));
            ASSERT_NE(view.scene(), nullptr);
            ASSERT_FALSE(GraphCanvas::AssetEditorRequestBus::HasHandlers(CanvasHistoryToolId));
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
            GraphCanvas::SlotUIRequestBus::EventResult(targetAfterGrab, targetSlot, &GraphCanvas::SlotUIRequests::GetConnectionPoint);
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
                    GraphCanvas::AssetEditorSettingsRequestBus::EventResult(snapDistance, CanvasHistoryToolId,
                        &GraphCanvas::AssetEditorSettingsRequests::GetSnapDistance);
                    AZStd::vector<GraphCanvas::Endpoint> endpoints;
                    GraphCanvas::SceneRequestBus::EventResult(endpoints, m_graphId, &GraphCanvas::SceneRequests::GetEndpointsInRect,
                        QRectF(scenePoint - QPointF(snapDistance, snapDistance), scenePoint + QPointF(snapDistance, snapDistance)));
                    ASSERT_TRUE(endpoints.empty()) << "The changed-release control requires an empty last-move candidate area.";
                    ASSERT_NE(evaluationPoint, targetPoint);
                }
                sendMouse(QEvent::MouseMove, evaluationPoint, Qt::NoButton, Qt::LeftButton);
            }
            QPointF targetBeforeRelease;
            GraphCanvas::SlotUIRequestBus::EventResult(targetBeforeRelease, targetSlot, &GraphCanvas::SlotUIRequests::GetConnectionPoint);
            recordPoint("release_target_scene_before_release", targetBeforeRelease);
            recordPoint("release_target_pixel_mapped_before_release", view.mapToScene(targetPoint));
            if (control == ReleaseControl::Cancel)
            {
                AZStd::vector<AZ::EntityId> connections;
                GraphCanvas::SceneRequestBus::EventResult(connections, m_graphId, &GraphCanvas::SceneRequests::GetConnections);
                ASSERT_EQ(connections.size(), 1);
                GraphCanvas::Endpoint proposedTarget;
                GraphCanvas::ConnectionRequestBus::EventResult(proposedTarget, connections.front(),
                    &GraphCanvas::ConnectionRequests::GetTargetEndpoint);
                ASSERT_EQ(proposedTarget.GetSlotId(), targetSlot) << "Escape must cancel an actual accepted proposal.";
                GraphCanvas::ViewNotificationBus::Event(view.GetViewId(), &GraphCanvas::ViewNotifications::OnEscape);
                sendMouse(QEvent::MouseButtonRelease, targetPoint, Qt::LeftButton, Qt::NoButton);
                GraphCanvas::SceneRequestBus::EventResult(connections, m_graphId, &GraphCanvas::SceneRequests::GetConnections);
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
            GraphCanvas::NodeId sourceCanvasNode, targetCanvasNode;
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
            GraphCanvas::GraphCanvasGraphicsView view(nullptr, false);
            view.resize(1000, 650);
            view.SetEditorId(CanvasHistoryToolId);
            view.SetScene(m_graphId);
            view.show();
            for (const auto& canvasNode : {sourceCanvasNode, targetCanvasNode})
            {
                QGraphicsLayout* layout = nullptr;
                GraphCanvas::NodeLayoutRequestBus::EventResult(layout, canvasNode, &GraphCanvas::NodeLayoutRequests::GetLayout);
                ASSERT_NE(layout, nullptr);
                layout->activate();
                GraphCanvas::NodeUIRequestBus::Event(canvasNode, &GraphCanvas::NodeUIRequests::AdjustSize);
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            view.centerOn(350.0, 150.0);
            GraphCanvas::SlotId sourceSlot, extenderSlot;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, source->GetSlot("value"));
            AZStd::vector<AZ::EntityId> targetSlots;
            GraphCanvas::NodeRequestBus::EventResult(targetSlots, targetCanvasNode, &GraphCanvas::NodeRequests::GetSlotIds);
            for (const auto& slot : targetSlots)
            {
                GraphCanvas::SlotType type = GraphCanvas::SlotTypes::Invalid;
                GraphCanvas::SlotRequestBus::EventResult(type, slot, &GraphCanvas::SlotRequests::GetSlotType);
                if (type == GraphCanvas::SlotTypes::ExtenderSlot)
                {
                    ASSERT_FALSE(extenderSlot.IsValid());
                    extenderSlot = slot;
                }
            }
            ASSERT_TRUE(sourceSlot.IsValid());
            ASSERT_TRUE(extenderSlot.IsValid());
            QPointF sourceScenePoint, extenderScenePoint;
            GraphCanvas::SlotUIRequestBus::EventResult(sourceScenePoint, sourceSlot, &GraphCanvas::SlotUIRequests::GetConnectionPoint);
            GraphCanvas::SlotUIRequestBus::EventResult(extenderScenePoint, extenderSlot, &GraphCanvas::SlotUIRequests::GetConnectionPoint);
            const QPoint sourcePoint = view.mapFromScene(sourceScenePoint);
            const QPoint extenderPoint = view.mapFromScene(extenderScenePoint);
            ASSERT_TRUE(view.viewport()->rect().contains(sourcePoint));
            ASSERT_TRUE(view.viewport()->rect().contains(extenderPoint));
            ASSERT_NE(sourcePoint, extenderPoint);
            ASSERT_FALSE(GraphCanvas::AssetEditorRequestBus::HasHandlers(CanvasHistoryToolId));
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
            GraphCanvas::SceneRequestBus::EventResult(proposals, m_graphId, &GraphCanvas::SceneRequests::GetConnections);
            ASSERT_EQ(proposals.size(), 1);
            GraphCanvas::SlotId createdSlot;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(createdSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot,
                *target->GetExtendableSlots("value").begin());
            GraphCanvas::Endpoint proposedTarget;
            GraphCanvas::ConnectionRequestBus::EventResult(proposedTarget, proposals.front(), &GraphCanvas::ConnectionRequests::GetTargetEndpoint);
            ASSERT_EQ(proposedTarget.GetSlotId(), createdSlot);
            if (checkOwnership)
            {
                GraphCanvas::ScopedGraphUndoBlocker undoBlocker(m_graphId);
                const auto ownerId = proposals.front();
                auto* ownerGrabber = view.scene()->mouseGrabberItem();
                ASSERT_NE(ownerGrabber, nullptr);
                GraphCanvas::Endpoint sourceEndpoint;
                GraphCanvas::SlotRequestBus::EventResult(sourceEndpoint, sourceSlot, &GraphCanvas::SlotRequests::GetEndpoint);
                ASSERT_TRUE(sourceEndpoint.IsValid());
                auto checkOwner = [&]()
                {
                    ASSERT_TRUE(GraphCanvas::ConnectionRequestBus::HasHandlers(ownerId));
                    GraphCanvas::GraphId ownerScene;
                    GraphCanvas::SceneMemberRequestBus::EventResult(ownerScene, ownerId, &GraphCanvas::SceneMemberRequests::GetScene);
                    EXPECT_EQ(ownerScene, m_graphId);
                    GraphCanvas::Endpoint actualSource, actualTarget;
                    GraphCanvas::ConnectionRequestBus::EventResult(actualSource, ownerId, &GraphCanvas::ConnectionRequests::GetSourceEndpoint);
                    GraphCanvas::ConnectionRequestBus::EventResult(actualTarget, ownerId, &GraphCanvas::ConnectionRequests::GetTargetEndpoint);
                    EXPECT_EQ(actualSource, sourceEndpoint);
                    EXPECT_EQ(actualTarget, proposedTarget);
                    EXPECT_EQ(target->GetExtendableSlots("value").size(), 1);
                    EXPECT_EQ(graph->GetConnectionCount(), 0);
                };
                GraphCanvas::Endpoint reused;
                GraphCanvas::ExtenderSlotRequestBus::EventResult(reused, extenderSlot,
                    &GraphCanvas::ExtenderSlotRequests::ExtendForConnectionProposal, ownerId, sourceEndpoint);
                EXPECT_EQ(reused, proposedTarget);
                GraphCanvas::ConnectionId foreignId;
                GraphCanvas::SlotRequestBus::EventResult(foreignId, sourceSlot,
                    &GraphCanvas::SlotRequests::DisplayConnectionWithEndpoint, proposedTarget);
                ASSERT_TRUE(foreignId.IsValid());
                ASSERT_NE(foreignId, ownerId);
                ASSERT_TRUE(GraphCanvas::ConnectionRequestBus::HasHandlers(foreignId));
                GraphCanvas::GraphId foreignScene;
                GraphCanvas::SceneMemberRequestBus::EventResult(foreignScene, foreignId, &GraphCanvas::SceneMemberRequests::GetScene);
                ASSERT_EQ(foreignScene, m_graphId);
                ASSERT_NO_FATAL_FAILURE(checkOwner());
                GraphCanvas::Endpoint rejected;
                GraphCanvas::ExtenderSlotRequestBus::EventResult(rejected, extenderSlot,
                    &GraphCanvas::ExtenderSlotRequests::ExtendForConnectionProposal, foreignId, sourceEndpoint);
                EXPECT_FALSE(rejected.IsValid());
                ASSERT_NO_FATAL_FAILURE(checkOwner());
                const AZStd::unordered_set<AZ::EntityId> foreignOnly{foreignId};
                GraphCanvas::SceneRequestBus::Event(m_graphId, &GraphCanvas::SceneRequests::Delete, foreignOnly);
                EXPECT_FALSE(GraphCanvas::ConnectionRequestBus::HasHandlers(foreignId));
                ASSERT_NO_FATAL_FAILURE(checkOwner());
                AZStd::vector<AZ::EntityId> remaining;
                GraphCanvas::SceneRequestBus::EventResult(remaining, m_graphId, &GraphCanvas::SceneRequests::GetConnections);
                ASSERT_EQ(remaining.size(), 1);
                ASSERT_EQ(remaining.front(), ownerId);
                ASSERT_EQ(view.scene()->mouseGrabberItem(), ownerGrabber) << "Deleting the foreign connection must restore the owner grab.";
                GraphCanvas::ExtenderSlotRequestBus::EventResult(reused, extenderSlot,
                    &GraphCanvas::ExtenderSlotRequests::ExtendForConnectionProposal, ownerId, sourceEndpoint);
                EXPECT_EQ(reused, proposedTarget);
                ASSERT_NO_FATAL_FAILURE(checkOwner());
            }
            QPoint releasePoint = extenderPoint;
            if (changedReleasePoint)
            {
                GraphCanvas::SlotUIRequestBus::EventResult(extenderScenePoint, extenderSlot, &GraphCanvas::SlotUIRequests::GetConnectionPoint);
                releasePoint = view.mapFromScene(extenderScenePoint) + QPoint(1, 0);
                ASSERT_NE(releasePoint, extenderPoint);
                ASSERT_TRUE(view.viewport()->rect().contains(releasePoint));
                ASSERT_TRUE(GraphCanvas::GraphUtils::IsSlotVisible(extenderSlot));
                double snapDistance = 10.0;
                GraphCanvas::AssetEditorSettingsRequestBus::EventResult(snapDistance, CanvasHistoryToolId,
                    &GraphCanvas::AssetEditorSettingsRequests::GetSnapDistance);
                const auto releaseScenePoint = view.mapToScene(releasePoint);
                AZStd::vector<GraphCanvas::Endpoint> endpoints;
                GraphCanvas::SceneRequestBus::EventResult(endpoints, m_graphId, &GraphCanvas::SceneRequests::GetEndpointsInRect,
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
                GraphCanvas::NodeId canvasId;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(canvasId, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeIdByNode, node);
                ASSERT_TRUE(canvasId.IsValid());
                JE::Editor::Tools::GraphModel::NodePtr mapped;
                JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(mapped, m_graphId,
                    &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetNodeById, canvasId);
                EXPECT_EQ(mapped, node);
                GraphCanvas::SlotId canvasSlot;
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
        AZStd::vector<AZStd::shared_ptr<AZ::ModuleData>> m_modules;
        AZStd::unique_ptr<GraphCanvas::StyleManager> m_styleManager;
        JE::Editor::Tools::GraphModel::GraphContextPtr m_graphContext;
        JE::Editor::Tools::DocumentHistoryPolicy m_policy;
        AZStd::unique_ptr<CanvasHistorySourceFolder> m_sourceFolder;
        AZStd::unique_ptr<JE::Editor::Tools::GraphDocument> m_document;
        AZStd::string m_path;
        GraphCanvas::GraphId m_graphId;
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
                GraphCanvas::NodeId canvasId;
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
        EXPECT_EQ(jobs.m_paths, m_compiler->GetGeneratedFilePaths());
        ASSERT_EQ(jobs.m_paths.size(), 9);
        const auto root = m_directory->Resolve("source/generation").Native();
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
            JE::Editor::Tools::GraphCompiler::Reflect(context);
            JE::Editor::Tools::GraphViewSettings::Reflect(context);
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

        struct FramingNotifications final : GraphCanvas::ViewNotificationBus::Handler
        {
            explicit FramingNotifications(const GraphCanvas::ViewId& viewId) { BusConnect(viewId); }
            ~FramingNotifications() { BusDisconnect(); }
            void OnViewCenteredOnArea() override { ++m_centeredCount; }
            int m_centeredCount = 0;
        };

        void PrepareFramingDocument(AZ::Uuid& id, GraphCanvas::GraphId& scene)
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
            GraphCanvas::NodeId canvasNode;
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
                GraphCanvas::NodeLayoutRequestBus::EventResult(
                    nodeLayout, canvasNode, &GraphCanvas::NodeLayoutRequests::GetLayout);
                ASSERT_NE(nodeLayout, nullptr);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
                nodeLayout->activate();
                GraphCanvas::NodeUIRequestBus::Event(canvasNode, &GraphCanvas::NodeUIRequests::AdjustSize);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                nodeLayout->activate();
                GraphCanvas::NodeUIRequestBus::Event(canvasNode, &GraphCanvas::NodeUIRequests::AdjustSize);
            }
            GraphCanvas::ViewId releasedViewId;
            GraphCanvas::SceneRequestBus::EventResult(releasedViewId, scene, &GraphCanvas::SceneRequests::GetViewId);
            ASSERT_FALSE(releasedViewId.IsValid());
        }

        AZStd::unique_ptr<AZ::UserSettingsProvider> m_localUserSettings;
        AZStd::unique_ptr<AZ::UserSettingsProvider> m_globalUserSettings;
        AZStd::unique_ptr<JE::Editor::MaterialWorkspaceSession> m_session;
        AZ::Uuid m_materialConfigId;
        bool m_workspaceReflected = false;
    };

    TEST_F(PassGraphWorkspaceTests, InitialGraphFramingMatchesLayoutReadyControl)
    {
        const auto id = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::Material);
        ASSERT_FALSE(id.IsNull());
        JE::Editor::Tools::GraphModel::GraphPtr graph;
        GraphCanvas::GraphId scene;
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
        GraphCanvas::NodeId canvasNode;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(
            canvasNode, scene, &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, node, position);
        ASSERT_TRUE(canvasNode.IsValid());
        JE::Editor::Tools::AtomToolsDocumentRequestBus::EventResult(
            success, id, &JE::Editor::Tools::AtomToolsDocumentRequests::EndEdit);
        ASSERT_TRUE(success);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        {
            struct ViewParamChanges final : GraphCanvas::ViewNotificationBus::Router
            {
                ViewParamChanges() { BusRouterConnect(); }
                ~ViewParamChanges() { BusRouterDisconnect(); }
                void OnViewParamsChanged(const GraphCanvas::ViewParams&) override { ++changes; }
                int changes = 0;
            } viewParamChanges;
            GraphCanvas::ViewId preparationViewId;
            {
                // Registering the first real view activates scene items/styles, but GraphView does not fit the graph.
                JE::Editor::Tools::GraphView preparationView(
                    m_session->GetToolId(), scene, m_session->EnableGraphDocuments().GetViewSettings());
                GraphCanvas::SceneRequestBus::EventResult(
                    preparationViewId, scene, &GraphCanvas::SceneRequests::GetViewId);
                ASSERT_TRUE(preparationViewId.IsValid());
                qreal preparationZoom = 0.0;
                GraphCanvas::ViewRequestBus::EventResult(
                    preparationZoom, preparationViewId, &GraphCanvas::ViewRequests::GetZoomLevel);
                ASSERT_DOUBLE_EQ(preparationZoom, 1.0);
                RecordProperty("preparation_zoom", QString::number(preparationZoom, 'g', 17).toStdString());

                QGraphicsLayout* nodeLayout = nullptr;
                GraphCanvas::NodeLayoutRequestBus::EventResult(
                    nodeLayout, canvasNode, &GraphCanvas::NodeLayoutRequests::GetLayout);
                ASSERT_NE(nodeLayout, nullptr);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::PolishRequest);
                nodeLayout->activate();
                GraphCanvas::NodeUIRequestBus::Event(canvasNode, &GraphCanvas::NodeUIRequests::AdjustSize);
                QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                nodeLayout->activate();
                GraphCanvas::NodeUIRequestBus::Event(canvasNode, &GraphCanvas::NodeUIRequests::AdjustSize);

                QRectF preparedArea;
                GraphCanvas::ViewRequestBus::EventResult(
                    preparedArea, preparationViewId, &GraphCanvas::ViewRequests::GetCompleteArea);
                ASSERT_FALSE(preparedArea.isEmpty());
                RecordProperty("prepared_node_width", QString::number(preparedArea.width(), 'g', 17).toStdString());
                RecordProperty("prepared_node_height", QString::number(preparedArea.height(), 'g', 17).toStdString());
            }
            GraphCanvas::ViewId releasedViewId;
            GraphCanvas::SceneRequestBus::EventResult(releasedViewId, scene, &GraphCanvas::SceneRequests::GetViewId);
            ASSERT_FALSE(releasedViewId.IsValid());
            ASSERT_FALSE(GraphCanvas::ViewRequestBus::HasHandlers(preparationViewId));
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
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, scene, &GraphCanvas::SceneRequests::GetViewId);
        ASSERT_TRUE(viewId.IsValid());

        const QSize hiddenViewport = graphicsView->viewport()->size();
        const QTransform hiddenTransform = graphicsView->transform();
        QRectF initialArea;
        GraphCanvas::ViewRequestBus::EventResult(initialArea, viewId, &GraphCanvas::ViewRequests::GetCompleteArea);
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
        GraphCanvas::ViewRequestBus::EventResult(laidOutArea, viewId, &GraphCanvas::ViewRequests::GetCompleteArea);
        RecordProperty("laid_out_viewport_width", laidOutViewport.width());
        RecordProperty("laid_out_viewport_height", laidOutViewport.height());
        RecordProperty("initial_zoom_after_layout", QString::number(initialAfterLayout.m11(), 'g', 17).toStdString());
        ASSERT_GT(laidOutViewport.width(), hiddenViewport.width());
        ASSERT_GT(laidOutViewport.height(), hiddenViewport.height());
        ASSERT_EQ(laidOutArea, initialArea) << "Node bounds changed; viewport timing alone is not isolated.";

        // Re-run the existing fit from the source default only after the real viewport geometry exists.
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::SetViewParams, GraphCanvas::ViewParams{});
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
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
        GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(id, scene));
        QWidget host;
        host.resize(900, 650);
        auto* layout = new QVBoxLayout(&host);
        auto view = AZStd::make_unique<JE::Editor::Tools::GraphDocumentView>(
            m_session->GetToolId(), id, m_session->EnableGraphDocuments().GetViewSettings(), &host);
        const auto graphicsViews = view->findChildren<QGraphicsView*>();
        ASSERT_EQ(graphicsViews.size(), 1);
        auto* graphicsView = graphicsViews.front();
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, scene, &GraphCanvas::SceneRequests::GetViewId);
        ASSERT_TRUE(viewId.IsValid());
        FramingNotifications notifications(viewId);
        layout->addWidget(view.get());
        host.show();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        layout->activate();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        ASSERT_GT(notifications.m_centeredCount, 0);

        GraphCanvas::ViewParams manualParams;
        manualParams.m_scale = 0.5;
        manualParams.m_anchorPointX = 4000.0f;
        manualParams.m_anchorPointY = 5000.0f;
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::SetViewParams, manualParams);
        // SetViewParams can replay the queued first fit; pan after that native restoration.
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOn, QPointF(4000.0, 5000.0));
        const QSize manualViewport = graphicsView->viewport()->size();
        const QPointF manualCenter = graphicsView->mapToScene(graphicsView->viewport()->rect().center());
        QRectF graphArea;
        GraphCanvas::ViewRequestBus::EventResult(graphArea, viewId, &GraphCanvas::ViewRequests::GetCompleteArea);
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
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
        EXPECT_GT(notifications.m_centeredCount, centeredBeforeShow);
        EXPECT_NE(graphicsView->mapToScene(graphicsView->viewport()->rect().center()), manualCenter);
    }

    TEST_F(PassGraphWorkspaceTests, InputBeforeInitialFramingPreservesManualCamera)
    {
        AZ::Uuid id;
        GraphCanvas::GraphId scene;
        ASSERT_NO_FATAL_FAILURE(PrepareFramingDocument(id, scene));
        QWidget host;
        host.resize(900, 650);
        auto* layout = new QVBoxLayout(&host);
        auto view = AZStd::make_unique<JE::Editor::Tools::GraphDocumentView>(
            m_session->GetToolId(), id, m_session->EnableGraphDocuments().GetViewSettings(), &host);
        const auto graphicsViews = view->findChildren<QGraphicsView*>();
        ASSERT_EQ(graphicsViews.size(), 1);
        auto* graphicsView = graphicsViews.front();
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, scene, &GraphCanvas::SceneRequests::GetViewId);
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
        GraphCanvas::ViewParams manualParams;
        manualParams.m_scale = 0.5;
        manualParams.m_anchorPointX = 4000.0f;
        manualParams.m_anchorPointY = 5000.0f;
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::SetViewParams, manualParams);
        const QSize manualViewport = graphicsView->viewport()->size();
        const QPointF manualCenter = graphicsView->mapToScene(graphicsView->viewport()->rect().center());
        const qreal manualZoom = graphicsView->transform().m11();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        ASSERT_EQ(graphicsView->viewport()->size(), manualViewport);
        EXPECT_EQ(notifications.m_centeredCount, 0);
        EXPECT_EQ(graphicsView->mapToScene(graphicsView->viewport()->rect().center()), manualCenter);
        EXPECT_DOUBLE_EQ(graphicsView->transform().m11(), manualZoom);

        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
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
        GraphCanvas::GraphId scene;
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
        GraphCanvas::NodeId canvasNode;
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
        EXPECT_FALSE(m_session->RequestCompile(id));
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

    TEST_F(PassGraphWorkspaceTests, SharedPaneSwitchesPaletteAndActionsWithoutClosingEitherDocument)
    {
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        const auto pass = OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass);
        ASSERT_FALSE(pass.IsNull());
        auto* paletteWidget = widget.findChild<QWidget*>(QStringLiteral("GraphNodePalette"));
        ASSERT_NE(paletteWidget, nullptr);
        auto* palette = static_cast<GraphCanvas::NodePaletteWidget*>(paletteWidget->qt_metacast(GraphCanvas::NodePaletteWidget::staticMetaObject.className()));
        ASSERT_NE(palette, nullptr);
        EXPECT_NE(palette->FindItemWithName(QStringLiteral("Only Pass")), nullptr);
        EXPECT_EQ(palette->FindItemWithName(QStringLiteral("Only Material")), nullptr);
        GraphCanvas::NodePaletteWidget contextPalette(nullptr);
        GraphCanvas::NodePaletteConfig contextSettings;
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
        GraphCanvas::GraphId nodeScene;
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
        GraphCanvas::NodePaletteWidget palette(nullptr);
        GraphCanvas::NodePaletteConfig settings;
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
        GraphCanvas::SlotId sourceSlot, targetSlot;
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceSlot, m_graphId,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, source);
        JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(targetSlot, m_graphId,
            &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, target);
        GraphCanvas::Endpoint sourceEndpoint;
        GraphCanvas::SlotRequestBus::EventResult(sourceEndpoint, sourceSlot, &GraphCanvas::SlotRequests::GetEndpoint);
        bool canCreate = false;
        GraphCanvas::SlotRequestBus::EventResult(canCreate, targetSlot, &GraphCanvas::SlotRequests::CanCreateConnectionTo, sourceEndpoint);
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
        GraphCanvas::SlotId targetSlot;
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
            GraphCanvas::NodeId nodeId;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(nodeId, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::AddNode, input, position);
            ASSERT_TRUE(nodeId.IsValid());
            GraphCanvas::SlotId sourceSlot;
            JE::Editor::Tools::GraphModel::Integration::GraphControllerRequestBus::EventResult(sourceSlot, m_graphId,
                &JE::Editor::Tools::GraphModel::Integration::GraphControllerRequests::GetSlotIdBySlot, input->GetSlot("value"));
            ASSERT_TRUE(sourceSlot.IsValid());
            GraphCanvas::Endpoint sourceEndpoint;
            GraphCanvas::SlotRequestBus::EventResult(sourceEndpoint, sourceSlot, &GraphCanvas::SlotRequests::GetEndpoint);
            ASSERT_TRUE(sourceEndpoint.IsValid());
            bool canCreate = true;
            GraphCanvas::SlotRequestBus::EventResult(canCreate, targetSlot, &GraphCanvas::SlotRequests::CanCreateConnectionTo, sourceEndpoint);
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
