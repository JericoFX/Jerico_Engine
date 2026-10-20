/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Editor/Render/RPI/Edit/Common/JsonUtils.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertyId.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialUtils.h>
#include <JE/Render/RPI/Public/Material/Material.h>
#include <JE/Render/RPI/Reflect/Material/MaterialFunctor.h>
#include <JE/Render/RPI/Reflect/Material/MaterialNameContext.h>
#include <JE/Render/RPI/Reflect/Material/MaterialPropertiesLayout.h>
#include <JE/Editor/Tools/Document/AtomToolsDocumentNotificationBus.h>
#include <JE/Editor/Tools/Util/MaterialPropertyUtil.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <JE/Editor/Tools/Util/Util.h>
#include <AzCore/Instance/Instance.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <JE/Editor/Materials/Document/MaterialDocument.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(JE_EDITOR_MATERIALS_API, JE::Editor::MaterialDocumentRequests);

namespace JE::Editor
{
    namespace
    {
        using PropertyValueMap = AZStd::unordered_map<AZ::Name, AZStd::any>;

        void NormalizeMaterialSourceReferences(JE::Render::RPI::MaterialSourceData& source, const AZStd::string& path)
        {
            source.m_parentMaterial = JE::Render::RPI::AssetUtils::ResolvePathReference(path, source.m_parentMaterial);
            source.m_materialType = JE::Render::RPI::AssetUtils::ResolvePathReference(path, source.m_materialType);
            source.m_materialType = JE::Render::RPI::MaterialUtils::PredictOriginalMaterialTypeSourcePath(source.m_materialType);
        }

        // Resolve the retained session document at replay time. Commands never own a document pointer.
        bool RestoreDocumentSnapshot(const AZ::Uuid& documentId, const AZStd::string& sourcePath,
            const PropertyValueMap& expectedValues, const PropertyValueMap& values)
        {
            auto* document = JE::Editor::Tools::AtomToolsDocumentRequestBus::FindFirstHandler(documentId);
            auto* material = MaterialDocumentRequestBus::FindFirstHandler(documentId);
            if (!document || !material || sourcePath.empty() || document->GetAbsolutePath() != sourcePath)
            {
                AZ_Error("MaterialDocument", false, "Cannot replay material edit: document was destroyed, closed, or changed source path.");
                return false;
            }

            // A material type may change during source reload. Validate every property before applying any delta.
            for (const auto& [name, value] : values)
            {
                const auto& current = material->GetPropertyValue(name.GetStringView());
                if (current.empty() || current.type() != value.type())
                {
                    AZ_Error("MaterialDocument", false, "Cannot replay material edit: property '%s' is missing or changed type.", name.GetCStr());
                    return false;
                }
                const auto expected = expectedValues.find(name);
                if (expected == expectedValues.end() || !JE::Editor::Tools::ArePropertyValuesEqual(current, expected->second))
                {
                    AZ_Error("MaterialDocument", false, "Cannot replay material edit: property '%s' changed outside the retained history.", name.GetCStr());
                    return false;
                }
            }
            for (const auto& [name, value] : values)
            {
                MaterialDocumentRequestBus::Event(documentId, &MaterialDocumentRequestBus::Events::SetPropertyValue,
                    name.GetStringView(), value);
            }
            return true;
        }

        class MaterialDocumentUndo final : public AzToolsFramework::UndoSystem::URSequencePoint
        {
        public:
            AZ_CLASS_ALLOCATOR(MaterialDocumentUndo, AZ::SystemAllocator);

            MaterialDocumentUndo(const AZ::Uuid& documentId, const AZStd::string& sourcePath,
                PropertyValueMap undoValues, PropertyValueMap redoValues)
                : URSequencePoint("Edit Material Document")
                , m_documentId(documentId)
                , m_sourcePath(sourcePath)
                , m_undoValues(AZStd::move(undoValues))
                , m_redoValues(AZStd::move(redoValues))
            {
            }

            bool Changed() const override { return !m_undoValues.empty(); }
            void Undo() override { RestoreDocumentSnapshot(m_documentId, m_sourcePath, m_redoValues, m_undoValues); }
            void Redo() override { RestoreDocumentSnapshot(m_documentId, m_sourcePath, m_undoValues, m_redoValues); }

        private:
            const AZ::Uuid m_documentId;
            const AZStd::string m_sourcePath;
            const PropertyValueMap m_undoValues;
            const PropertyValueMap m_redoValues;
        };
    }

    void MaterialDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            if (serialize->IsRemovingReflection() || !serialize->FindClassData(azrtti_typeid<MaterialDocument>()))
            {
                serialize->Class<MaterialDocument, JE::Editor::Tools::AtomToolsDocument>()
                    ->Version(0);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            behaviorContext && (behaviorContext->IsRemovingReflection() ||
                behaviorContext->m_ebuses.find("MaterialDocumentRequestBus") == behaviorContext->m_ebuses.end()))
        {
            behaviorContext->EBus<MaterialDocumentRequestBus>("MaterialDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialeditor")
                ->Event("SetPropertyValue", &MaterialDocumentRequestBus::Events::SetPropertyValue)
                ->Event("GetPropertyValue", &MaterialDocumentRequestBus::Events::GetPropertyValue);
        }
    }

    MaterialDocument::MaterialDocument(const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& documentTypeInfo,
        HistoryOwner historyOwner)
        : JE::Editor::Tools::AtomToolsDocument(toolId, documentTypeInfo)
        , m_historyOwner(historyOwner)
    {
        MaterialDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialDocument::~MaterialDocument()
    {
        MaterialDocumentRequestBus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    AZ::Data::Asset<JE::Render::RPI::MaterialAsset> MaterialDocument::GetAsset() const
    {
        return m_materialAsset;
    }

    AZ::Data::Instance<JE::Render::RPI::Material> MaterialDocument::GetInstance() const
    {
        return m_materialInstance;
    }

    const JE::Render::RPI::MaterialSourceData* MaterialDocument::GetMaterialSourceData() const
    {
        return &m_materialSourceData;
    }

    const JE::Render::RPI::MaterialTypeSourceData* MaterialDocument::GetMaterialTypeSourceData() const
    {
        return &m_materialTypeSourceData;
    }

    void MaterialDocument::SetPropertyValue(const AZStd::string& propertyId, const AZStd::any& value)
    {
        const AZ::Name propertyName(propertyId);

        JE::Editor::Tools::DynamicProperty* foundProperty = {};
        TraverseGroups(m_groups, [&, this](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (property.GetId() == propertyName)
                {
                    foundProperty = &property;

                    if (m_materialInstance)
                    {
                        // This first converts to an acceptable runtime type in case the value came from script
                        const JE::Render::RPI::MaterialPropertyValue propertyValue = JE::Editor::Tools::ConvertToRuntimeType(value);

                        property.SetValue(JE::Editor::Tools::ConvertToEditableType(propertyValue));

                        const auto propertyIndex = m_materialInstance->FindPropertyIndex(propertyName);
                        if (!propertyIndex.IsNull())
                        {
                            if (m_materialInstance->SetPropertyValue(propertyIndex, propertyValue))
                            {
                                JE::Render::RPI::MaterialPropertyFlags dirtyFlags = m_materialInstance->GetPropertyDirtyFlags();

                                Recompile();
                                RunEditorMaterialFunctors(dirtyFlags);
                            }
                        }
                    }

                    JE::Editor::Tools::AtomToolsDocumentNotificationBus::Event(
                        m_toolId, &JE::Editor::Tools::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoChanged, m_id,
                        GetObjectInfoFromDynamicPropertyGroup(group.get()), false);

                    JE::Editor::Tools::AtomToolsDocumentNotificationBus::Event(
                        m_toolId, &JE::Editor::Tools::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
                    return false;
                }
            }
            return true;
        });

        if (!foundProperty)
        {
            AZ_Error("MaterialDocument", false, "Document property could not be found: '%s'.", propertyId.c_str());
        }
    }

    const AZStd::any& MaterialDocument::GetPropertyValue(const AZStd::string& propertyId) const
    {
        auto property = FindProperty(AZ::Name(propertyId));
        if (!property)
        {
            AZ_Error("MaterialDocument", false, "Document property could not be found: '%s'.", propertyId.c_str());
            return m_invalidValue;
        }

        return property->GetValue();
    }

    JE::Editor::Tools::DocumentTypeInfo MaterialDocument::BuildDocumentTypeInfo(HistoryOwner historyOwner)
    {
        JE::Editor::Tools::DocumentTypeInfo documentType;
        documentType.m_documentTypeName = "Material";
        documentType.m_documentFactoryCallback = [historyOwner](const AZ::Crc32& toolId, const JE::Editor::Tools::DocumentTypeInfo& documentTypeInfo) {
            return aznew MaterialDocument(toolId, documentTypeInfo, historyOwner); };
        documentType.m_supportedExtensionsToCreate.push_back({ "Material Type", JE::Render::RPI::MaterialTypeSourceData::Extension });
        documentType.m_supportedExtensionsToCreate.push_back({ "Material", JE::Render::RPI::MaterialSourceData::Extension });
        documentType.m_supportedExtensionsToOpen.push_back({ "Material Type", JE::Render::RPI::MaterialTypeSourceData::Extension });
        documentType.m_supportedExtensionsToOpen.push_back({ "Material", JE::Render::RPI::MaterialSourceData::Extension });
        documentType.m_supportedExtensionsToSave.push_back({ "Material", JE::Render::RPI::MaterialSourceData::Extension });
        documentType.m_defaultDocumentTemplate =
            JE::Editor::Tools::GetPathWithoutAlias(JE::Editor::Tools::GetSettingsValue<AZStd::string>(
                "/O3DE/Atom/MaterialEditor/DefaultMaterialType",
                "@gemroot:Atom_Feature_Common@/Assets/Materials/Types/StandardPBR.materialtype"));
        return documentType;
    }

    JE::Editor::Tools::DocumentObjectInfoVector MaterialDocument::GetObjectInfo() const
    {
        JE::Editor::Tools::DocumentObjectInfoVector objects = AtomToolsDocument::GetObjectInfo();
        objects.reserve(objects.size() + m_groups.size());

        for (const auto& group : m_groups)
        {
            objects.push_back(GetObjectInfoFromDynamicPropertyGroup(group.get()));
        }

        return objects;
    }

    bool MaterialDocument::Save()
    {
        if (!AtomToolsDocument::Save())
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        // populate sourceData with modified or overridden properties and save object
        JE::Render::RPI::MaterialSourceData sourceData;
        if (m_materialAsset.IsReady() &&
            m_materialAsset->GetMaterialTypeAsset().IsReady())
        {
            sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        }
        sourceData.m_materialType = JE::Editor::Tools::GetPathToExteralReference(m_absolutePath, m_materialSourceData.m_materialType);
        sourceData.m_parentMaterial = JE::Editor::Tools::GetPathToExteralReference(m_absolutePath, m_materialSourceData.m_parentMaterial);
        auto propertyFilter = [](const JE::Editor::Tools::DynamicProperty& property) {
            return !JE::Editor::Tools::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue);
        };

        if (!SaveSourceData(sourceData, propertyFilter))
        {
            return SaveFailed();
        }

        m_materialSourceData = AZStd::move(sourceData);
        NormalizeMaterialSourceReferences(m_materialSourceData, m_absolutePath);

        // after saving, reset to a clean state
        TraverseGroups(m_groups, [&](auto& group) {
            for (auto& property : group->m_properties)
            {
                auto propertyConfig = property.GetConfig();
                propertyConfig.m_originalValue = property.GetValue();
                property.SetConfig(propertyConfig);
            }
            return true;
        });
        return SaveSucceeded();
    }

    bool MaterialDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        // populate sourceData with modified or overridden properties and save object
        JE::Render::RPI::MaterialSourceData sourceData;
        if (m_materialAsset.IsReady() &&
            m_materialAsset->GetMaterialTypeAsset().IsReady())
        {
            sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        }
        sourceData.m_materialType = JE::Editor::Tools::GetPathToExteralReference(m_savePathNormalized, m_materialSourceData.m_materialType);
        sourceData.m_parentMaterial = JE::Editor::Tools::GetPathToExteralReference(m_savePathNormalized, m_materialSourceData.m_parentMaterial);
        auto propertyFilter = [](const JE::Editor::Tools::DynamicProperty& property) {
            return !JE::Editor::Tools::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue);
        };

        if (!SaveSourceData(sourceData, propertyFilter))
        {
            return SaveFailed();
        }

        // If the document is saved to a new file we need to reopen the new document to update assets, paths, property deltas.
        if (!Open(m_savePathNormalized))
        {
            return SaveFailed();
        }

        return SaveSucceeded();
    }

    bool MaterialDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        // populate sourceData with modified or overridden properties and save object
        JE::Render::RPI::MaterialSourceData sourceData;
        if (m_materialAsset.IsReady() &&
            m_materialAsset->GetMaterialTypeAsset().IsReady())
        {
            sourceData.m_materialTypeVersion = m_materialAsset->GetMaterialTypeAsset()->GetVersion();
        }
        sourceData.m_materialType = JE::Editor::Tools::GetPathToExteralReference(m_savePathNormalized, m_materialSourceData.m_materialType);

        // Only assign a parent path if the source was a .material
        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), JE::Render::RPI::MaterialSourceData::Extension))
        {
            sourceData.m_parentMaterial = JE::Editor::Tools::GetPathToExteralReference(m_savePathNormalized, m_absolutePath);
        }

        auto propertyFilter = [](const JE::Editor::Tools::DynamicProperty& property) {
            return !JE::Editor::Tools::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_originalValue);
        };

        if (!SaveSourceData(sourceData, propertyFilter))
        {
            return SaveFailed();
        }

        // If the document is saved to a new file we need to reopen the new document to update assets, paths, property deltas.
        if (!Open(m_savePathNormalized))
        {
            return SaveFailed();
        }

        return SaveSucceeded();
    }

    bool MaterialDocument::IsModified() const
    {
        bool result = false;
        TraverseGroups(m_groups, [&](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (!JE::Editor::Tools::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_originalValue))
                {
                    result = true;
                    return false;
                }
            }
            return true;
        });
        return result;
    }

    bool MaterialDocument::CanSaveAsChild() const
    {
        return true;
    }

    bool MaterialDocument::BeginEdit()
    {
        if (m_historyOwner == HistoryOwner::Editor)
        {
            AzToolsFramework::UndoSystem::UndoStack* stack = nullptr;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(stack,
                &AzToolsFramework::ToolsApplicationRequestBus::Events::GetUndoStack);
            bool replaying = false;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(replaying,
                &AzToolsFramework::ToolsApplicationRequestBus::Events::IsDuringUndoRedo);
            if (!stack || replaying || m_absolutePath.empty())
            {
                AZ_Error("MaterialDocument", false, "Material edit requires an open document and an available Editor Undo stack outside replay.");
                return false;
            }
        }
        // Save the current properties as a momento for undo before any changes are applied
        m_propertyValuesBeforeEdit.clear();
        TraverseGroups(m_groups, [this](auto& group) {
            for (auto& property : group->m_properties)
            {
                m_propertyValuesBeforeEdit[property.GetId()] = property.GetValue();
            }
            return true;
        });
        return true;
    }

    bool MaterialDocument::EndEdit()
    {
        PropertyValueMap propertyValuesForUndo;
        PropertyValueMap propertyValuesForRedo;

        // After editing has completed, check to see if properties have changed so the deltas can be recorded in the history
        for (const auto& propertyBeforeEditPair : m_propertyValuesBeforeEdit)
        {
            const auto& propertyName = propertyBeforeEditPair.first;
            const auto& propertyValueForUndo = propertyBeforeEditPair.second;
            const auto& propertyValueForRedo = GetPropertyValue(propertyName.GetStringView());
            if (!JE::Editor::Tools::ArePropertyValuesEqual(propertyValueForUndo, propertyValueForRedo))
            {
                propertyValuesForUndo[propertyName] = propertyValueForUndo;
                propertyValuesForRedo[propertyName] = propertyValueForRedo;
            }
        }

        if (!propertyValuesForUndo.empty() && !propertyValuesForRedo.empty())
        {
            if (m_historyOwner == HistoryOwner::Editor)
            {
                AzToolsFramework::UndoSystem::UndoStack* stack = nullptr;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(stack,
                    &AzToolsFramework::ToolsApplicationRequestBus::Events::GetUndoStack);
                if (!stack)
                {
                    RestorePropertyValues(propertyValuesForUndo);
                    m_propertyValuesBeforeEdit.clear();
                    AZ_Error("MaterialDocument", false, "Material edit cancelled: Editor Undo stack is unavailable.");
                    return false;
                }
                AzToolsFramework::ScopedUndoBatch undoBatch("Edit Material Document");
                auto* command = aznew MaterialDocumentUndo(m_id, m_absolutePath,
                    AZStd::move(propertyValuesForUndo), AZStd::move(propertyValuesForRedo));
                command->SetParent(undoBatch.GetUndoBatch());
            }
            else
            {
                AddUndoRedoHistory(
                    [this, propertyValuesForUndo]() { RestorePropertyValues(propertyValuesForUndo); },
                    [this, propertyValuesForRedo]() { RestorePropertyValues(propertyValuesForRedo); });
            }
        }

        m_propertyValuesBeforeEdit.clear();
        return true;
    }

    void MaterialDocument::OnSystemTick()
    {
        if (m_compilePending)
        {
            if (m_materialInstance && m_materialInstance->Compile())
            {
                m_compilePending = false;
                AZ::SystemTickBus::Handler::BusDisconnect();
            }
        }
    }

    bool MaterialDocument::SaveSourceData(JE::Render::RPI::MaterialSourceData& sourceData, PropertyFilterFunction propertyFilter) const
    {
        bool addPropertiesResult = true;

        // populate sourceData with properties that meet the filter
        m_materialTypeSourceData.EnumerateProperties([&](const auto& propertyDefinition, const JE::Render::RPI::MaterialNameContext& nameContext) {

            AZ::Name propertyId{propertyDefinition->GetName()};
            nameContext.ContextualizeProperty(propertyId);

            const auto property = FindProperty(propertyId);
            if (property && propertyFilter(*property))
            {
                JE::Render::RPI::MaterialPropertyValue propertyValue = JE::Editor::Tools::ConvertToRuntimeType(property->GetValue());
                if (propertyValue.IsValid())
                {
                    if (!JE::Editor::Tools::ConvertToExportFormat(m_savePathNormalized, propertyId, *propertyDefinition, propertyValue))
                    {
                        AZ_Error("MaterialDocument", false, "Document property could not be converted: '%s' in '%s'.", propertyId.GetCStr(), m_absolutePath.c_str());
                        addPropertiesResult = false;
                        return false;
                    }

                    // Match the JSON source representation retained after Save.
                    if (propertyValue.Is<AZ::Color>())
                    {
                        propertyValue = propertyValue.CastToType(azrtti_typeid<AZ::Vector4>());
                    }

                    sourceData.SetPropertyValue(propertyId, propertyValue);
                }
            }
            return true;
        });

        if (!addPropertiesResult)
        {
            AZ_Error("MaterialDocument", false, "Document properties could not be saved: '%s'.", m_savePathNormalized.c_str());
            return false;
        }

        // Copy the description property to the outgoing source data
        if (const AZStd::any descriptionProperty = GetPropertyValue("overview.materialDescription");
            descriptionProperty.is<AZStd::string>())
        {
            sourceData.m_description = AZStd::any_cast<AZStd::string>(descriptionProperty);
        }

        if (!JE::Render::RPI::JsonUtils::SaveObjectToFile(m_savePathNormalized, sourceData))
        {
            AZ_Error("MaterialDocument", false, "Document could not be saved: '%s'.", m_savePathNormalized.c_str());
            return false;
        }

        return true;
    }

    bool MaterialDocument::Open(const AZStd::string& loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
        {
            return false;
        }

        // The material document can load both material source data and material type source data files. Saving material type documents is
        // not supported but they can be used to save a child or create a new material from the material type. This could also be extended
        // to load material product assets, like the material instance editor on the material component. Those would also not be savable
        // but could be used to create material source file, like the material component UI.
        if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), JE::Render::RPI::MaterialSourceData::Extension))
        {
            if (!LoadMaterialSourceData())
            {
                return OpenFailed();
            }
        }
        else if (AzFramework::StringFunc::Path::IsExtension(m_absolutePath.c_str(), JE::Render::RPI::MaterialTypeSourceData::Extension))
        {
            if (!LoadMaterialTypeSourceData())
            {
                return OpenFailed();
            }
        }
        else
        {
            AZ_Error("MaterialDocument", false, "Document extension not supported: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        const bool elevateWarnings = false;

        // In order to support automation, general usability, and 'save as' functionality, the user must not have to wait
        // for their JSON file to be cooked by the asset processor before opening or editing it.
        // We need to reduce or remove dependency on the asset processor. In order to get around the bottleneck for now,
        // we can create the asset dynamically from the source data.
        // Long term, the material document should not be concerned with assets at all. The viewport window should be the
        // only thing concerned with assets or instances.
        auto materialAssetResult = m_materialSourceData.CreateMaterialAssetFromSourceData(
            AZ::Uuid::CreateRandom(), m_absolutePath, elevateWarnings, &m_sourceDependencies);
        if (!materialAssetResult)
        {
            AZ_Error("MaterialDocument", false, "Material asset could not be created from source data: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        m_materialAsset = materialAssetResult.GetValue();
        if (!m_materialAsset.IsReady())
        {
            AZ_Error("MaterialDocument", false, "Material asset is not ready: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        const auto& materialTypeAsset = m_materialAsset->GetMaterialTypeAsset();
        if (!materialTypeAsset.IsReady())
        {
            AZ_Error("MaterialDocument", false, "Material type asset is not ready: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        // The parent material asset is only needed to retrieve property values for comparison.
        AZStd::span<const JE::Render::RPI::MaterialPropertyValue> parentPropertyValues = materialTypeAsset->GetDefaultPropertyValues();
        AZ::Data::Asset<JE::Render::RPI::MaterialAsset> parentMaterialAsset;
        if (!m_materialSourceData.m_parentMaterial.empty())
        {
            JE::Render::RPI::MaterialSourceData parentMaterialSourceData;
            auto loadResult = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(m_materialSourceData.m_parentMaterial);
            if (!loadResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent source data could not be loaded for: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return OpenFailed();
            }

            parentMaterialSourceData = loadResult.TakeValue();

            const auto parentMaterialAssetIdResult = JE::Render::RPI::AssetUtils::MakeAssetId(m_materialSourceData.m_parentMaterial, 0);
            if (!parentMaterialAssetIdResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent asset ID could not be created: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return OpenFailed();
            }

            // In order to avoid reliance on the asset processor, the material asset is generated in memory, directly from source files.
            auto parentMaterialAssetResult = parentMaterialSourceData.CreateMaterialAssetFromSourceData(
                parentMaterialAssetIdResult.GetValue(), m_materialSourceData.m_parentMaterial, true);
            if (!parentMaterialAssetResult)
            {
                AZ_Error("MaterialDocument", false, "Material parent asset could not be created from source data: '%s'.", m_materialSourceData.m_parentMaterial.c_str());
                return OpenFailed();
            }

            parentMaterialAsset = parentMaterialAssetResult.GetValue();
            parentPropertyValues = parentMaterialAsset->GetPropertyValues();
        }

        // A material instance needs to be created from the loaded asset to execute functors and be able to modify properties in real time
        // on the object in the viewport. Now that there is much better support for hot reloading, and material assets cook fairly
        // quickly, this direct connection to the viewport instance may not be required. It will still be required for functors. The
        // instance will fail to create a new document will not open if the material asset has bad texture or material type references.
        m_materialInstance = JE::Render::RPI::Material::Create(m_materialAsset);
        if (!m_materialInstance)
        {
            AZ_Error("MaterialDocument", false, "Material instance could not be created: '%s'.", m_absolutePath.c_str());
            return OpenFailed();
        }

        // Pipeline State Object changes are always allowed in the material editor because it only runs on developer systems where such
        // changes are supported at runtime.
        m_materialInstance->SetPsoHandlingOverride(JE::Render::RPI::MaterialPropertyPsoHandling::Allowed);

        // Inserting hardcoded properties to display material type, parent material, description, UV set names, and other information at the
        // top of the inspector. Dynamic properties were originally created to generically adapt and edit JSON and other non-standard
        // reflected data using the RPE. Most of these hardcoded properties are readonly. As that changes, it may be cleaner to add
        // explicit functions and reflection for things that are more complicated to edit like parent material and material type.
        auto createHeadingPropertyConfig = [](const AZStd::string& group, const AZStd::string& name, const AZStd::string& description,
            const AZStd::any& value, bool readOnly)
        {
            JE::Editor::Tools::DynamicPropertyConfig propertyConfig;
            propertyConfig.m_name = name;
            propertyConfig.m_displayName = JE::Editor::Tools::GetDisplayNameFromText(propertyConfig.m_name);
            propertyConfig.m_groupName = group;
            propertyConfig.m_groupDisplayName = JE::Editor::Tools::GetDisplayNameFromText(propertyConfig.m_groupName);
            propertyConfig.m_id = propertyConfig.m_groupName + "." + name;
            propertyConfig.m_description = description;
            propertyConfig.m_parentValue = propertyConfig.m_originalValue = propertyConfig.m_defaultValue = value;
            propertyConfig.m_readOnly = readOnly;
            propertyConfig.m_showThumbnail = true;
            return propertyConfig;
        };

        m_groups.emplace_back(aznew JE::Editor::Tools::DynamicPropertyGroup);
        m_groups.back()->m_name = "overview";
        m_groups.back()->m_displayName = "Overview";
        m_groups.back()->m_description = "Overview of the current material and its dependencies";

        m_groups.back()->m_properties.emplace_back(createHeadingPropertyConfig(
            "overview",
            "materialType",
            AZStd::string::format(
                "The material type defines the layout, properties, default values, shader connections, and other data needed to create and "
                "edit a material.\n\nDescription of %s:\n%s",
                JE::Editor::Tools::GetDisplayNameFromPath(m_materialSourceData.m_materialType).c_str(),
                m_materialTypeSourceData.m_description.c_str()),
            AZStd::any(materialTypeAsset),
            true));

        m_groups.back()->m_properties.emplace_back(createHeadingPropertyConfig(
            "overview",
            "parentMaterial",
            "The parent material provides an initial configuration whose properties are inherited and overriden by a derived material.",
            AZStd::any(parentMaterialAsset),
            true));

        m_groups.back()->m_properties.emplace_back(createHeadingPropertyConfig(
            "overview",
            "materialDescription",
            "Description of the selected material.",
            AZStd::any(m_materialSourceData.m_description),
            false));

        // Inserting a hard coded property group to display UV channels specified in the material type.
        m_groups.emplace_back(aznew JE::Editor::Tools::DynamicPropertyGroup);
        m_groups.back()->m_name = UvGroupName;
        m_groups.back()->m_displayName = "UV Sets";
        m_groups.back()->m_description = "UV set names in this material, which can be renamed to match those in the model.";

        const JE::Render::RPI::MaterialUvNameMap& uvNameMap = materialTypeAsset->GetUvNameMap();
        for (const JE::Render::RPI::UvNamePair& uvNamePair : uvNameMap)
        {
            const AZStd::string shaderInput = uvNamePair.m_shaderInput.ToString();
            const AZStd::string uvName = uvNamePair.m_uvName.GetStringView();
            m_groups.back()->m_properties.emplace_back(createHeadingPropertyConfig(UvGroupName, shaderInput, shaderInput, AZStd::any(uvName), true));
        }

        // Populate the property map from a combination of source data and assets
        // Assets must still be used for now because they contain the final accumulated value after all other materials
        // in the hierarchy are applied
        bool enumerateResult = m_materialTypeSourceData.EnumeratePropertyGroups(
            [this, &parentPropertyValues](const JE::Render::RPI::MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack)
            {
                using namespace JE::Render::RPI;

                const MaterialTypeSourceData::PropertyGroup* propertyGroup = propertyGroupStack.back();

                MaterialNameContext groupNameContext = MaterialTypeSourceData::MakeMaterialNameContext(propertyGroupStack);

                if (!AddEditorMaterialFunctors(propertyGroup->GetFunctors(), groupNameContext))
                {
                    return false;
                }

                // Build a container of all of the group and display names accumulated while enumerating the group hierarchy. These will be
                // joined together for assembling full property IDs and group display names.
                AZStd::vector<AZStd::string> groupNameVector;
                groupNameVector.reserve(propertyGroupStack.size());

                AZStd::vector<AZStd::string> groupDisplayNameVector;
                groupDisplayNameVector.reserve(propertyGroupStack.size());

                for (auto& propertyGroupStackItem : propertyGroupStack)
                {
                    groupNameVector.push_back(propertyGroupStackItem->GetName());
                    groupDisplayNameVector.push_back(propertyGroupStackItem->GetDisplayName());
                }

                // Create a dynamic property group that will be managed by the document and used to display the properties in the inspector.
                AZStd::shared_ptr<JE::Editor::Tools::DynamicPropertyGroup> dynamicPropertyGroup;
                dynamicPropertyGroup.reset(aznew JE::Editor::Tools::DynamicPropertyGroup);

                // Copy details about this property group from the material type property group definition. Recombine the group name and
                // display name vectors so that the complete hierarchy will be displayed in the UI and available for creating property IDs.
                AzFramework::StringFunc::Join(dynamicPropertyGroup->m_name, groupNameVector.begin(), groupNameVector.end(), ".");
                AzFramework::StringFunc::Join(dynamicPropertyGroup->m_displayName, groupDisplayNameVector.begin(), groupDisplayNameVector.end(), " | ");

                if (dynamicPropertyGroup->m_displayName.empty())
                {
                    dynamicPropertyGroup->m_displayName =
                        !propertyGroup->GetDisplayName().empty() ? propertyGroup->GetDisplayName() : propertyGroup->GetName();
                }

                dynamicPropertyGroup->m_description = propertyGroup->GetDescription();
                if (dynamicPropertyGroup->m_description.empty())
                {
                    dynamicPropertyGroup->m_description = dynamicPropertyGroup->m_displayName;
                }

                // All of the material type properties must be adapted for display in the ui. This is done by converting them into a dynamic
                // property class that can be used to display and edit multiple types.
                for (const auto& propertyDefinition : propertyGroup->GetProperties())
                {
                    JE::Editor::Tools::DynamicPropertyConfig propertyConfig;

                    // The property ID must be set up before calling the function to convert the rest of the material type property
                    // definition into the dynamic property config. The dynamic property config will set up a description that includes the
                    // ID.
                    propertyConfig.m_id = propertyDefinition->GetName();
                    groupNameContext.ContextualizeProperty(propertyConfig.m_id);

                    // A valid property index is required to look up property values in the material type and material asset property vectors.
                    const auto& propertyIndex = m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);
                    const bool propertyIndexInBounds =
                        propertyIndex.IsValid() && propertyIndex.GetIndex() < m_materialAsset->GetPropertyValues().size();

                    AZ_Warning(
                        "MaterialDocument",
                        propertyIndexInBounds,
                        "Failed to add material property '%s' to document '%s'.",
                        propertyConfig.m_id.GetCStr(),
                        m_absolutePath.c_str());

                    if (propertyIndexInBounds)
                    {
                        // Utility function converts most attributes from the property definition into a dynamic property config.
                        JE::Editor::Tools::ConvertToPropertyConfig(propertyConfig, *propertyDefinition);

                        // The utility function assigns a description from the property definition along with its name and display name.
                        // This will be displayed as the tooltip when dragging over the property in the inspector UI. The description is
                        // extended here so that the tooltip will display an image and additional information about the indicator that
                        // appears when properties are modified. The tooltip will automatically interpret the embedded HTML and display the
                        // image and formatting.
                        propertyConfig.m_description +=
                            "\n\n<img src=\':/Icons/changed_property.svg\'> An indicator icon will be shown to the left of properties with "
                            "overridden values that are different from the parent material, or material type if there is no parent.\n";

                        // The dynamic property uses the group name and display name to forward as attributes to the RPE and property asset
                        // control. The control will then use the attributes to display a context sensitive title when opening the asset
                        // picker for textures and other assets. Rather than using strings, this data could also be specified using
                        // AZStd::function.
                        propertyConfig.m_groupName = dynamicPropertyGroup->m_name;
                        propertyConfig.m_groupDisplayName = dynamicPropertyGroup->m_displayName;

                        // Enabling thumbnails will display a preview image next to an asset property in the RPE, if one is available.
                        propertyConfig.m_showThumbnail = true;

                        // Multiple values are recorded for the property, including the original value, default value, and parent value.
                        // These values are compared against each other to determine if an indicator needs to be displayed in the property
                        // inspector as well as which values get saved with the material.
                        propertyConfig.m_originalValue =
                            JE::Editor::Tools::ConvertToEditableType(m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                        propertyConfig.m_parentValue =
                            JE::Editor::Tools::ConvertToEditableType(parentPropertyValues[propertyIndex.GetIndex()]);

                        // The data change callback is invoked whenever the properties are modified in the inspector. The changes will be
                        // stored in the dynamic property automatically but need to be processed and applied to the material instance that's
                        // displayed in the viewport. This is also necessary to update and rerun functors.
                        propertyConfig.m_dataChangeCallback = [documentId = m_id, propertyId = propertyConfig.m_id](const AZStd::any& value)
                        {
                            MaterialDocumentRequestBus::Event(
                                documentId, &MaterialDocumentRequestBus::Events::SetPropertyValue, propertyId.GetStringView(), value);
                            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
                        };

                        dynamicPropertyGroup->m_properties.push_back(JE::Editor::Tools::DynamicProperty(propertyConfig));
                    }
                }

                // The group will not be added if no properties were added to it.
                if (!dynamicPropertyGroup->m_properties.empty())
                {
                    m_groups.push_back(dynamicPropertyGroup);
                }
                return true;
            });

        if (!enumerateResult)
        {
            return OpenFailed();
        }

        // Add material functors that are in the top-level functors list.
        JE::Render::RPI::MaterialNameContext materialNameContext; // There is no name context for top-level functors, only functors inside PropertyGroups
        if (!AddEditorMaterialFunctors(m_materialTypeSourceData.m_materialFunctorSourceData, materialNameContext))
        {
            return OpenFailed();
        }

        JE::Render::RPI::MaterialPropertyFlags dirtyFlags;
        dirtyFlags.set(); // Mark all properties as dirty since we just loaded the material and need to initialize property visibility
        RunEditorMaterialFunctors(dirtyFlags);

        return OpenSucceeded();
    }

    void MaterialDocument::SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID)
    {
        const auto sourcePath = JE::Render::RPI::AssetUtils::ResolvePathReference(scanFolder, relativePath);
        if (sourcePath == m_absolutePath &&
            AzFramework::StringFunc::Path::IsExtension(sourcePath.c_str(), JE::Render::RPI::MaterialSourceData::Extension))
        {
            // AP also reports the initial discovery of a newly created source. Compare only the last loaded/saved source,
            // not the dynamic properties, so an unchanged notification never discards unsaved edits or history.
            auto json = AZ::JsonSerializationUtils::ReadJsonFile(sourcePath, JE::Render::RPI::JsonUtils::DefaultMaxFileSize);
            if (json.IsSuccess())
            {
                using namespace AZ::JsonSerializationResult;
                const auto completed = [](ResultCode result)
                {
                    const auto outcome = result.GetOutcome();
                    return result.GetProcessing() == Processing::Completed &&
                        (outcome == Outcomes::Success || outcome == Outcomes::DefaultsUsed || outcome == Outcomes::PartialDefaults);
                };
                bool comparable = true;
                AZ::JsonDeserializerSettings settings;
                settings.m_reporting = [&comparable, &completed](
                    AZStd::string_view, ResultCode result, AZStd::string_view)
                {
                    // This is a local probe. Anything skipped, warned about or not fully loaded follows the normal reload path.
                    comparable = comparable && completed(result);
                    return result;
                };
                JE::Render::RPI::MaterialSourceData source;
                const auto result = AZ::JsonSerialization::Load(source, json.GetValue(), settings);
                if (comparable && completed(result))
                {
                    source.UpgradeLegacyFormat();
                    NormalizeMaterialSourceReferences(source, m_absolutePath);
                    const auto& loadedValues = m_materialSourceData.GetPropertyValues();
                    const auto& sourceValues = source.GetPropertyValues();
                    bool propertiesMatch = loadedValues.size() == sourceValues.size();
                    for (const auto& [name, value] : sourceValues)
                    {
                        const auto found = loadedValues.find(name);
                        if (found == loadedValues.end() || !(found->second == value))
                        {
                            propertiesMatch = false;
                            break;
                        }
                    }
                    if (propertiesMatch && source.m_description == m_materialSourceData.m_description &&
                        source.m_materialType == m_materialSourceData.m_materialType &&
                        source.m_parentMaterial == m_materialSourceData.m_parentMaterial &&
                        source.m_materialTypeVersion == m_materialSourceData.m_materialTypeVersion)
                    {
                        m_ignoreSourceFileChangeToSelf = false;
                        return;
                    }
                }
            }
            // A failed probe or real difference must not be hidden by a still-pending self-save notification.
            m_ignoreSourceFileChangeToSelf = false;
        }
        AtomToolsDocument::SourceFileChanged(AZStd::move(relativePath), AZStd::move(scanFolder), sourceUUID);
    }

    void MaterialDocument::Clear()
    {
        JE::Editor::Tools::AtomToolsDocument::Clear();

        AZ::SystemTickBus::Handler::BusDisconnect();

        m_materialAsset = {};
        m_materialInstance = {};
        m_compilePending = {};
        m_groups.clear();
        m_editorFunctors.clear();
        m_materialTypeSourceData = JE::Render::RPI::MaterialTypeSourceData();
        m_materialSourceData = JE::Render::RPI::MaterialSourceData();
        m_propertyValuesBeforeEdit.clear();
    }

    bool MaterialDocument::ReopenRecordState()
    {
        m_propertyValuesBeforeReopen.clear();
        TraverseGroups(m_groups, [this](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (!JE::Editor::Tools::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_parentValue))
                {
                    m_propertyValuesBeforeReopen[property.GetId()] = property.GetValue();
                }
            }
            return true;
        });
        return AtomToolsDocument::ReopenRecordState();
    }

    bool MaterialDocument::ReopenRestoreState()
    {
        RestorePropertyValues(m_propertyValuesBeforeReopen);
        m_propertyValuesBeforeReopen.clear();
        return AtomToolsDocument::ReopenRestoreState();
    }

    void MaterialDocument::Recompile()
    {
        if (!m_compilePending)
        {
            AZ::SystemTickBus::Handler::BusConnect();
            m_compilePending = true;
        }
    }

    bool MaterialDocument::LoadMaterialSourceData()
    {
        auto loadResult = JE::Render::RPI::MaterialUtils::LoadMaterialSourceData(m_absolutePath);
        if (!loadResult)
        {
            AZ_Error("MaterialDocument", false, "Material source data could not be loaded: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_materialSourceData = loadResult.TakeValue();

        // We always need the absolute path for the material type and parent material to load source data and resolving
        // relative paths when saving. This will convert and store them as absolute paths for use within the document.
        // If the material was previously saved with a reference to a material pipeline generated material type in the intermediate asset
        // folder, attempt to redirect to the original source material type.
        NormalizeMaterialSourceReferences(m_materialSourceData, m_absolutePath);

        // Load the material type source data which provides the layout and default values of all of the properties
        auto materialTypeOutcome = JE::Render::RPI::MaterialUtils::LoadMaterialTypeSourceData(m_materialSourceData.m_materialType);
        if (!materialTypeOutcome.IsSuccess())
        {
            AZ_Error("MaterialDocument", false, "Material type source data could not be loaded: '%s'.", m_materialSourceData.m_materialType.c_str());
            return false;
        }

        m_materialTypeSourceData = materialTypeOutcome.TakeValue();
        return true;
    }

    bool MaterialDocument::LoadMaterialTypeSourceData()
    {
        // A material document can be created or loaded from material or material type source data. If we are attempting to load
        // material type source data then the material source data object can be created just by referencing the document path as the
        // material type path.
        auto materialTypeOutcome = JE::Render::RPI::MaterialUtils::LoadMaterialTypeSourceData(m_absolutePath);
        if (!materialTypeOutcome.IsSuccess())
        {
            AZ_Error("MaterialDocument", false, "Material type source data could not be loaded: '%s'.", m_absolutePath.c_str());
            return false;
        }

        m_materialTypeSourceData = materialTypeOutcome.TakeValue();

        // We are storing absolute paths in the loaded version of the source data so that the files can be resolved at all times.
        m_materialSourceData.m_materialType = m_absolutePath;
        m_materialSourceData.m_parentMaterial.clear();
        return true;
    }

    void MaterialDocument::RestorePropertyValues(const PropertyValueMap& propertyValues)
    {
        for (const auto& propertyValuePair : propertyValues)
        {
            const auto& propertyName = propertyValuePair.first;
            const auto& propertyValue = propertyValuePair.second;
            SetPropertyValue(propertyName.GetStringView(), propertyValue);
        }
    }

    bool MaterialDocument::AddEditorMaterialFunctors(
        const AZStd::vector<JE::Render::RPI::Ptr<JE::Render::RPI::MaterialFunctorSourceDataHolder>>& functorSourceDataHolders,
        const JE::Render::RPI::MaterialNameContext& nameContext)
    {
        const JE::Render::RPI::MaterialFunctorSourceData::EditorContext editorContext = JE::Render::RPI::MaterialFunctorSourceData::EditorContext(
            m_materialSourceData.m_materialType, m_materialAsset->GetMaterialPropertiesLayout(), &nameContext);

        for (JE::Render::RPI::Ptr<JE::Render::RPI::MaterialFunctorSourceDataHolder> functorData : functorSourceDataHolders)
        {
            JE::Render::RPI::MaterialFunctorSourceData::FunctorResult result = functorData->CreateFunctor(editorContext);

            if (result.IsSuccess())
            {
                JE::Render::RPI::Ptr<JE::Render::RPI::MaterialFunctor>& functor = result.GetValue();
                if (functor != nullptr)
                {
                    m_editorFunctors.push_back(functor);
                }
            }
            else
            {
                AZ_Error("MaterialDocument", false, "Material functors were not created: '%s'.", m_absolutePath.c_str());
                return false;
            }
        }

        return true;
    }

    void MaterialDocument::RunEditorMaterialFunctors(JE::Render::RPI::MaterialPropertyFlags dirtyFlags)
    {
        if (!m_materialInstance)
        {
            return;
        }

        AZStd::unordered_map<AZ::Name, JE::Render::RPI::MaterialPropertyDynamicMetadata> propertyDynamicMetadata;
        AZStd::unordered_map<AZ::Name, JE::Render::RPI::MaterialPropertyGroupDynamicMetadata> propertyGroupDynamicMetadata;

        TraverseGroups(m_groups, [&](auto& group) {
            JE::Render::RPI::MaterialPropertyGroupDynamicMetadata& metadata = propertyGroupDynamicMetadata[AZ::Name{ group->m_name }];
            metadata.m_visibility = group->m_visible ? JE::Render::RPI::MaterialPropertyGroupVisibility::Enabled : JE::Render::RPI::MaterialPropertyGroupVisibility::Hidden;

            for (auto& property : group->m_properties)
            {
                JE::Editor::Tools::ConvertToPropertyMetaData(propertyDynamicMetadata[property.GetId()], property.GetConfig());
            }
            return true;
        });

        AZStd::unordered_set<AZ::Name> updatedProperties;
        AZStd::unordered_set<AZ::Name> updatedPropertyGroups;

        for (JE::Render::RPI::Ptr<JE::Render::RPI::MaterialFunctor>& functor : m_editorFunctors)
        {
            const JE::Render::RPI::MaterialPropertyFlags& materialPropertyDependencies = functor->GetMaterialPropertyDependencies();

            // None also covers case that the client code doesn't register material properties to dependencies,
            // which will later get caught in Process() when trying to access a property.
            if (materialPropertyDependencies.none() || functor->NeedsProcess(dirtyFlags))
            {
                JE::Render::RPI::MaterialFunctorAPI::EditorContext context = JE::Render::RPI::MaterialFunctorAPI::EditorContext(
                    m_materialInstance->GetPropertyCollection(), propertyDynamicMetadata,
                    propertyGroupDynamicMetadata, updatedProperties, updatedPropertyGroups,
                    &materialPropertyDependencies);
                functor->Process(context);
            }
        }

        TraverseGroups(m_groups, [&](auto& group) {
            bool groupChange = false;
            bool groupRebuilt = false;
            if (updatedPropertyGroups.find(AZ::Name(group->m_name)) != updatedPropertyGroups.end())
            {
                JE::Render::RPI::MaterialPropertyGroupDynamicMetadata& metadata = propertyGroupDynamicMetadata[AZ::Name{ group->m_name }];
                group->m_visible = metadata.m_visibility != JE::Render::RPI::MaterialPropertyGroupVisibility::Hidden;
                groupChange = true;
            }

            for (auto& property : group->m_properties)
            {
                if (updatedProperties.find(AZ::Name(property.GetId())) != updatedProperties.end())
                {
                    const bool visibleBefore = property.GetConfig().m_visible;
                    JE::Editor::Tools::DynamicPropertyConfig propertyConfig = property.GetConfig();
                    JE::Editor::Tools::ConvertToPropertyConfig(propertyConfig, propertyDynamicMetadata[property.GetId()]);
                    property.SetConfig(propertyConfig);
                    groupChange = true;
                    groupRebuilt |= visibleBefore != property.GetConfig().m_visible;
                }
            }

            if (groupChange || groupRebuilt)
            {
                JE::Editor::Tools::AtomToolsDocumentNotificationBus::Event(
                    m_toolId, &JE::Editor::Tools::AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoChanged, m_id,
                    GetObjectInfoFromDynamicPropertyGroup(group.get()), groupRebuilt);
            }
            return true;
        });
    }

    JE::Editor::Tools::DocumentObjectInfo MaterialDocument::GetObjectInfoFromDynamicPropertyGroup(
        const JE::Editor::Tools::DynamicPropertyGroup* group) const
    {
        JE::Editor::Tools::DocumentObjectInfo objectInfo;
        objectInfo.m_visible = group->m_visible;
        objectInfo.m_name = group->m_name;
        objectInfo.m_displayName = group->m_displayName;
        objectInfo.m_description = group->m_description;
        objectInfo.m_objectType = azrtti_typeid<JE::Editor::Tools::DynamicPropertyGroup>();
        objectInfo.m_objectPtr = const_cast<JE::Editor::Tools::DynamicPropertyGroup*>(group);

        if (group->m_name == "overview")
        {
            // Properties in the overview category don't require special comparison or indicator icons. However, the blank icon is still
            // needed to keep everything aligned.
            objectInfo.m_nodeIndicatorFunction = []([[maybe_unused]] const AzToolsFramework::InstanceDataNode* node)
            {
                return ":/Icons/blank.png";
            };
        }
        else
        {
            objectInfo.m_nodeIndicatorFunction = [](const AzToolsFramework::InstanceDataNode* node)
            {
                const auto property = JE::Editor::Tools::FindAncestorInstanceDataNodeByType<JE::Editor::Tools::DynamicProperty>(node);
                return property && !JE::Editor::Tools::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_parentValue)
                    ? ":/Icons/changed_property.svg"
                    : ":/Icons/blank.png";
            };
        }

        return objectInfo;
    }

    bool MaterialDocument::TraverseGroups(
        AZStd::vector<AZStd::shared_ptr<JE::Editor::Tools::DynamicPropertyGroup>>& groups,
        AZStd::function<bool(AZStd::shared_ptr<JE::Editor::Tools::DynamicPropertyGroup>&)> callback)
    {
        if (!callback)
        {
            return false;
        }

        for (auto& group : groups)
        {
            if (!callback(group) || !TraverseGroups(group->m_groups, callback))
            {
                return false;
            }
        }

        return true;
    }

    bool MaterialDocument::TraverseGroups(
        const AZStd::vector<AZStd::shared_ptr<JE::Editor::Tools::DynamicPropertyGroup>>& groups,
        AZStd::function<bool(const AZStd::shared_ptr<JE::Editor::Tools::DynamicPropertyGroup>&)> callback) const
    {
        if (!callback)
        {
            return false;
        }

        for (auto& group : groups)
        {
            if (!callback(group) || !TraverseGroups(group->m_groups, callback))
            {
                return false;
            }
        }

        return true;
    }

    JE::Editor::Tools::DynamicProperty* MaterialDocument::FindProperty(const AZ::Name& propertyId)
    {
        JE::Editor::Tools::DynamicProperty* result = nullptr;
        TraverseGroups(m_groups, [&](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (property.GetId() == propertyId)
                {
                    result = &property;
                    return false;
                }
            }
            return true;
        });
        return result;
    }

    const JE::Editor::Tools::DynamicProperty* MaterialDocument::FindProperty(const AZ::Name& propertyId) const
    {
        JE::Editor::Tools::DynamicProperty* result = nullptr;
        TraverseGroups(m_groups, [&](auto& group) {
            for (auto& property : group->m_properties)
            {
                if (property.GetId() == propertyId)
                {
                    result = &property;
                    return false;
                }
            }
            return true;
        });
        return result;
    }
} // namespace JE::Editor
