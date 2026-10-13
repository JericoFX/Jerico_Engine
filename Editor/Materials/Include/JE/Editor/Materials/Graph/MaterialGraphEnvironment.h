/* Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT */
#pragma once

#include <JE/Editor/Materials/Base.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Graph/GraphViewSettings.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <GraphModel/Model/GraphContext.h>

class QTranslator;
namespace GraphCanvas { class StyleManager; class GraphCanvasTreeItem; }
namespace AtomToolsFramework
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

        const AZStd::shared_ptr<GraphModel::GraphContext>& GetGraphContext(GraphKind kind = GraphKind::Material) const;
        GraphCanvas::GraphCanvasTreeItem* CreateNodePaletteTree(GraphKind kind) const;
        void SetActiveGraphKind(GraphKind kind);
        const AtomToolsFramework::GraphViewSettingsPtr& GetViewSettings() const;
        AtomToolsFramework::DocumentTypeInfoVector BuildDocumentTypes(HistoryOwner owner = HistoryOwner::Standalone) const;

    private:
        void InitDynamicNodeManager();
        void LoadNodeConfigs(const AZStd::string& extension, AZStd::vector<AZ::Uuid>& accepted);
        void InitDynamicNodeEditData();
        void InitSharedGraphContext();
        void InitGraphViewSettings();

        const AZ::Crc32 m_toolId;
        AtomToolsFramework::DocumentHistoryPolicy m_editorHistoryPolicy;
        // The manager borrows these attributes; destroy it and its consumers before their sole owner.
        AZStd::vector<AZStd::unique_ptr<AZ::Attribute>> m_editDataAttributes;
        AZStd::unique_ptr<AtomToolsFramework::DynamicNodeManager> m_dynamicNodeManager;
        AZStd::unique_ptr<AtomToolsFramework::AssetStatusReporterSystem> m_assetStatusReporterSystem;
        AZStd::shared_ptr<AtomToolsFramework::GraphTemplateFileDataCache> m_graphTemplateFileDataCache;
        AZStd::shared_ptr<GraphModel::GraphContext> m_graphContext;
        AZStd::shared_ptr<GraphModel::GraphContext> m_passGraphContext;
        AZStd::vector<AZ::Uuid> m_materialNodeConfigs;
        AZStd::vector<AZ::Uuid> m_passNodeConfigs;
        GraphKind m_activeGraphKind = GraphKind::Material;
        AtomToolsFramework::GraphViewSettingsPtr m_graphViewSettingsPtr;
        AZStd::unique_ptr<GraphCanvas::StyleManager> m_styleManager;
        AZStd::unique_ptr<QTranslator> m_translator;
    };
}
