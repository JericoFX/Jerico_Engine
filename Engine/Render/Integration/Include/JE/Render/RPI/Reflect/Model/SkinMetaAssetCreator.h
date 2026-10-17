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
#include <JE/Render/RPI/Reflect/Model/SkinMetaAsset.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Constructs an instance of a SkinMetaAsset
        class JE_RENDER_INTEGRATION_API SkinMetaAssetCreator
            : public AssetCreator<SkinMetaAsset>
        {
        public:
            SkinMetaAssetCreator() = default;
            ~SkinMetaAssetCreator() = default;

            //! Begin construction of a new SkinMetaAsset instance. Resets the builder to a fresh state
            //! @param assetId The unique id to use when creating the asset
            void Begin(const AZ::Data::AssetId& assetId);

            //! Set the mapping between joint names and its indices.
            //! @param jointNameToIndexMap Joint name to index mapping.
            void SetJointNameToIndexMap(const AZStd::unordered_map<AZStd::string, AZ::u16>& jointNameToIndexMap);

            //! Finalize and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(AZ::Data::Asset<SkinMetaAsset>& result);
        };
    } // namespace RPI
} // namespace AZ
