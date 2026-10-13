/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/Materials/Graph/MaterialCanvasDocumentHistory.h>
#include <JE/Editor/Materials/Graph/MaterialGraphCompiler.h>
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>
#include "Workspace/MaterialWorkspaceSession.h"
#include "Workspace/MaterialGraphWorkspaceWidget.h"
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <Atom/RPI.Edit/Material/LuaMaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataHolder.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <Atom/RPI.Edit/Material/MaterialPropertySourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <AtomToolsFramework/Graph/AssetStatusReporterSystemRequestBus.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManager.h>
#include <AtomToolsFramework/Graph/GraphTemplateFileDataCache.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AtomToolsFramework/Graph/GraphCompiler.h>
#include <AtomToolsFramework/Graph/GraphDocument.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
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
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Module/InputOutputNodes.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
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
            m_application->InitializeUndo();
            ASSERT_NE(system->FindComponent<AZ::AssetManagerComponent>(), nullptr);
            ASSERT_NE(system->FindComponent<AZ::JobManagerComponent>(), nullptr);
            ASSERT_NE(system->FindComponent<AZ::TaskGraphSystemComponent>(), nullptr);
            ASSERT_NE(system->FindComponent<AZ::StreamerComponent>(), nullptr);

            // Register both production modules before activation so their required services are sorted together.
            for (const char* name : {"GraphCanvas.Editor", "GraphModel.Editor"})
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
            ASSERT_TRUE(GraphModelIntegration::GraphManagerRequestBus::HasHandlers());

            // Copy the production stylesheet into a private products root; no project cache or authored assets are touched.
            const auto style = AZ::Utils::ReadFile(engineRoot + "/Editor/Materials/Assets/MaterialCanvas/StyleSheet/materialcanvas_style.json");
            ASSERT_TRUE(style.IsSuccess());
            ASSERT_TRUE(AZ::Utils::WriteFile(style.GetValue(), m_directory->Resolve("products/canvas_style.json").Native()).IsSuccess());
            // Seed private products before enabling the real cache-write protection on their alias.
            m_fileIO->SetAlias("@products@", productsRoot.c_str());
            m_styleManager = AZStd::make_unique<GraphCanvas::StyleManager>(CanvasHistoryToolId, "canvas_style.json");
            const GraphModel::DataTypeList types{
                AZStd::make_shared<GraphModel::DataType>(1, 0.0f, "Float"),
                AZStd::make_shared<GraphModel::DataType>(2, AZStd::string{}, "String")};
            m_graphContext = AZStd::make_shared<GraphModel::GraphContext>("GraphDocumentHistory", ".materialgraph", types);
            m_policy = JE::Editor::CreateMaterialCanvasDocumentHistoryPolicy();
            m_sourceFolder = AZStd::make_unique<CanvasHistorySourceFolder>(sourceRoot);
            m_path = m_directory->Resolve("source/history.materialgraph").Native();
            GraphModel::Graph initial(m_graphContext);
            ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, m_path).IsSuccess());
            const auto type = AtomToolsFramework::GraphDocument::BuildDocumentTypeInfo(
                "Material Graph", {"materialgraph"}, {}, {}, m_graphContext, {});
            m_document = AZStd::make_unique<AtomToolsFramework::GraphDocument>(CanvasHistoryToolId, type,
                m_graphContext, AZStd::make_shared<AtomToolsFramework::GraphCompiler>(CanvasHistoryToolId));
            ASSERT_TRUE(m_document->SetHistoryPolicy(m_policy));
            ASSERT_TRUE(m_document->Open(m_path));
            m_graphId = m_document->GetGraphId();
            ASSERT_TRUE(m_graphId.IsValid());
            ASSERT_TRUE(GraphCanvas::SceneRequestBus::HasHandlers(m_graphId));
            ASSERT_TRUE(GraphModelIntegration::GraphControllerRequestBus::HasHandlers(m_graphId));
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
            auto input = AZStd::make_shared<GraphModel::GraphInputNode>(graph, m_graphContext->GetDataType<float>());
            auto output = AZStd::make_shared<GraphModel::GraphOutputNode>(graph, m_graphContext->GetDataType<float>());
            input->GetSlot("name")->SetValue(AZStd::string("input"));
            output->GetSlot("name")->SetValue(AZStd::string("output"));
            input->GetSlot("defaultValue")->SetValue(2.0f);
            AZ::Vector2 inputPosition(24.0f, 48.0f);
            AZ::Vector2 outputPosition(480.0f, 96.0f);
            GraphCanvas::NodeId inputCanvasId, outputCanvasId;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(inputCanvasId, m_graphId,
                &GraphModelIntegration::GraphControllerRequests::AddNode, input, inputPosition);
            GraphModelIntegration::GraphControllerRequestBus::EventResult(outputCanvasId, m_graphId,
                &GraphModelIntegration::GraphControllerRequests::AddNode, output, outputPosition);
            ASSERT_TRUE(inputCanvasId.IsValid());
            ASSERT_TRUE(outputCanvasId.IsValid());
            m_inputId = input->GetId();
            m_outputId = output->GetId();
            GraphModelIntegration::GraphControllerRequestBus::EventResult(m_inputPosition, m_graphId,
                &GraphModelIntegration::GraphControllerRequests::GetPosition, input);
            GraphModelIntegration::GraphControllerRequestBus::EventResult(m_outputPosition, m_graphId,
                &GraphModelIntegration::GraphControllerRequests::GetPosition, output);
            ASSERT_TRUE(m_document->EndEdit());
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
                EXPECT_EQ(connection->GetSourceEndpoint(), GraphModel::Endpoint(m_inputId, source->GetSlotId()));
                EXPECT_EQ(connection->GetTargetEndpoint(), GraphModel::Endpoint(m_outputId, target->GetSlotId()));
            }
            for (auto node : {input, output})
            {
                GraphCanvas::NodeId canvasId;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(canvasId, m_graphId,
                    &GraphModelIntegration::GraphControllerRequests::GetNodeIdByNode, node);
                ASSERT_TRUE(canvasId.IsValid());
                GraphModel::NodePtr mapped;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(mapped, m_graphId,
                    &GraphModelIntegration::GraphControllerRequests::GetNodeById, canvasId);
                EXPECT_EQ(mapped, node);
                GraphCanvas::SlotId canvasSlot;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(canvasSlot, m_graphId,
                    &GraphModelIntegration::GraphControllerRequests::GetSlotIdBySlot, node->GetSlot("value"));
                EXPECT_TRUE(canvasSlot.IsValid());
                AZ::Vector2 position;
                GraphModelIntegration::GraphControllerRequestBus::EventResult(position, m_graphId,
                    &GraphModelIntegration::GraphControllerRequests::GetPosition, node);
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
        GraphModel::GraphContextPtr m_graphContext;
        AtomToolsFramework::DocumentHistoryPolicy m_policy;
        AZStd::unique_ptr<CanvasHistorySourceFolder> m_sourceFolder;
        AZStd::unique_ptr<AtomToolsFramework::GraphDocument> m_document;
        AZStd::string m_path;
        GraphCanvas::GraphId m_graphId;
        GraphModel::NodeId m_inputId = 0, m_outputId = 0;
        AZ::Vector2 m_inputPosition, m_outputPosition;
    };

    namespace
    {
        // Only job-status transport is supplied. No AP, shader compile or product success is simulated.
        class GenerationJobStatus final : public AtomToolsFramework::AssetStatusReporterSystemRequestBus::Handler
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
            AtomToolsFramework::AssetStatusReporterState GetStatus(const AZ::Uuid& requestId) const override
            {
                EXPECT_TRUE(m_active);
                EXPECT_EQ(requestId, m_requestId);
                return m_active && requestId == m_requestId ? AtomToolsFramework::AssetStatusReporterState::Failed
                                                          : AtomToolsFramework::AssetStatusReporterState::Invalid;
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
            AtomToolsFramework::DynamicNode::Reflect(context);
            AZ::RPI::MaterialFunctor::Reflect(context);
            AZ::RPI::MaterialPropertyValue::Reflect(context);
            AZ::RPI::MaterialPropertySourceData::Reflect(context);
            AZ::RPI::MaterialTypeSourceData::Reflect(context);
            AZ::RPI::MaterialFunctorSourceData::Reflect(context);
            AZ::RPI::MaterialFunctorSourceDataHolder::Reflect(context);
            AZ::RPI::LuaMaterialFunctorSourceData::Reflect(context);
        }

        void SetUp() override
        {
            GraphDocumentHistoryTests::SetUp();
            ASSERT_FALSE(HasFatalFailure());
            ReflectGenerationTypes(m_application->GetSerializeContext());
            ReflectGenerationTypes(m_application->GetJsonRegistrationContext());
            m_generationReflected = true;
            ASSERT_EQ(AZ::RPI::MaterialFunctorSourceDataRegistration::Get(), nullptr);
            m_functorRegistration = AZStd::make_unique<AZ::RPI::MaterialFunctorSourceDataRegistration>();
            m_functorRegistration->Init();
            m_functorRegistration->RegisterMaterialFunctor("Lua", azrtti_typeid<AZ::RPI::LuaMaterialFunctorSourceData>());
            m_fileIO->SetAlias("@engroot@", AZ::Test::GetEngineRootPath().c_str());
            m_nodeManager = AZStd::make_unique<AtomToolsFramework::DynamicNodeManager>(CanvasHistoryToolId);
            m_nodeManager->RegisterDataTypes({
                AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("bool"), false, "bool"),
                AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float"), 0.0f, "float"),
                AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float3"), AZ::Vector3::CreateZero(), "float3"),
                AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("string"), AZStd::string{}, "string")});
            m_templates = AZStd::make_unique<AtomToolsFramework::GraphTemplateFileDataCache>(CanvasHistoryToolId);
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
            m_generationContext = AZStd::make_shared<GraphModel::GraphContext>(
                "Material Graph", ".materialgraph", m_nodeManager->GetRegisteredDataTypes());
            GraphModel::Graph initial(m_generationContext);
            ASSERT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, m_generationPath).IsSuccess());
            m_compiler = AZStd::make_shared<JE::Editor::MaterialGraphCompiler>(CanvasHistoryToolId);
            const auto type = AtomToolsFramework::GraphDocument::BuildDocumentTypeInfo(
                "Material Graph", {"materialgraph"}, {}, {}, m_generationContext, {});
            m_generationDocument = AZStd::make_unique<AtomToolsFramework::GraphDocument>(
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
                GraphModelIntegration::GraphControllerRequestBus::EventResult(canvasId, graphId,
                    &GraphModelIntegration::GraphControllerRequests::AddNode, node, position);
                ASSERT_TRUE(canvasId.IsValid());
            }
            GraphModel::ConnectionPtr connection;
            GraphModelIntegration::GraphControllerRequestBus::EventResult(connection, graphId,
                &GraphModelIntegration::GraphControllerRequests::AddConnection,
                constant->GetSlot("outValue"), output->GetSlot("inRoughness"));
            ASSERT_NE(connection, nullptr);
            EXPECT_EQ(connection->GetSourceSlot(), constant->GetSlot("outValue"));
            EXPECT_EQ(connection->GetTargetSlot(), output->GetSlot("inRoughness"));
            m_constantId = constant->GetId();
        }

        bool m_generationReflected = false;
        AZStd::unique_ptr<AZ::RPI::MaterialFunctorSourceDataRegistration> m_functorRegistration;
        AZStd::unique_ptr<AtomToolsFramework::DynamicNodeManager> m_nodeManager;
        AZStd::unique_ptr<AtomToolsFramework::GraphTemplateFileDataCache> m_templates;
        AtomToolsFramework::DynamicNodeConfig m_constantConfig, m_outputConfig;
        GraphModel::GraphContextPtr m_generationContext;
        AZStd::shared_ptr<JE::Editor::MaterialGraphCompiler> m_compiler;
        AZStd::unique_ptr<AtomToolsFramework::GraphDocument> m_generationDocument;
        AZStd::string m_generationPath;
        GraphModel::NodeId m_constantId = 0;
    };

    TEST_F(MaterialGraphCompilerGenerationTests, ConnectedStandardPBRGeneratesRealSourcesWithoutClaimingAPSuccess)
    {
        CreateConnectedMaterialGraph(false);
        ASSERT_FALSE(HasFatalFailure());
        GenerationJobStatus jobs;
        const auto graph = m_generationDocument->GetGraph();
        EXPECT_FALSE(m_compiler->CompileGraph(graph, "Connected", m_generationPath));
        EXPECT_EQ(m_compiler->GetState(), AtomToolsFramework::GraphCompiler::State::Failed);
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
        EXPECT_EQ(m_compiler->GetState(), AtomToolsFramework::GraphCompiler::State::Failed);
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
        EXPECT_EQ(m_compiler->GetState(), AtomToolsFramework::GraphCompiler::State::Failed);
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

            AtomToolsFramework::DynamicNodeConfig material, pass;
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
            AtomToolsFramework::DocumentTypeInfo materialType;
            materialType.m_documentTypeName = "Material";
            materialType.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& info)
            {
                return aznew AtomToolsFramework::AtomToolsDocument(toolId, info);
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
            AtomToolsFramework::DynamicNode::Reflect(context);
            AtomToolsFramework::GraphCompiler::Reflect(context);
            AtomToolsFramework::GraphViewSettings::Reflect(context);
            AtomToolsFramework::InspectorWidget::Reflect(context);
            JE::Editor::MaterialGraphEnvironment::Reflect(context);
        }

        AZ::Uuid OpenGraph(JE::Editor::MaterialGraphEnvironment::GraphKind kind)
        {
            auto& environment = m_session->EnableGraphDocuments();
            GraphModel::Graph initial(environment.GetGraphContext(kind));
            const auto path = m_directory->Resolve(kind == JE::Editor::MaterialGraphEnvironment::GraphKind::RenderPass
                ? "source/test.passgraph" : "source/test.materialgraph").Native();
            EXPECT_TRUE(AZ::JsonSerializationUtils::SaveObjectToFile(&initial, path).IsSuccess());
            return m_session->OpenDocument(path);
        }

        AZStd::unique_ptr<AZ::UserSettingsProvider> m_localUserSettings;
        AZStd::unique_ptr<AZ::UserSettingsProvider> m_globalUserSettings;
        AZStd::unique_ptr<JE::Editor::MaterialWorkspaceSession> m_session;
        AZ::Uuid m_materialConfigId;
        bool m_workspaceReflected = false;
    };

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
        GraphModel::GraphPtr graph;
        GraphCanvas::GraphId scene;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(graph, id, &AtomToolsFramework::GraphDocumentRequests::GetGraph);
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(scene, id, &AtomToolsFramework::GraphDocumentRequests::GetGraphId);
        ASSERT_NE(graph, nullptr);
        ASSERT_TRUE(scene.IsValid());
        bool success = false;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(success, id, &AtomToolsFramework::AtomToolsDocumentRequests::BeginEdit);
        ASSERT_TRUE(success);
        auto node = AZStd::make_shared<GraphModel::GraphInputNode>(graph, graph->GetContext()->GetDataType<float>());
        node->GetSlot("defaultValue")->SetValue(0.375f);
        AZ::Vector2 position(24.0f, 48.0f);
        GraphCanvas::NodeId canvasNode;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(canvasNode, scene,
            &GraphModelIntegration::GraphControllerRequests::AddNode, node, position);
        ASSERT_TRUE(canvasNode.IsValid());
        const auto logicalId = node->GetId();
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(success, id, &AtomToolsFramework::AtomToolsDocumentRequests::EndEdit);
        ASSERT_TRUE(success);
        ASSERT_TRUE(m_application->GetUndoStack()->CanUndo());
        m_application->GetUndoStack()->Undo();
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(graph, id, &AtomToolsFramework::GraphDocumentRequests::GetGraph);
        EXPECT_EQ(graph->GetNodeCount(), 0);
        m_application->GetUndoStack()->Redo();
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(graph, id, &AtomToolsFramework::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph->GetNode(logicalId), nullptr);
        EXPECT_FLOAT_EQ(graph->GetNode(logicalId)->GetSlot("defaultValue")->GetValue<float>(), 0.375f);
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(success, id, &AtomToolsFramework::AtomToolsDocumentRequests::Save);
        ASSERT_TRUE(success);
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(success, id, &AtomToolsFramework::AtomToolsDocumentRequests::Reopen);
        ASSERT_TRUE(success);
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(graph, id, &AtomToolsFramework::GraphDocumentRequests::GetGraph);
        ASSERT_NE(graph->GetNode(logicalId), nullptr);
        EXPECT_FLOAT_EQ(graph->GetNode(logicalId)->GetSlot("defaultValue")->GetValue<float>(), 0.375f);

        class CompilationNotifications final : public AtomToolsFramework::GraphDocumentNotificationBus::Handler
        {
        public:
            CompilationNotifications() { BusConnect(JE::Editor::MaterialWorkspaceSession::GetToolId()); }
            ~CompilationNotifications() override { BusDisconnect(); }
            void OnCompileGraphStarted(const AZ::Uuid&) override { ++m_count; }
            void OnCompileGraphCompleted(const AZ::Uuid&) override { ++m_count; }
            void OnCompileGraphFailed(const AZ::Uuid&) override { ++m_count; }
            int m_count = 0;
        } notifications;
        AtomToolsFramework::GraphDocumentRequestBus::Event(id, &AtomToolsFramework::GraphDocumentRequests::QueueCompileGraph);
        bool queued = true;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(queued, id, &AtomToolsFramework::GraphDocumentRequests::IsCompileGraphQueued);
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
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(queued, material, &AtomToolsFramework::GraphDocumentRequests::IsCompileGraphQueued);
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
        EXPECT_TRUE(AtomToolsFramework::AtomToolsDocumentRequestBus::HasHandlers(pass));
        EXPECT_TRUE(AtomToolsFramework::AtomToolsDocumentRequestBus::HasHandlers(material));
        EXPECT_EQ(m_session->GetGraphDocumentTypes().size(), 5);
        const auto node = m_session->OpenDocument(m_directory->Resolve("source/pass.passgraphnode").Native());
        ASSERT_FALSE(node.IsNull());
        AtomToolsFramework::DocumentObjectInfoVector objects;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(objects, node,
            &AtomToolsFramework::AtomToolsDocumentRequests::GetObjectInfo);
        ASSERT_EQ(objects.size(), 1);
        EXPECT_EQ(objects.front().m_objectType, azrtti_typeid<AtomToolsFramework::DynamicNodeConfig>());
        GraphCanvas::GraphId nodeScene;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(nodeScene, node, &AtomToolsFramework::GraphDocumentRequests::GetGraphId);
        EXPECT_FALSE(nodeScene.IsValid());
        EXPECT_TRUE(compile->isHidden());
        EXPECT_NE(palette->FindItemWithName(QStringLiteral("Only Pass")), nullptr);
    }

    TEST_F(PassGraphWorkspaceTests, PassNodeConfigurationKeepsNoticeCompactAndInspectorUsable)
    {
        JE::Editor::MaterialGraphWorkspaceWidget widget(*m_session);
        const auto node = m_session->OpenDocument(m_directory->Resolve("source/pass.passgraphnode").Native());
        ASSERT_FALSE(node.IsNull());
        AtomToolsFramework::DocumentObjectInfoVector objects;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(objects, node,
            &AtomToolsFramework::AtomToolsDocumentRequests::GetObjectInfo);
        ASSERT_EQ(objects.size(), 1);
        EXPECT_EQ(objects.front().m_objectType, azrtti_typeid<AtomToolsFramework::DynamicNodeConfig>());

        auto* notice = widget.findChild<QLabel*>(QStringLiteral("GraphCompileStatus"));
        ASSERT_NE(notice, nullptr);
        QWidget* inspector = nullptr;
        for (auto* child : widget.findChildren<QWidget*>())
        {
            if (child->qt_metacast("AtomToolsFramework::AtomToolsDocumentInspector"))
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
        AtomToolsFramework::DynamicNodeConfig config;
        AtomToolsFramework::DynamicNodeManagerRequestBus::EventResult(config, m_session->GetToolId(),
            &AtomToolsFramework::DynamicNodeManagerRequests::GetConfigById, m_materialConfigId);
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
        GraphModel::ConnectionPtr connection;
        GraphModelIntegration::GraphControllerRequestBus::EventResult(connection, m_graphId,
            &GraphModelIntegration::GraphControllerRequests::AddConnection,
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
        const auto invalidTarget = AtomToolsFramework::DocumentHistoryState{0, 1, 2, 3};
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
