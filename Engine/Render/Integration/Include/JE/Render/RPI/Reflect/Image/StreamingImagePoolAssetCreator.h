/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Reflect/AssetCreator.h>
#include <JE/Render/RPI/Reflect/Configuration.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImagePoolAsset.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Constructs an instance of an StreamingImagePoolAsset.
        //! (Note this class generally follows the builder design pattern, but is called a "creator" rather 
        //! than a "builder" to avoid confusion with the AssetBuilderSDK).
        class JE_RENDER_INTEGRATION_API StreamingImagePoolAssetCreator
            : public AssetCreator<StreamingImagePoolAsset>
        {
        public:
            StreamingImagePoolAssetCreator() = default;

            //! Begins construction of a new streaming image pool asset instance. Resets the builder to a fresh state.
            //! assetId the unique id to use when creating the asset.
            void Begin(const AZ::Data::AssetId& assetId);

            //! Assigns the descriptor used to initialize the RHI streaming image pool.
            void SetPoolDescriptor(AZStd::unique_ptr<AZ::RHI::StreamingImagePoolDescriptor>&& descriptor);

            //! Assigns the name of the pool
            void SetPoolName(AZStd::string_view poolName);
            
            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(AZ::Data::Asset<StreamingImagePoolAsset>& result);
        };
    }
}
