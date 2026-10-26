/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneProcessing/Config/SettingsObjects/SoftNameSetting.h>

namespace JE::Assets::RenderBuilders::SceneProcessing::Config
{
    SoftNameSetting::SoftNameSetting(const char* pattern, AZ::SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
        const char* virtualType)
        : m_pattern(pattern, approach)
        , m_virtualType(virtualType)
    {
    }

    SoftNameSetting::~SoftNameSetting() = default;

    AZ::Crc32 SoftNameSetting::GetVirtualTypeHash() const
    {
        if (m_virtualTypeHash == AZ::Crc32())
        {
            m_virtualTypeHash = AZ::Crc32(m_virtualType.c_str());
        }
        return m_virtualTypeHash;
    }

    const AZStd::string& SoftNameSetting::GetVirtualType() const
    {
        return m_virtualType;
    }

    void SoftNameSetting::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SoftNameSetting>()
                ->Version(1)
                ->Field("pattern", &SoftNameSetting::m_pattern)
                ->Field("virtualType", &SoftNameSetting::m_virtualType);

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<SoftNameSetting>(
                        QT_TRANSLATE_NOOP("SceneProcessing", "Soft name setting"),
                        QT_TRANSLATE_NOOP("SceneProcessing", "A pattern matcher to setup project specific naming conventions."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SoftNameSetting::m_pattern,
                        QT_TRANSLATE_NOOP("SceneProcessing", "Pattern"),
                        QT_TRANSLATE_NOOP("SceneProcessing", "The pattern the matcher will check against."))
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SoftNameSetting::m_virtualType,
                        QT_TRANSLATE_NOOP("SceneProcessing", "Virtual Type"),
                        QT_TRANSLATE_NOOP("SceneProcessing", "The node(s) will be converted to this type after their pattern matches."))
                        ->Attribute(AZ::Edit::Attributes::StringList, &SoftNameSetting::GetAllVirtualTypes);
            }
        }
    }

    AZStd::vector<AZStd::string> SoftNameSetting::GetAllVirtualTypes() const
    {
        using namespace AZ::SceneAPI::Events;

        GraphMetaInfo::VirtualTypesSet virtualTypes;
        GraphMetaInfoBus::Broadcast(&GraphMetaInfoBus::Events::GetAllVirtualTypes, virtualTypes);

        AZStd::vector<AZStd::string> result;
        for (AZ::Crc32 virtualType : virtualTypes)
        {
            AZStd::string virtualTypeName;
            GraphMetaInfoBus::Broadcast(&GraphMetaInfoBus::Events::GetVirtualTypeName, virtualTypeName, virtualType);

            AZ_Assert(!virtualTypeName.empty(), "No name found for virtual type with hash %i.", static_cast<AZ::u32>(virtualType));
            result.emplace_back(AZStd::move(virtualTypeName));
        }

        AZStd::sort(result.begin(), result.end());

        return result;
    }
} // namespace JE::Assets::RenderBuilders::SceneProcessing::Config
