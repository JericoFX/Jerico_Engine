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
#include <JE/Render/RPI/Reflect/Model/ModelAsset.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Constructs an instance of a ModelAsset
        class JE_RENDER_INTEGRATION_API ModelAssetCreator
            : public AssetCreator<ModelAsset>
        {
        public:
            ModelAssetCreator() = default;
            ~ModelAssetCreator() = default;

            //! Begins construction of a new ModelAsset instance. Resets the builder to a fresh state
            //! @param assetId The unique id to use when creating the asset
            void Begin(const AZ::Data::AssetId& assetId);

            //! Assigns a name to the model
            void SetName(AZStd::string_view name);
            
            //! Adds a new material slot to the asset.
            //! If a slot with the same stable ID already exists, it will be replaced.
            void AddMaterialSlot(const ModelMaterialSlot& materialSlot);

            //! Adds a Lod to the model.
            void AddLodAsset(AZ::Data::Asset<ModelLodAsset>&& lodAsset);
            
            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(AZ::Data::Asset<ModelAsset>& result);
            
            void AddTag(AZ::Name tag);

            //! Clone the given source model asset.
            //! @param sourceAsset The source model asset to clone.
            //! @param clonedResult The resulting, cloned model lod asset.
            //! @param cloneAssetId The asset id to assign to the cloned model asset
            //! @result True in case the asset got cloned successfully, false in case an error happened and the clone process got cancelled.
            static bool Clone(const AZ::Data::Asset<ModelAsset>& sourceAsset, AZ::Data::Asset<ModelAsset>& clonedResult, const AZ::Data::AssetId& cloneAssetId);

        private:
            AZ::Aabb m_modelAabb;
        };
    } // namespace RPI
} // namespace AZ
