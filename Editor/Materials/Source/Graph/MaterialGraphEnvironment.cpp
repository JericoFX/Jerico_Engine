/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#include <JE/Editor/Materials/Graph/MaterialGraphEnvironment.h>
#include <JE/Editor/Materials/Graph/MaterialGraphCompiler.h>
#include <JE/Editor/Materials/Graph/PassGraphCompiler.h>
#include <JE/Editor/Materials/Graph/MaterialCanvasDocumentHistory.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <JE/Editor/Render/RPI/Edit/Shader/ShaderSourceData.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>
#include <JE/Editor/Tools/Document/AtomToolsAnyDocument.h>
#include <JE/Editor/Tools/Graph/GraphDocument.h>
#include <JE/Editor/Tools/Graph/AssetStatusReporterSystem.h>
#include <JE/Editor/Tools/Graph/GraphTemplateFileDataCache.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodeManager.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodePaletteItem.h>
#include <JE/Editor/Tools/Graph/DynamicNode/DynamicNodeUtil.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <GraphModel/Integration/NodePalette/GraphCanvasNodePaletteItems.h>
#include <GraphModel/Model/DataType.h>
#include <QApplication>
#include <QTranslator>

static void InitMaterialGraphResources()
{
    Q_INIT_RESOURCE(MaterialCanvas);
    Q_INIT_RESOURCE(GraphView);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace JE::Editor
{
    namespace
    {
        // Editing remains functional; no automatic generation job exists for Pass Graph documents.
        class PassGraphDocument final : public JE::Editor::Tools::GraphDocument
        {
        public:
            AZ_CLASS_ALLOCATOR(PassGraphDocument, AZ::SystemAllocator);
            using GraphDocument::GraphDocument;
            bool CompileGraph() override { return false; }
            void QueueCompileGraph() override {}
            bool IsCompileGraphQueued() const override { return false; }
        };
    }

    AZStd::string MaterialGraphEnvironment::GetDefaultPassGraphTemplatePath()
    {
        constexpr const char* nativeDefault =
            "@engroot@/Editor/Materials/RenderPass/Assets/PassCanvas/GraphData/blank_graph.passgraphtemplate";
        auto path = JE::Editor::Tools::GetSettingsValue<AZStd::string>(
            "/O3DE/Atom/PassCanvas/DefaultPassGraphTemplate", nativeDefault);
        if (path == "@gemroot:PassCanvas@/Assets/PassCanvas/GraphData/blank_graph.passgraphtemplate")
        {
            path = nativeDefault;
        }
        return JE::Editor::Tools::GetPathWithoutAlias(path);
    }

    void MaterialGraphEnvironment::Reflect(AZ::ReflectContext* context)
    {
        MaterialGraphCompiler::Reflect(context);
        Materials::PassGraphCompiler::Reflect(context);
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->RegisterGenericType<AZStd::array<AZ::Vector2, 2>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector3, 3>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector4, 3>>();
            serialize->RegisterGenericType<AZStd::array<AZ::Vector4, 4>>();
        }
    }

    MaterialGraphEnvironment::MaterialGraphEnvironment(const AZ::Crc32& toolId)
        : m_toolId(toolId)
        , m_editorHistoryPolicy(CreateMaterialCanvasDocumentHistoryPolicy())
    {
        static const bool resourcesInitialized = []() { InitMaterialGraphResources(); return true; }();
        AZ_UNUSED(resourcesInitialized);
        InitDynamicNodeManager();
        InitDynamicNodeEditData();
        InitSharedGraphContext();
        InitGraphViewSettings();
        m_assetStatusReporterSystem = AZStd::make_unique<JE::Editor::Tools::AssetStatusReporterSystem>(m_toolId);
        m_graphTemplateFileDataCache = AZStd::make_shared<JE::Editor::Tools::GraphTemplateFileDataCache>(m_toolId);
        m_styleManager = AZStd::make_unique<GraphCanvas::StyleManager>(m_toolId, m_graphViewSettingsPtr->m_styleManagerPath);
        m_translator = AZStd::make_unique<QTranslator>();
        const auto translationPath = JE::Editor::Tools::GetPathWithoutAlias(m_graphViewSettingsPtr->m_translationPath);
        if (m_translator->load(QLocale::Language::English, translationPath.c_str()))
        {
            qApp->installTranslator(m_translator.get());
        }
        else
        {
            AZ_Warning("MaterialCanvas", false, "Error loading translation file %s", translationPath.c_str());
        }
    }

    MaterialGraphEnvironment::~MaterialGraphEnvironment()
    {
        JE::Editor::Tools::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", m_graphViewSettingsPtr);
        m_graphViewSettingsPtr->m_createNodeTreeItemsFn = {};
        qApp->removeTranslator(m_translator.get());
    }

    const AZStd::shared_ptr<GraphModel::GraphContext>& MaterialGraphEnvironment::GetGraphContext(GraphKind kind) const
    {
        return kind == GraphKind::RenderPass ? m_passGraphContext : m_graphContext;
    }

    GraphCanvas::GraphCanvasTreeItem* MaterialGraphEnvironment::CreateNodePaletteTree(GraphKind kind) const
    {
        auto* root = aznew GraphCanvas::NodePaletteTreeItem("Root", m_toolId);
        AZStd::unordered_map<AZStd::string, GraphCanvas::GraphCanvasTreeItem*> categories;
        categories[""] = root;
        const auto& ids = kind == GraphKind::RenderPass ? m_passNodeConfigs : m_materialNodeConfigs;
        for (const auto& id : ids)
        {
            const auto config = m_dynamicNodeManager->GetConfigById(id);
            auto category = categories.find(config.m_category);
            if (category == categories.end())
            {
                auto* item = root->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>(config.m_category, m_toolId);
                item->SetTitlePalette(config.m_titlePaletteName);
                category = categories.emplace(config.m_category, item).first;
            }
            category->second->CreateChildNode<JE::Editor::Tools::DynamicNodePaletteItem>(m_toolId, config);
        }
        GraphModelIntegration::AddCommonNodePaletteUtilities(root, m_toolId);
        return root;
    }

    void MaterialGraphEnvironment::SetActiveGraphKind(GraphKind kind)
    {
        m_activeGraphKind = kind;
    }

    void MaterialGraphEnvironment::LoadNodeConfigs(const AZStd::string& extension, AZStd::vector<AZ::Uuid>& accepted)
    {
        for (const auto& path : JE::Editor::Tools::GetPathsInSourceFoldersMatchingExtension(extension))
        {
            JE::Editor::Tools::DynamicNodeConfig config;
            if (config.Load(path))
            {
                config.AutoFillMissingData();
                // Duplicate UUIDs are rejected without replacing or renumbering authoring identities.
                if (m_dynamicNodeManager->RegisterConfig(config))
                {
                    accepted.push_back(config.m_id);
                }
            }
        }
    }

    const JE::Editor::Tools::GraphViewSettingsPtr& MaterialGraphEnvironment::GetViewSettings() const
    {
        return m_graphViewSettingsPtr;
    }

    void MaterialGraphEnvironment::InitDynamicNodeManager()
    {
        // Instantiate the dynamic node manager to register all dynamic node configurations and data types used in this tool
        m_dynamicNodeManager.reset(aznew JE::Editor::Tools::DynamicNodeManager(m_toolId));

        // Creating default sampler state with settings common to pre-existing material types.
        AZ::RHI::SamplerState defaultSamplerState{};
        defaultSamplerState.m_filterMin = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_filterMag = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_filterMip = AZ::RHI::FilterMode::Linear;
        defaultSamplerState.m_anisotropyMax = 16;

        // Register all data types required by Material Canvas nodes with the dynamic node manager
        m_dynamicNodeManager->RegisterDataTypes({
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("bool"), bool{}, "bool"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("int"), int32_t{}, "int"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("uint"), uint32_t{}, "uint"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float"), float{}, "float"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float2"), AZ::Vector2{}, "float2"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float3"), AZ::Vector3{}, "float3"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float4"), AZ::Vector4{}, "float4"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float2x2"), AZStd::array<AZ::Vector2, 2>{ AZ::Vector2(1.0f, 0.0f), AZ::Vector2(0.0f, 1.0f) }, "float2x2"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float3x3"), AZStd::array<AZ::Vector3, 3>{ AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f) }, "float3x3"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float4x3"), AZStd::array<AZ::Vector4, 3>{ AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f) }, "float4x3"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("float4x4"), AZStd::array<AZ::Vector4, 4>{ AZ::Vector4(1.0f, 0.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 1.0f, 0.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 1.0f, 0.0f), AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f) }, "float4x4"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("color"), AZ::Color::CreateOne(), "color"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("string"), AZStd::string{}, "string"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("image"), AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>{ AZ::Data::AssetLoadBehavior::NoLoad }, "image"),
            AZStd::make_shared<GraphModel::DataType>(AZ_CRC_CE("sampler"), defaultSamplerState, "sampler"),
        });

        // Search the project and gems for dynamic node configurations and register them with the manager
        LoadNodeConfigs("materialgraphnode", m_materialNodeConfigs);
        LoadNodeConfigs("passgraphnode", m_passNodeConfigs);
    }

    void MaterialGraphEnvironment::InitDynamicNodeEditData()
    {
        // Registering custom property handlers for dynamic node configuration settings. The settings are just a map of string data.
        // Recognized settings will need special controls for selecting files or editing large blocks of text without taking up much real
        // estate in the property editor. In the future, this will likely be replaced with a more specialized node configuration editor.
        AZ::Edit::ElementData editData;
        const auto addOwnedAttribute = [this, &editData](const AZ::Crc32& id, const auto& value)
        {
            JE::Editor::Tools::AddEditDataAttribute(editData, id, value);
            m_editDataAttributes.emplace_back(editData.m_attributes.back().second);
        };
        editData.m_elementId = AZ_CRC_CE("MultilineStringDialog");
        m_dynamicNodeManager->RegisterEditDataForSetting("instructions", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("classDefinitions", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("functionDefinitions", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertySrgMember", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyDescription", editData);

        editData = {};
        editData.m_elementId = AZ::Edit::UIHandlers::LineEdit;
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyName", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyDisplayName", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyConnectionName", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyGroupName", editData);
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyGroup", editData);

        editData = {};
        editData.m_elementId = AZ::Edit::UIHandlers::ComboBox;
        addOwnedAttribute(AZ::Edit::Attributes::StringList, AZStd::vector<AZStd::string>{ "", "0", "1", "2", "3", "4" });
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyMinVectorSize", editData);

        editData = {};
        editData.m_elementId = AZ::Edit::UIHandlers::ComboBox;
        addOwnedAttribute(AZ::Edit::Attributes::StringList,
            AZStd::vector<AZStd::string>{ "None", "ShaderInput", "ShaderOption", "ShaderEnabled", "InternalProperty", "" });
        m_dynamicNodeManager->RegisterEditDataForSetting("materialPropertyConnectionType", editData);

        editData = {};
        editData.m_elementId = AZ_CRC_CE("StringFilePath");
        addOwnedAttribute(AZ_CRC_CE("Title"), AZStd::string("Template File"));
        addOwnedAttribute(AZ_CRC_CE("Extensions"), AZStd::vector<AZStd::string>{ "azsl", "azsli", "material", "materialtype", "shader" });
        m_dynamicNodeManager->RegisterEditDataForSetting("templatePaths", editData);

        editData = {};
        editData.m_elementId = AZ_CRC_CE("StringFilePath");
        addOwnedAttribute(AZ_CRC_CE("Title"), AZStd::string("Include File"));
        addOwnedAttribute(AZ_CRC_CE("Extensions"), AZStd::vector<AZStd::string>{ "azsli" });
        m_dynamicNodeManager->RegisterEditDataForSetting("includePaths", editData);
    }

    void MaterialGraphEnvironment::InitSharedGraphContext()
    {
        // Each graph document creates its own graph context but we want to use a shared graph context instead to avoid data duplication
        m_graphContext = AZStd::make_shared<GraphModel::GraphContext>(
            "Material Graph", ".materialgraph", m_dynamicNodeManager->GetRegisteredDataTypes());
        m_graphContext->CreateModuleGraphManager();
        m_passGraphContext = AZStd::make_shared<GraphModel::GraphContext>(
            "Pass Graph", ".passgraph", m_dynamicNodeManager->GetRegisteredDataTypes());
        m_passGraphContext->CreateModuleGraphManager();
    }

    void MaterialGraphEnvironment::InitGraphViewSettings()
    {
        // This configuration data is passed through the main window and graph views to setup translation data, styling, and node palettes
        m_graphViewSettingsPtr = JE::Editor::Tools::GetSettingsObject(
            "/O3DE/Atom/GraphView/ViewSettings", AZStd::make_shared<JE::Editor::Tools::GraphViewSettings>());

        // Initialize the application specific graph view settings that are not serialized.
        m_graphViewSettingsPtr->m_translationPath = "@products@/materialcanvas/translation/materialcanvas_en_us.qm";
        m_graphViewSettingsPtr->m_styleManagerPath = "MaterialCanvas/StyleSheet/materialcanvas_style.json";
        m_graphViewSettingsPtr->m_nodeMimeType = "MaterialCanvas/node-palette-mime-event";
        m_graphViewSettingsPtr->m_nodeSaveIdentifier = "MaterialCanvas/ContextMenu";
        m_graphViewSettingsPtr->m_createNodeTreeItemsFn = [this](const AZ::Crc32&)
        {
            return CreateNodePaletteTree(m_activeGraphKind);
        };

        // Initialize the default group preset names and colors needed by the graph canvas view to create node groups.
        const AZStd::map<AZStd::string, AZ::Color> defaultGroupPresets = JE::Editor::Tools::GetSettingsObject(
            "/O3DE/Atom/GraphView/DefaultGroupPresets",
            AZStd::map<AZStd::string, AZ::Color>{ { "Logic", AZ::Color(0.188f, 0.972f, 0.243f, 1.0f) },
                                                  { "Function", AZ::Color(0.396f, 0.788f, 0.788f, 1.0f) },
                                                  { "Output", AZ::Color(0.866f, 0.498f, 0.427f, 1.0f) },
                                                  { "Input", AZ::Color(0.396f, 0.788f, 0.549f, 1.0f) } });

        // Connect the graph view settings to the required buses so that they can be accessed throughout the application.
        m_graphViewSettingsPtr->Initialize(m_toolId, defaultGroupPresets);
    }

    JE::Editor::Tools::DocumentTypeInfoVector MaterialGraphEnvironment::BuildDocumentTypes(HistoryOwner owner) const
    {
        JE::Editor::Tools::DocumentTypeInfoVector types;
        types.push_back(JE::Editor::Tools::GraphDocument::BuildDocumentTypeInfo(
            "Material Graph", {"materialgraph"}, {"materialgraphtemplate"},
            JE::Editor::Tools::GetPathWithoutAlias(JE::Editor::Tools::GetSettingsValue<AZStd::string>(
                "/O3DE/Atom/MaterialCanvas/DefaultMaterialGraphTemplate",
                "@engroot@/Editor/Materials/Assets/MaterialCanvas/GraphData/blank_graph.materialgraphtemplate")),
            m_graphContext, [toolId = m_toolId]() { return AZStd::make_shared<MaterialGraphCompiler>(toolId); }));
        types.push_back(JE::Editor::Tools::AtomToolsAnyDocument::BuildDocumentTypeInfo(
            "Material Graph Node Config", {"materialgraphnode"}, {"materialgraphnodetemplate"},
            AZStd::any(JE::Editor::Tools::DynamicNodeConfig()), AZ::Uuid::CreateNull()));
        types.push_back(JE::Editor::Tools::AtomToolsAnyDocument::BuildDocumentTypeInfo(
            "Shader Source Data", {"shader"}, {}, AZStd::any(JE::Render::RPI::ShaderSourceData()), JE::Render::RPI::ShaderSourceData::TYPEINFO_Uuid()));
        auto passType = JE::Editor::Tools::GraphDocument::BuildDocumentTypeInfo(
            PassGraphTypeName, {"passgraph"}, {"passgraphtemplate"}, GetDefaultPassGraphTemplatePath(), m_passGraphContext);
        passType.m_documentFactoryCallback = [context = m_passGraphContext](
            const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& info)
            -> JE::Editor::Tools::AtomToolsDocumentRequests*
        {
            // GraphDocument requires a compiler for its state callback; editing never submits it.
            return aznew PassGraphDocument(toolId, info, context, AZStd::make_shared<Materials::PassGraphCompiler>(toolId));
        };
        types.push_back(AZStd::move(passType));
        types.push_back(JE::Editor::Tools::AtomToolsAnyDocument::BuildDocumentTypeInfo(
            PassNodeConfigTypeName, {"passgraphnode"}, {"passgraphnodetemplate"},
            AZStd::any(JE::Editor::Tools::DynamicNodeConfig()), AZ::Uuid::CreateNull()));
        if (owner == HistoryOwner::Editor)
        {
            for (auto& type : types)
            {
                const auto factory = type.m_documentFactoryCallback;
                type.m_documentFactoryCallback = [factory, policy = m_editorHistoryPolicy](
                    const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& info)
                    -> JE::Editor::Tools::AtomToolsDocumentRequests*
                {
                    auto* document = factory(toolId, info);
                    if (document && !document->SetHistoryPolicy(policy))
                    {
                        delete document;
                        return nullptr;
                    }
                    return document;
                };
            }
        }
        return types;
    }
}
