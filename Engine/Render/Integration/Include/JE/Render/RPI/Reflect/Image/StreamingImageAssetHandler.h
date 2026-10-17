/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Reflect/Asset/AssetHandler.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

namespace JE::Render
{
    namespace RPI
    {
        //! The StreamingImageAsset's handler with customized loading steps in LoadAssetData override.
        class JE_RENDER_INTEGRATION_API StreamingImageAssetHandler final
            : public AssetHandler<StreamingImageAsset>
            , private AZ::Data::AssetBus::MultiHandler
        {
            using Base = AssetHandler<StreamingImageAsset>;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImageAssetHandler, AZ::SystemAllocator)
            virtual ~StreamingImageAssetHandler();
            
            AZ::Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;

            void InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override;

            // Return a default fallback image if an asset is missing
            AZ::Data::AssetId AssetMissingInCatalog(const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/) override;

        private:
            // Data::AssetBus::MultiHandler overrides...
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            // Update pending image asset info when an ImageMipChainAsset was loaded or failed to load
            void HandleMipChainAssetLoad(AZ::Data::Asset<AZ::Data::AssetData> imageMipChainAsset, bool isLoadSuccess);
            // Connect to or disconnect from AssetBus for the ImageMipChainAssets of the input StreamingImageAsset
            void HandleMipChainAssetBuses(AZ::Data::Asset<AZ::Data::AssetData> streamingImageAsset, bool connect);

            struct PendingImageAssetInfo
            {
                AZ::Data::Asset<AZ::Data::AssetData> m_imageAsset;
                AZStd::vector<AZ::u32> m_mipChainAssetSubIds;
            };

            AZStd::mutex m_accessPendingAssetsMutex;
            AZStd::unordered_map<AZ::Uuid, PendingImageAssetInfo> m_pendingReloadImageAsset;
        };
    }
}
