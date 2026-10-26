/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneProcessing/Config/SettingsObjects/FileSoftNameSetting.h>

namespace JE::Assets::RenderBuilders::SceneProcessing::Config
{
    FileSoftNameSetting::GraphType::GraphType()
        : m_cachedId(AZ::Uuid::CreateNull())
    {
    }

    FileSoftNameSetting::GraphType::GraphType(const AZStd::string& name)
        : m_name(name)
        , m_cachedId(AZ::Uuid::CreateNull())
    {
    }

    FileSoftNameSetting::GraphType::GraphType(AZStd::string&& name)
        : m_name(AZStd::move(name))
        , m_cachedId(AZ::Uuid::CreateNull())
    {
    }

    const AZStd::string& FileSoftNameSetting::GraphType::GetName() const
    {
        return m_name;
    }

    const AZ::Uuid& FileSoftNameSetting::GraphType::GetId() const
    {
        if (m_cachedId.IsNull())
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "Unable to find valid serialize context.");

            context->EnumerateDerived<AZ::SceneAPI::DataTypes::IGraphObject>(
                [this](const AZ::SerializeContext::ClassData* data, const AZ::Uuid& typeId) -> bool
                {
                    AZ_UNUSED(typeId);
                    if (AzFramework::StringFunc::Equal(data->m_name, m_name.c_str()))
                    {
                        m_cachedId = data->m_typeId;
                        return false;
                    }
                    return true;
                });

            if (m_cachedId.IsNull())
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow, "Unable to find '%s' in the serialize context.", m_name.c_str());
            }
        }
        return m_cachedId;
    }

    void FileSoftNameSetting::GraphType::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GraphType>()
                ->Version(1)
                ->Field("name", &GraphType::m_name);
        }
    }

    FileSoftNameSetting::GraphTypeContainer::GraphTypeContainer(std::initializer_list<GraphType> graphTypes)
        : m_types(graphTypes)
    {
    }

    AZStd::vector<FileSoftNameSetting::GraphType>& FileSoftNameSetting::GraphTypeContainer::GetGraphTypes()
    {
        return m_types;
    }

    const AZStd::vector<FileSoftNameSetting::GraphType>& FileSoftNameSetting::GraphTypeContainer::GetGraphTypes() const
    {
        return m_types;
    }

    void FileSoftNameSetting::GraphTypeContainer::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GraphTypeContainer>()
                ->Version(1)
                ->Field("types", &GraphTypeContainer::m_types);
        }
    }


    FileSoftNameSetting::FileSoftNameSetting(const char* pattern, AZ::SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
        const char* virtualType, bool inclusive, std::initializer_list<GraphType> graphTypes)
        : SoftNameSetting(pattern, approach, virtualType)
        , m_inclusiveList(inclusive)
        , m_graphTypes(graphTypes)
        , m_cachedScene(nullptr)
    {
    }

    bool FileSoftNameSetting::IsVirtualType(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::Containers::SceneGraph::NodeIndex node) const
    {
        bool nameMatch = false;
        if (m_cachedScene == &scene)
        {
            nameMatch = m_cachedNameMatch;
        }
        else
        {
            switch (m_pattern.GetMatchApproach())
            {
            case AZ::SceneAPI::SceneCore::PatternMatcher::MatchApproach::PreFix:
                nameMatch = m_pattern.MatchesPattern(scene.GetName());
                break;
            case AZ::SceneAPI::SceneCore::PatternMatcher::MatchApproach::PostFix:
                nameMatch = m_pattern.MatchesPattern(scene.GetName());
                break;
            case AZ::SceneAPI::SceneCore::PatternMatcher::MatchApproach::Regex:
                nameMatch = m_pattern.MatchesPattern(scene.GetSourceFilename());
                break;
            default:
                AZ_Assert(false, "Unknown option '%i' for pattern matcher.", m_pattern.GetMatchApproach());
                nameMatch = false;
            }

            m_cachedNameMatch = nameMatch;
            m_cachedScene = &scene;
        }

        if (nameMatch)
        {
            AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IGraphObject> object = scene.GetGraph().GetNodeContent(node);
            for (const GraphType& type : m_graphTypes.GetGraphTypes())
            {
                if (object->RTTI_IsTypeOf(type.GetId()))
                {
                    return m_inclusiveList;
                }
            }
            return !m_inclusiveList;
        }
        else
        {
            return false;
        }
    }

    const AZ::Uuid FileSoftNameSetting::GetTypeId() const
    {
        return azrtti_typeid<FileSoftNameSetting>();
    }

    void FileSoftNameSetting::Reflect(AZ::ReflectContext* context)
    {
        GraphType::Reflect(context);
        GraphTypeContainer::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<FileSoftNameSetting, SoftNameSetting>()
                ->Version(1)
                ->Field("graphTypes", &FileSoftNameSetting::m_graphTypes)
                ->Field("inclusiveList", &FileSoftNameSetting::m_inclusiveList);

            serialize->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<FileSoftNameSetting>>>();

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<FileSoftNameSetting>(
                        QT_TRANSLATE_NOOP("SceneProcessing", "File name setting"),
                        QT_TRANSLATE_NOOP("SceneProcessing", "Applies the pattern to the name of the scene file."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ_CRC_CE("GraphTypeSelector"), &FileSoftNameSetting::m_graphTypes,
                        QT_TRANSLATE_NOOP("SceneProcessing", "Graph type"),
                        QT_TRANSLATE_NOOP("SceneProcessing", "The graph types that are the soft name applies to."))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &FileSoftNameSetting::m_inclusiveList,
                        QT_TRANSLATE_NOOP("SceneProcessing", "Inclusive"),
                        QT_TRANSLATE_NOOP("SceneProcessing", "If true the types in the list will marked as the virtual type, otherwise any types that are NOT in the list."));
            }
        }
    }
} // namespace JE::Assets::RenderBuilders::SceneProcessing::Config
