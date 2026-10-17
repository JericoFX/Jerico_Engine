/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <JE/Editor/Render/RPI/Edit/Common/ConvertibleSource.h>
#include <JE/Editor/Render/RPI/Edit/Configuration.h>
#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;
    }

namespace JE::Render::RPI
{          
        //! Source data for AssetAliases. It implements the ConvertibleSource interface so it can be converted to 
        //! AssetAliases data when it's used for AnyAsset and be processed by AnyAssetBuilder.
        class JE_EDITOR_RENDER_INTEGRATION_API AssetAliasesSourceData final
            : public ConvertibleSource
        {
        public:
            AZ_TYPE_INFO(AssetAliasesSourceData, "{6EEE3144-33CC-4CE9-9C03-E411571D0712}");
            AZ_CLASS_ALLOCATOR(AssetAliasesSourceData, AZ::SystemAllocator);
            
            static void Reflect(AZ::ReflectContext* context);

            // ConvertibleSource overrides...
            bool Convert(AZ::TypeId& outTypeId, AZStd::shared_ptr<void>& outData) const override;

        private:
            struct AssetAliasInfo
            {
                AZ_TYPE_INFO(AssetAliasInfo, "{192A7D39-BE4D-4C4C-AEC9-D56745EB62D0}");
                AZ_CLASS_ALLOCATOR(AssetAliasInfo, AZ::SystemAllocator);
                AZStd::string m_alias;
                AZStd::string m_path;
            };

            AZStd::vector<AssetAliasInfo> m_assetPaths;
        };

    }

namespace AZ
{ // namespace RPI
} // namespace AZ
