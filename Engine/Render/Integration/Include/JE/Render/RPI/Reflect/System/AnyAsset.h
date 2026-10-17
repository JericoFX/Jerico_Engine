/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <JE/Render/RPI/Reflect/Base.h>

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        //! Any asset can be used for storage any az serialization class data. So user don't need to create their own 
        //! builder and handler.
        class JE_RENDER_INTEGRATION_API AnyAsset
            : public AZ::Data::AssetData
        {
            friend class AnyAssetHandler;
            friend class AnyAssetBuilder;
            friend class AnyAssetCreator;

        public:
            AZ_RTTI(AnyAsset, "{2643D686-3E7C-450C-BB61-427EDEBF13D5}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(AnyAsset, AZ::SystemAllocator);

            static constexpr const char* DisplayName{ "AnyAsset" };
            static constexpr const char* Group{ "Common" };
            static constexpr const char* Extension{ "azasset" };
            
            template <typename T>
            const T* GetDataAs() const
            {
                return AZStd::any_cast<T>(&m_data);
            }

            const AZStd::any& GetAny() const
            {
                return m_data;
            }

        private:
            // Called by asset creators to assign the asset to a ready state.
            void SetReady();

            AZStd::any m_data;
        };

        template<typename T>
        const T* GetDataFromAnyAsset(AZ::Data::Asset<AnyAsset> anyAsset)
        {
            // If the asset wasn't loaded, load it first
            if (!anyAsset.IsReady())
            {
                anyAsset = AZ::Data::AssetManager::Instance().GetAsset<AnyAsset>(anyAsset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
                anyAsset.BlockUntilLoadComplete();
            }

            if (!anyAsset.IsReady())
            {
                AZ_Error("AnyAsset", false, "Failed to load asset [%s]", anyAsset.GetHint().c_str());
                return nullptr;
            }

            const T* dataT = anyAsset->GetDataAs<T>();
            AZ_Error("AnyAsset", dataT, "Asset [%s] doesn't have expected data", anyAsset.GetHint().c_str());
            return dataT;
        }

        class JE_RENDER_INTEGRATION_API AnyAssetHandler
            : public AssetHandler<AnyAsset>
        {
            using Base = AssetHandler<AnyAsset>;
        public:
            AZ::Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;
        };

        class JE_RENDER_INTEGRATION_API AnyAssetCreator
        {
        public:
            static bool CreateAnyAsset(const AZStd::any& anyData, const AZ::Data::AssetId& assetId, AZ::Data::Asset<AnyAsset>& result);
            static void SetAnyAssetData(const AZStd::any& anyData, AnyAsset& result);
        };

    }

namespace AZ
{ // namespace RPI
} // namespace AZ
