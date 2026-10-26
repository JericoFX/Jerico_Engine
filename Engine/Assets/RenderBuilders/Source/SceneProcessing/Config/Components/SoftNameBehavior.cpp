/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <JE/Assets/RenderBuilders/SceneProcessing/Config/SceneProcessingConfigBus.h>
#include <SceneProcessing/Config/Components/SoftNameBehavior.h>
#include <SceneProcessing/Config/SettingsObjects/SoftNameSetting.h>

namespace JE::Assets::RenderBuilders::SceneProcessing::Config
{
    void SoftNameBehavior::Activate()
    {
        AZ::SceneAPI::Events::GraphMetaInfoBus::Handler::BusConnect();
    }

    void SoftNameBehavior::Deactivate()
    {
        AZ::SceneAPI::Events::GraphMetaInfoBus::Handler::BusDisconnect();
    }

    void SoftNameBehavior::GetVirtualTypes(
        AZ::SceneAPI::Events::GraphMetaInfo::VirtualTypesSet& types,
        const AZ::SceneAPI::Containers::Scene& scene,
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex node)
    {
        const AZStd::vector<AZStd::unique_ptr<SoftNameSetting>>* softNames = nullptr;
        SceneProcessingConfigRequestBus::BroadcastResult(softNames, &SceneProcessingConfigRequestBus::Events::GetSoftNames);

        if (softNames)
        {
            for (const AZStd::unique_ptr<SoftNameSetting>& softName : *softNames)
            {
                if (types.find(softName->GetVirtualTypeHash()) != types.end())
                {
                    // This type has already been added.
                    continue;
                }

                if (softName->IsVirtualType(scene, node))
                {
                    types.insert(softName->GetVirtualTypeHash());
                }
            }
        }
    }

    void SoftNameBehavior::GetVirtualTypeName(AZStd::string& name, AZ::Crc32 type)
    {
        if (type == AZ_CRC_CE("Ignore"))
        {
            name = "Ignore";
        }
    }

    void SoftNameBehavior::GetAllVirtualTypes(AZ::SceneAPI::Events::GraphMetaInfo::VirtualTypesSet& types)
    {
        // Add types that aren't handled by one specific behavior and have a more global utility.
        if (types.find(AZ_CRC_CE("Ignore")) == types.end())
        {
            types.insert(AZ_CRC_CE("Ignore"));
        }
    }

    void SoftNameBehavior::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SoftNameBehavior, BehaviorComponent>()->Version(1);
        }
    }
} // namespace JE::Assets::RenderBuilders::SceneProcessing::Config
