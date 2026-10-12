/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Assets/Prefab/PrefabGroup/PrefabGroup.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/optional.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>

namespace JE::Assets::Prefab
{
     // PrefabGroup

     PrefabGroup::PrefabGroup()
        : m_id(AZ::Uuid::CreateNull())
        , m_name()
    {
    }

    const AZStd::string& PrefabGroup::GetName() const
    {
        return m_name;
    }

    void PrefabGroup::SetName(AZStd::string name)
    {
        m_name = AZStd::move(name);
    }

    const AZ::Uuid& PrefabGroup::GetId() const
    {
        return m_id;
    }

    void PrefabGroup::SetId(AZ::Uuid id)
    {
        m_id = AZStd::move(id);
    }

    AZ::SceneAPI::Containers::RuleContainer& PrefabGroup::GetRuleContainer()
    {
        return m_rules;
    }

    const AZ::SceneAPI::Containers::RuleContainer& PrefabGroup::GetRuleContainerConst() const
    {
        return m_rules;
    }

    AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& PrefabGroup::GetSceneNodeSelectionList()
    {
        return m_nodeSelectionList;
    }

    const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& PrefabGroup::GetSceneNodeSelectionList() const
    {
        return m_nodeSelectionList;
    }

    void PrefabGroup::GetManifestObjectsToRemoveOnRemoved(
        AZStd::vector<const AZ::SceneAPI::DataTypes::IManifestObject*>& toRemove, const AZ::SceneAPI::Containers::SceneManifest& manifest) const
    {
        for (size_t manifestIndex = 0; manifestIndex < manifest.GetEntryCount(); ++manifestIndex)
        {
            const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IManifestObject> manifestObjectAtIndex = manifest.GetValue(manifestIndex);
            if (manifestObjectAtIndex->RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IGroup::TYPEINFO_Uuid()))
            {
                // Removing shared ownership is useful because this is about to be deleted.
                const AZ::SceneAPI::DataTypes::IGroup* sceneNodeGroup = azrtti_cast<const AZ::SceneAPI::DataTypes::IGroup*>(manifestObjectAtIndex.get());
                const AZ::SceneAPI::Containers::RuleContainer& rules = sceneNodeGroup->GetRuleContainerConst();
                // Anything with the procedural mesh group rule was added automatically for this prefab group, so mark it for removal.
                if (rules.FindFirstByType<JE::Assets::Prefab::ProceduralMeshGroupRule>())
                {
                    // Add it to the list to remove.
                    toRemove.push_back(sceneNodeGroup);
                }
            }
        }
    }

    void PrefabGroup::SetPrefabDom(AzToolsFramework::Prefab::PrefabDom prefabDom)
    {
        m_prefabDomData = AZStd::make_shared<AZ::Prefab::PrefabDomData>();
        m_prefabDomData->CopyValue(prefabDom);
    }

    AzToolsFramework::Prefab::PrefabDomConstReference PrefabGroup::GetPrefabDomRef() const
    {
        if (m_prefabDomData)
        {
            return m_prefabDomData->GetValue();
        }
        return {};
    }

    void PrefabGroup::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<JE::Assets::Prefab::IPrefabGroup, AZ::SceneAPI::DataTypes::ISceneNodeGroup>()
                ->Version(1);

            serializeContext->Class<ProceduralMeshGroupRule, AZ::SceneAPI::DataTypes::IRule>()
                ->Version(1);

            serializeContext->Class<PrefabGroup, JE::Assets::Prefab::IPrefabGroup>()
                ->Version(3) // added createProceduralPrefab
                ->Field("name", &PrefabGroup::m_name)
                ->Field("nodeSelectionList", &PrefabGroup::m_nodeSelectionList)
                ->Field("rules", &PrefabGroup::m_rules)
                ->Field("id", &PrefabGroup::m_id)
                ->Field("prefabDomData", &PrefabGroup::m_prefabDomData);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            const char* prefabTooltip = "The prefab group controls the generation of default procedural prefabs. "
                "This includes the generation of necessary mesh groups to construct the prefab. "
                "Removing this group will disable the default procedural prefab and remove the mesh groups used by that prefab. "
                "This group does not control the generation of non-default procedural prefabs, those must be disabled in the script that generates them.";

            if (editContext)
            {
                editContext->Class<PrefabGroup>("Prefab group", prefabTooltip)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ::Edit::Attributes::Max, 1)
                        ->Attribute(AZ::Edit::Attributes::CategoryStyle, "display divider")
                        // There isn't a documentation page for default prefabs under the scene settings documentation category, yet.
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/assets/scene-settings/")
                    ->UIElement(AZ::Edit::UIHandlers::MultiLineEdit, "", prefabTooltip)
                        ->Attribute(AZ::Edit::Attributes::ValueText, "The prefab group controls the generation of default procedural prefabs.")
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, true);
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            auto setPrefabDomData = [](PrefabGroup& self, const AZStd::string& json)
            {
                auto jsonOutcome = AZ::JsonSerializationUtils::ReadJsonString(json);
                if (jsonOutcome.IsSuccess())
                {
                    self.SetPrefabDom(AZStd::move(jsonOutcome.GetValue()));
                    return true;
                }
                AZ_Error("prefab", false, "Set PrefabDom failed (%s)", jsonOutcome.GetError().c_str());
                return false;
            };

            auto getPrefabDomData = [](const PrefabGroup& self) -> AZStd::string
            {
                if (self.GetPrefabDomRef().has_value() == false)
                {
                    return {};
                }
                AZStd::string buffer;
                AZ::JsonSerializationUtils::WriteJsonString(self.GetPrefabDomRef().value(), buffer);
                return buffer;
            };

            behaviorContext->Class<PrefabGroup>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "prefab")
                ->Property("name", BehaviorValueProperty(&PrefabGroup::m_name))
                ->Property("id", BehaviorValueProperty(&PrefabGroup::m_id))
                ->Property("prefabDomData", getPrefabDomData, setPrefabDomData);
        }
    }

    bool ProceduralMeshGroupRule::ModifyTooltip(AZStd::string& tooltip)
    {
        tooltip = AZStd::string::format("This group was generated by the procedural prefab. To remove this group, remove the procedural prefab. %.*s", AZ_STRING_ARG(tooltip));
        return true;
    }
}
