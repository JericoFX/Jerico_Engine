/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once

#include <JE/Editor/Materials/Base.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentTypeInfo.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentRequestBus.h>
#include <JE/Editor/Tools/Graph/GraphViewSettings.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <JE/Editor/Tools/GraphModel/Model/GraphContext.h>

class QTranslator;
namespace GraphCanvas { class StyleManager; class GraphCanvasTreeItem; }
namespace JE::Editor::Tools
{
    class DynamicNodeManager;
    class AssetStatusReporterSystem;
    class GraphTemplateFileDataCache;
}

namespace JE::Editor
{
    //! One graph environment per tool session. Documents and views must be destroyed before this owner.
    class JE_EDITOR_MATERIALS_API MaterialGraphEnvironment final
    {
    public:
        enum class HistoryOwner { Standalone, Editor };
        enum class GraphKind { Material, RenderPass };
        static constexpr const char* PassGraphTypeName = "Pass Graph";
        static constexpr const char* PassNodeConfigTypeName = "Pass Graph Node Config";
        static AZStd::string GetDefaultPassGraphTemplatePath();
        static void Reflect(AZ::ReflectContext* context);
        explicit MaterialGraphEnvironment(const AZ::Crc32& toolId);
        ~MaterialGraphEnvironment();
        AZ_DISABLE_COPY_MOVE(MaterialGraphEnvironment);

        const AZStd::shared_ptr<JE::Editor::Tools::GraphModel::GraphContext>& GetGraphContext(GraphKind kind = GraphKind::Material) const;
        GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteTree(GraphKind kind) const;
        void SetActiveGraphKind(GraphKind kind);
        const JE::Editor::Tools::GraphViewSettingsPtr& GetViewSettings() const;
        JE::Editor::Tools::DocumentTypeInfoVector BuildDocumentTypes(HistoryOwner owner = HistoryOwner::Standalone) const;

    private:
        void InitDynamicNodeManager();
        void LoadNodeConfigs(const AZStd::string& extension, AZStd::vector<AZ::Uuid>& accepted);
        void InitDynamicNodeEditData();
        void InitSharedGraphContext();
        void InitGraphViewSettings();

        const AZ::Crc32 m_toolId;
        JE::Editor::Tools::DocumentHistoryPolicy m_editorHistoryPolicy;
        // The manager borrows these attributes; destroy it and its consumers before their sole owner.
        AZStd::vector<AZStd::unique_ptr<AZ::Attribute>> m_editDataAttributes;
        AZStd::unique_ptr<JE::Editor::Tools::DynamicNodeManager> m_dynamicNodeManager;
        AZStd::unique_ptr<JE::Editor::Tools::AssetStatusReporterSystem> m_assetStatusReporterSystem;
        AZStd::shared_ptr<JE::Editor::Tools::GraphTemplateFileDataCache> m_graphTemplateFileDataCache;
        AZStd::shared_ptr<JE::Editor::Tools::GraphModel::GraphContext> m_graphContext;
        AZStd::shared_ptr<JE::Editor::Tools::GraphModel::GraphContext> m_passGraphContext;
        AZStd::vector<AZ::Uuid> m_materialNodeConfigs;
        AZStd::vector<AZ::Uuid> m_passNodeConfigs;
        GraphKind m_activeGraphKind = GraphKind::Material;
        JE::Editor::Tools::GraphViewSettingsPtr m_graphViewSettingsPtr;
        AZStd::unique_ptr<GraphCanvas::StyleManager> m_styleManager;
        AZStd::unique_ptr<QTranslator> m_translator;
    };
}
