/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/System/AssetAliases.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace JE::Render
{
    namespace RPI
    {        
        void AssetAliases::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AssetAliases>()
                    ->Version(0)
                    ->Field("AssetMapping", &AssetAliases::m_assetMapping)
                    ;
            }
        }

        AZ::Data::AssetId AssetAliases::FindAssetId(const AZ::Name& alias) const
        {
            auto foundAsset = m_assetMapping.find(alias.GetCStr());
            if (foundAsset != m_assetMapping.end())
            {
                return foundAsset->second;
            }
            else
            {
                return AZ::Data::AssetId();
            }
        }

        const AZStd::unordered_map<AZStd::string, AZ::Data::AssetId>& AssetAliases::GetAssetMapping() const
        {
            return m_assetMapping;
        }

    } // namespace RPI
} // namespace AZ
