/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Assets/Prefab/PrefabGroup/PrefabGroupBehavior.h>
#include <JE/Assets/Prefab/PrefabGroup/PrefabGroup.h>
#include <JE/Assets/Prefab/PrefabGroup/ProceduralAssetHandler.h>
#include <JE/Assets/Prefab/PrefabGroup/DefaultProceduralPrefab.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EntityUtilityComponent.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderScriptingBus.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemScriptingBus.h>
#include <AzToolsFramework/Prefab/Procedural/ProceduralPrefabAsset.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ICustomPropertyData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>
#include <SceneAPI/SceneCore/Events/ManifestMetaInfoBus.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>

namespace JE::Assets::Prefab
{
    //
    // ExportEventHandler
    //

    static constexpr const char s_PrefabGroupBehaviorCreateDefaultKey[] = "/O3DE/Preferences/Prefabs/CreateDefaults";
    static constexpr const char s_PrefabGroupBehaviorIgnoreActorsKey[] = "/O3DE/Preferences/Prefabs/IgnoreActors";

    struct PrefabGroupBehavior::ExportEventHandler final
        : public AZ::SceneAPI::SceneCore::ExportingComponent
        , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
        , public AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler
    {
        AZ_CLASS_ALLOCATOR(PrefabGroupBehavior, AZ::SystemAllocator)
        using PreExportEventContextFunction = AZStd::function<AZ::SceneAPI::Events::ProcessingResult(AZ::SceneAPI::Events::PreExportEventContext&)>;
        PreExportEventContextFunction m_preExportEventContextFunction;
        JE::Assets::Prefab::PrefabGroupAssetHandler m_prefabGroupAssetHandler;
        AZStd::unique_ptr<DefaultProceduralPrefabGroup> m_defaultProceduralPrefab;

        ExportEventHandler() = delete;

        ExportEventHandler(PreExportEventContextFunction function)
            : m_preExportEventContextFunction(AZStd::move(function))
        {
            m_defaultProceduralPrefab = AZStd::make_unique<DefaultProceduralPrefabGroup>();
            BindToCall(&ExportEventHandler::PrepareForExport);
            AZ::SceneAPI::SceneCore::ExportingComponent::Activate();
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
        }

        ~ExportEventHandler()
        {
            m_defaultProceduralPrefab.reset();
            AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
            AZ::SceneAPI::SceneCore::ExportingComponent::Deactivate();
        }

        AZ::SceneAPI::Events::ProcessingResult PrepareForExport(AZ::SceneAPI::Events::PreExportEventContext& context)
        {
            return m_preExportEventContextFunction(context);
        }

        AZ::SceneAPI::Events::ProcessingResult UpdateSceneForPrefabGroup(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action);

        // AssetImportRequest
        AZ::SceneAPI::Events::ProcessingResult UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action, RequestingApplication requester) override;
        AZ::SceneAPI::Events::ProcessingResult PrepareForAssetLoading(AZ::SceneAPI::Containers::Scene& scene, RequestingApplication requester) override;
        void GetPolicyName(AZStd::string& result) const override
        {
            result = "PrefabGroupBehavior::ExportEventHandler";
        }

        // ManifestMetaInfoBus
        void GetCategoryAssignments(AZ::SceneAPI::Events::ManifestMetaInfo::CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene) override;
        void InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target) override;
    };

    AZ::SceneAPI::Events::ProcessingResult PrefabGroupBehavior::ExportEventHandler::PrepareForAssetLoading(
        [[maybe_unused]] AZ::SceneAPI::Containers::Scene& scene,
        RequestingApplication requester)
    {
        using namespace AzToolsFramework;
        if (requester == RequestingApplication::AssetProcessor)
        {
            EntityUtilityBus::Broadcast(&EntityUtilityBus::Events::ResetEntityContext);
            AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get()->RemoveAllTemplates();
        }
        return AZ::SceneAPI::Events::ProcessingResult::Success;
    }

    void PrefabGroupBehavior::ExportEventHandler::GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene&)
    {
        categories.emplace_back("Procedural Prefab", JE::Assets::Prefab::PrefabGroup::TYPEINFO_Uuid(), s_prefabGroupPreferredTabOrder);
    }

    AZ::SceneAPI::Events::ProcessingResult PrefabGroupBehavior::ExportEventHandler::UpdateSceneForPrefabGroup(
        AZ::SceneAPI::Containers::Scene& scene,
        ManifestAction action)
    {
        if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry)
        {
            bool skip = false;

            // this toggle makes constructing default mesh groups and a prefab optional
            bool createDefaultPrefab = true;
            settingsRegistry->Get(createDefaultPrefab, s_PrefabGroupBehaviorCreateDefaultKey);
            if (createDefaultPrefab == false)
            {
                AZ_Info(
                    "PrefabGroupBehavior",
                    "Skipping default prefab generation - registry setting %s is disabled\n",
                    s_PrefabGroupBehaviorCreateDefaultKey);
                skip = true;
            }

            // do not make a Prefab Group if the animation policy will be applied (if ignore actors is set)
            bool ignoreActors = true;
            settingsRegistry->Get(ignoreActors, s_PrefabGroupBehaviorIgnoreActorsKey);
            if (!skip && ignoreActors)
            {
                AZStd::set<AZStd::string> appliedPolicies;
                AZ::SceneAPI::Events::GraphMetaInfoBus::Broadcast(
                    &AZ::SceneAPI::Events::GraphMetaInfoBus::Events::GetAppliedPolicyNames,
                    appliedPolicies,
                    scene);

                if (appliedPolicies.contains("ActorGroupBehavior"))
                {
                    AZ_Info(
                        "PrefabGroupBehavior",
                        "Skipping default prefab generation - scene has an Actor group present and registry setting %s"
                        " is enabled\n",
                        s_PrefabGroupBehaviorIgnoreActorsKey);
                    skip = true;
                }
            }

            // Remove the prefab group so it doesn't try fail to process an empty prefab group during export
            if (skip)
            {
                for (auto manifestItemIdx = 0; manifestItemIdx < scene.GetManifest().GetEntryCount(); ++manifestItemIdx)
                {
                    const auto* prefabGroup =
                        azrtti_cast<const JE::Assets::Prefab::PrefabGroup*>(scene.GetManifest().GetValue(manifestItemIdx).get());
                    if (prefabGroup)
                    {
                        if (prefabGroup->GetPrefabDomRef().has_value() == false)
                        {
                            scene.GetManifest().RemoveEntry(prefabGroup);
                            return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                        }
                    }
                }

                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }
        }

        if (action == AZ::SceneAPI::Events::AssetImportRequest::Update)
        {
            bool createProceduralPrefab = false;
            for (auto manifestItemIdx = 0; manifestItemIdx < scene.GetManifest().GetEntryCount(); ++manifestItemIdx)
            {
                const auto* prefabGroup = azrtti_cast<const JE::Assets::Prefab::PrefabGroup*>(scene.GetManifest().GetValue(manifestItemIdx).get());
                if (prefabGroup)
                {
                    // found a Prefab Group that wants to be created but does not have a DOM yet?
                    if (prefabGroup->GetPrefabDomRef().has_value() == false)
                    {
                        // re-create the Prefab Group to get the DOM
                        scene.GetManifest().RemoveEntry(prefabGroup);
                        createProceduralPrefab = true;
                        break;
                    }
                }
            }

            if (createProceduralPrefab)
            {
                // clear out the previous created default mesh groups made for this prefab group
                AZStd::vector<const AZ::SceneAPI::DataTypes::IMeshGroup*> proceduralMeshGroupList;
                for (auto manifestItemIdx = 0; manifestItemIdx < scene.GetManifest().GetEntryCount(); ++manifestItemIdx)
                {
                    auto* meshGroup = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshGroup*>(scene.GetManifest().GetValue(manifestItemIdx).get());
                    if (meshGroup)
                    {
                        const auto proceduralMeshGroupRule =
                            meshGroup->GetRuleContainer().FindFirstByType<JE::Assets::Prefab::ProceduralMeshGroupRule>();

                        if (proceduralMeshGroupRule)
                        {
                            proceduralMeshGroupList.push_back(meshGroup);
                        }
                    }
                }
                while (!proceduralMeshGroupList.empty())
                {
                    scene.GetManifest().RemoveEntry(proceduralMeshGroupList.back());
                    proceduralMeshGroupList.pop_back();
                }
            }
            else
            {
                // if a valid prefab group has already been created then do not generate another prefab group
                return AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }
        }

        // ignore empty scenes (i.e. only has the root node)
        if (scene.GetGraph().GetNodeCount() == 1)
        {
            return AZ::SceneAPI::Events::ProcessingResult::Ignored;
        }

        AZStd::optional<JE::Assets::Prefab::PrefabGroupRequests::ManifestUpdates> manifestUpdates;
        JE::Assets::Prefab::PrefabGroupEventBus::BroadcastResult(
            manifestUpdates, &JE::Assets::Prefab::PrefabGroupEventBus::Events::GeneratePrefabGroupManifestUpdates, scene);

        if (!manifestUpdates)
        {
            AZ_Warning("prefab", false, "Scene doesn't contain IMeshData, add at least 1 IMeshData to generate Manifest Updates");
            return AZ::SceneAPI::Events::ProcessingResult::Ignored;
        }

        // update manifest if there were no errors
        for (auto update : manifestUpdates.value())
        {
            scene.GetManifest().AddEntry(update);
        }
        return AZ::SceneAPI::Events::ProcessingResult::Success;
    }

    AZ::SceneAPI::Events::ProcessingResult PrefabGroupBehavior::ExportEventHandler::UpdateManifest(
        AZ::SceneAPI::Containers::Scene& scene,
        ManifestAction action,
        [[maybe_unused]] RequestingApplication requester)
    {
        return UpdateSceneForPrefabGroup(scene, action);
    }

    void PrefabGroupBehavior::ExportEventHandler::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
    {
        if (!target.RTTI_IsTypeOf(PrefabGroup::TYPEINFO_Uuid()))
        {
            return;
        }
        AZStd::vector<AZStd::shared_ptr<AZ::SceneAPI::DataTypes::IManifestObject>> manifestUpdates;
        JE::Assets::Prefab::PrefabGroupEventBus::BroadcastResult(
            manifestUpdates, &JE::Assets::Prefab::PrefabGroupEventBus::Events::GenerateDefaultPrefabMeshGroups, scene);

        AZ::SceneAPI::Events::ManifestMetaInfoBus::Broadcast(&AZ::SceneAPI::Events::ManifestMetaInfoBus::Events::AddObjects, manifestUpdates);

    }

    //
    // PrefabGroupBehavior
    //

    void PrefabGroupBehavior::Activate()
    {
        m_exportEventHandler = AZStd::make_shared<ExportEventHandler>([this](auto& context)
        {
            return this->OnPrepareForExport(context);
        });
    }

    void PrefabGroupBehavior::Deactivate()
    {
        m_exportEventHandler.reset();
    }

    AZStd::unique_ptr<rapidjson::Document> PrefabGroupBehavior::CreateProductAssetData(const JE::Assets::Prefab::PrefabGroup* prefabGroup, const AZ::IO::Path& relativePath) const
    {
        using namespace AzToolsFramework::Prefab;

        auto* prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();
        if (!prefabLoaderInterface)
        {
            AZ_Error("prefab", false, "Could not get PrefabLoaderInterface");
            return {};
        }

        // write to a UTF-8 string buffer
        auto prefabDomRef = prefabGroup->GetPrefabDomRef();
        if (!prefabDomRef)
        {
            AZ_Error("prefab", false, "PrefabGroup(%s) missing PrefabDom", prefabGroup->GetName().c_str());
            return {};
        }

        const AzToolsFramework::Prefab::PrefabDom& prefabDom = prefabDomRef.value();
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
        if (prefabDom.Accept(writer) == false)
        {
            AZ_Error("prefab", false, "Could not write PrefabGroup(%s) to JSON", prefabGroup->GetName().c_str());
            return {};
        }

        // The originPath we pass to LoadTemplateFromString must be the relative path of the file
        AZ::IO::Path templateName(prefabGroup->GetName());
        templateName.ReplaceExtension(JE::Assets::Prefab::PrefabGroupAssetHandler::s_Extension);
        if (!AZ::StringFunc::StartsWith(templateName.c_str(), relativePath.c_str()))
        {
            templateName = relativePath / templateName;
        }

        auto templateId = prefabLoaderInterface->LoadTemplateFromString(sb.GetString(), templateName.Native().c_str());
        if (templateId == InvalidTemplateId)
        {
            AZ_Error("prefab", false, "PrefabGroup(%s) Could not load template", prefabGroup->GetName().c_str());
            return {};
        }

        auto* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        if (!prefabSystemComponentInterface)
        {
            AZ_Error("prefab", false, "Could not get PrefabSystemComponentInterface");
            return {};
        }

        const rapidjson::Document& generatedInstanceDom = prefabSystemComponentInterface->FindTemplateDom(templateId);
        auto proceduralPrefab = AZStd::make_unique<rapidjson::Document>(rapidjson::kObjectType);
        proceduralPrefab->CopyFrom(generatedInstanceDom, proceduralPrefab->GetAllocator(), true);

        return proceduralPrefab;
    }

    bool PrefabGroupBehavior::WriteOutProductAsset(
        AZ::SceneAPI::Events::PreExportEventContext& context,
        const JE::Assets::Prefab::PrefabGroup* prefabGroup,
        const rapidjson::Document& doc) const
    {
        // Since the prefab group name already has the source file extension added as a part of the name (ex: "model_fbx"),
        // we won't pass the source file extension again to CreateOuputFileName. This prevents names like "model_fbx.fbx.procprefab".
        // CreateOutputFileName has been changed to preserve the model's extension as a bugfix, which occurred after the procprefab
        // system was built, so we need to be concerned with backwards compatibility. Procprefab files are typically referenced
        // by file name, not by asset ID or source GUID, so we can't introduce changes that would change the procprefab file name.
        const AZStd::string emptySourceExtension;

        AZStd::string filePath = AZ::SceneAPI::Utilities::FileUtilities::CreateOutputFileName(
            prefabGroup->GetName().c_str(),
            context.GetOutputDirectory(),
            JE::Assets::Prefab::PrefabGroupAssetHandler::s_Extension,
            emptySourceExtension);

        bool result = WriteOutProductAssetFile(filePath, context, prefabGroup, doc, false);

        if (context.GetDebug())
        {
            AZStd::string debugFilePath = AZ::SceneAPI::Utilities::FileUtilities::CreateOutputFileName(
                prefabGroup->GetName().c_str(),
                context.GetOutputDirectory(),
                JE::Assets::Prefab::PrefabGroupAssetHandler::s_Extension,
                emptySourceExtension);
            debugFilePath.append(".json");
            WriteOutProductAssetFile(debugFilePath, context, prefabGroup, doc, true);
        }

        return result;
    }

    bool PrefabGroupBehavior::WriteOutProductAssetFile(
        const AZStd::string& filePath,
        AZ::SceneAPI::Events::PreExportEventContext& context,
        const JE::Assets::Prefab::PrefabGroup* prefabGroup,
        const rapidjson::Document& doc,
        bool debug) const
    {
        AZ::IO::FileIOStream fileStream(filePath.c_str(), AZ::IO::OpenMode::ModeWrite);
        if (fileStream.IsOpen() == false)
        {
            AZ_Error("prefab", false, "File path(%s) could not open for write", filePath.c_str());
            return false;
        }

        // write to a UTF-8 string buffer
        AZ::Data::AssetType assetType = azrtti_typeid<AZ::Prefab::ProceduralPrefabAsset>();
        AZStd::string productPath {prefabGroup->GetName()};
        rapidjson::StringBuffer sb;
        bool writerResult = false;
        if (debug)
        {
            rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
            writerResult = doc.Accept(writer);
            productPath.append(".json");
            assetType = AZ::Data::AssetType::CreateNull();
        }
        else
        {
            rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>> writer(sb);
            writerResult = doc.Accept(writer);
        }
        if (writerResult == false)
        {
            AZ_Error("prefab", false, "PrefabGroup(%s) Could not buffer JSON", prefabGroup->GetName().c_str());
            return false;
        }

        const auto bytesWritten = fileStream.Write(sb.GetSize(), sb.GetString());
        if (bytesWritten > 1)
        {
            AZ::u32 subId = AZ::Crc32(productPath.c_str());
            context.GetProductList().AddProduct(
                filePath,
                context.GetScene().GetSourceGuid(),
                assetType,
                {},
                AZStd::make_optional(subId));

            return true;
        }
        return false;
    }

    AZ::SceneAPI::Events::ProcessingResult PrefabGroupBehavior::OnPrepareForExport(AZ::SceneAPI::Events::PreExportEventContext& context) const
    {
        AZStd::vector<const JE::Assets::Prefab::PrefabGroup*> prefabGroupCollection;
        const AZ::SceneAPI::Containers::SceneManifest& manifest = context.GetScene().GetManifest();

        for (size_t i = 0; i < manifest.GetEntryCount(); ++i)
        {
            const auto* group = azrtti_cast<const JE::Assets::Prefab::PrefabGroup*>(manifest.GetValue(i).get());
            if (group)
            {
                prefabGroupCollection.push_back(group);
            }
        }

        if (prefabGroupCollection.empty())
        {
            return AZ::SceneAPI::Events::ProcessingResult::Ignored;
        }

        // Get the relative path of the source and then take just the path portion of it (no file name)
        AZ::IO::Path relativePath = context.GetScene().GetSourceFilename();
        relativePath = relativePath.LexicallyRelative(AZStd::string_view(context.GetScene().GetWatchFolder()));
        AZStd::string relativeSourcePath { AZStd::move(relativePath.ParentPath().Native()) };
        AZ::StringFunc::Replace(relativeSourcePath, "\\", "/"); // the source paths use forward slashes

        for (const auto* prefabGroup : prefabGroupCollection)
        {
            auto result = CreateProductAssetData(prefabGroup, relativeSourcePath);
            if (!result)
            {
                return AZ::SceneAPI::Events::ProcessingResult::Failure;
            }

            if (WriteOutProductAsset(context, prefabGroup, *result.get()) == false)
            {
                return AZ::SceneAPI::Events::ProcessingResult::Failure;
            }
        }

        return AZ::SceneAPI::Events::ProcessingResult::Success;
    }

    void PrefabGroupBehavior::Reflect(AZ::ReflectContext* context)
    {
        JE::Assets::Prefab::PrefabGroup::Reflect(context);
        AZ::Prefab::ProceduralPrefabAsset::Reflect(context);
        DefaultProceduralPrefabGroup::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PrefabGroupBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(2);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {

            auto loadTemplate = [](const AZStd::string& prefabPath)
            {
                AZ::IO::FixedMaxPath path {prefabPath};
                auto* prefabLoaderInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                if (prefabLoaderInterface)
                {
                    return prefabLoaderInterface->LoadTemplateFromFile(path);
                }
                return AzToolsFramework::Prefab::TemplateId{};
            };

            behaviorContext->Method("LoadTemplate", loadTemplate)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "prefab");

            auto saveTemplateToString = [](AzToolsFramework::Prefab::TemplateId templateId) -> AZStd::string
            {
                AZStd::string output;
                auto* prefabLoaderInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                if (prefabLoaderInterface)
                {
                    prefabLoaderInterface->SaveTemplateToString(templateId, output);
                }
                return output;
            };

            behaviorContext->Method("SaveTemplateToString", saveTemplateToString)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "prefab");
        }
    }
}
