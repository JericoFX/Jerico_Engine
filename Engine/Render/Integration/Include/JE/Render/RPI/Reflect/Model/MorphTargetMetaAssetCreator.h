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
#include <JE/Render/RPI/Reflect/Model/MorphTargetMetaAsset.h>

namespace JE::Render::RPI
{
    //! Constructs an instance of a MorphTargetMetaAsset
    class JE_RENDER_INTEGRATION_API MorphTargetMetaAssetCreator
        : public AssetCreator<MorphTargetMetaAsset>
    {
    public:
        MorphTargetMetaAssetCreator() = default;
        ~MorphTargetMetaAssetCreator() = default;

        //! Begin construction of a new MorphTargetMetaAsset instance. Resets the builder to a fresh state.
        //! @param assetId The unique id to use when creating the asset.
        void Begin(const AZ::Data::AssetId& assetId);

        //! Add meta data for a morph target.
        //! @param morphTarget Meta target for the morph target.
        void AddMorphTarget(const MorphTargetMetaAsset::MorphTarget& morphTarget);

        //! Check if any morph target meta datas got added.
        bool IsEmpty();

        //! Finalize and assigns ownership of the asset to result, if successful. 
        //! Otherwise false is returned and result is left untouched.
        bool End(AZ::Data::Asset<MorphTargetMetaAsset>& result);
    };
} // namespace AZ::RPI
