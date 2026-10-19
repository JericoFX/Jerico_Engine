/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Materials/Base.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/RTTI.h>

#include <JE/Editor/Render/RPI/Edit/Material/MaterialSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <JE/Editor/Materials/Document/MaterialDocumentRequestBus.h>

namespace UnitTest { class MaterialDocumentTests; }

namespace JE::Editor
{
    //! MaterialDocument provides an API for modifying and saving material document properties.
    class JE_EDITOR_MATERIALS_API MaterialDocument
        : public AtomToolsFramework::AtomToolsDocument
        , public MaterialDocumentRequestBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_RTTI(MaterialDocument, "{90299628-AD02-4FEB-9527-7278FA2817AD}", AtomToolsFramework::AtomToolsDocument);
        AZ_CLASS_ALLOCATOR(MaterialDocument, AZ::SystemAllocator);
        AZ_DISABLE_COPY_MOVE(MaterialDocument);

        static void Reflect(AZ::ReflectContext* context);

        //! Editor history belongs to the host session; standalone tools retain document-local history.
        enum class HistoryOwner { Standalone, Editor };

        MaterialDocument() = default;
        MaterialDocument(const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo,
            HistoryOwner historyOwner = HistoryOwner::Standalone);
        virtual ~MaterialDocument();

        // AtomToolsFramework::AtomToolsDocument overrides...
        static AtomToolsFramework::DocumentTypeInfo BuildDocumentTypeInfo(HistoryOwner historyOwner = HistoryOwner::Standalone);
        AtomToolsFramework::DocumentObjectInfoVector GetObjectInfo() const override;
        bool Open(const AZStd::string& loadPath) override;
        bool Save() override;
        bool SaveAsCopy(const AZStd::string& savePath) override;
        bool SaveAsChild(const AZStd::string& savePath) override;
        bool IsModified() const override;
        bool CanSaveAsChild() const override;
        bool BeginEdit() override;
        bool EndEdit() override;

        // MaterialDocumentRequestBus::Handler overrides...
        AZ::Data::Asset<JE::Render::RPI::MaterialAsset> GetAsset() const override;
        AZ::Data::Instance<JE::Render::RPI::Material> GetInstance() const override;
        const JE::Render::RPI::MaterialSourceData* GetMaterialSourceData() const override;
        const JE::Render::RPI::MaterialTypeSourceData* GetMaterialTypeSourceData() const override;
        void SetPropertyValue(const AZStd::string& propertyId, const AZStd::any& value) override;
        const AZStd::any& GetPropertyValue(const AZStd::string& propertyId) const override;

    private:
        friend class UnitTest::MaterialDocumentTests;
        const HistoryOwner m_historyOwner = HistoryOwner::Standalone;

        // Predicate for evaluating properties
        using PropertyFilterFunction = AZStd::function<bool(const AtomToolsFramework::DynamicProperty&)>;

        // Map of raw property values for undo/redo comparison and storage
        using PropertyValueMap = AZStd::unordered_map<AZ::Name, AZStd::any>;

        // AZ::SystemTickBus overrides...
        void OnSystemTick() override;

        bool SaveSourceData(JE::Render::RPI::MaterialSourceData& sourceData, PropertyFilterFunction propertyFilter) const;

        // AtomToolsFramework::AtomToolsDocument overrides...
        void Clear() override;
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
        bool ReopenRecordState() override;
        bool ReopenRestoreState() override;

        void Recompile();

        bool LoadMaterialSourceData();
        bool LoadMaterialTypeSourceData();

        void RestorePropertyValues(const PropertyValueMap& propertyValues);

        bool AddEditorMaterialFunctors(
            const AZStd::vector<JE::Render::RPI::Ptr<JE::Render::RPI::MaterialFunctorSourceDataHolder>>& functorSourceDataHolders,
            const JE::Render::RPI::MaterialNameContext& nameContext);

        // Run editor material functor to update editor metadata.
        // @param dirtyFlags indicates which properties have changed, and thus which MaterialFunctors need to be run.
        void RunEditorMaterialFunctors(JE::Render::RPI::MaterialPropertyFlags dirtyFlags);

        // Convert a dynamic property group pointer into generic document object info used to populate the inspector
        AtomToolsFramework::DocumentObjectInfo GetObjectInfoFromDynamicPropertyGroup(
            const AtomToolsFramework::DynamicPropertyGroup* group) const;

        // In order traversal of dynamic property groups
        bool TraverseGroups(
            AZStd::vector<AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>>& groups,
            AZStd::function<bool(AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>&)> callback);

        // In order traversal of dynamic property groups
        bool TraverseGroups(
            const AZStd::vector<AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>>& groups,
            AZStd::function<bool(const AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>&)> callback) const;

        // Traverses dynamic property groups to find a property with a specific ID
        AtomToolsFramework::DynamicProperty* FindProperty(const AZ::Name& propertyId);

        // Traverses dynamic property groups to find a property with a specific ID
        const AtomToolsFramework::DynamicProperty* FindProperty(const AZ::Name& propertyId) const;

        // Material asset generated from source data, used to get the final values for properties to be assigned to the document 
        AZ::Data::Asset<JE::Render::RPI::MaterialAsset> m_materialAsset;

        // Material instance is only needed to run editor functors and is assigned directly to the viewport model to reflect real time
        // changes to material property values
        AZ::Data::Instance<JE::Render::RPI::Material> m_materialInstance;

        // If material instance value(s) were modified, do we need to recompile on next tick?
        bool m_compilePending = false;

        // Material functors that run in editor. See MaterialFunctor.h for details.
        AZStd::vector<JE::Render::RPI::Ptr<JE::Render::RPI::MaterialFunctor>> m_editorFunctors;

        // Material type source data used to enumerate all properties and populate the document
        JE::Render::RPI::MaterialTypeSourceData m_materialTypeSourceData;

        // Material source data with property values that override the material type
        JE::Render::RPI::MaterialSourceData m_materialSourceData;

        // State of property values prior to an edit, used for restoration during undo
        PropertyValueMap m_propertyValuesBeforeEdit;

        // State of property values prior to reopen
        PropertyValueMap m_propertyValuesBeforeReopen;

        // A container of root level dynamic property groups that represents the reflected, editable data within the document.
        // These groups will be mapped to document object info so they can populate and be edited directly in the inspector.
        AZStd::vector<AZStd::shared_ptr<AtomToolsFramework::DynamicPropertyGroup>> m_groups;

        // Dummy default value returned whenever a property cannot be located
        AZStd::any m_invalidValue;
    };
} // namespace JE::Editor
