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
#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>
#include <JE/Render/RPI/Reflect/Image/AttachmentImageAssetCreator.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

namespace JE::Render
{
    namespace RPI
    {
        //! Use a MaterialAssetCreator to create and configure a new MaterialAsset.
        //!
        //! This requires the MaterialTypeAsset to be fully populated so it can read the property layout.
        class JE_RENDER_INTEGRATION_API MaterialAssetCreator
            : public AssetCreator<MaterialAsset>
        {
        public:
            friend class MaterialSourceData;

            void Begin(const AZ::Data::AssetId& assetId, const AZ::Data::Asset<MaterialTypeAsset>& materialType);
            bool End(AZ::Data::Asset<MaterialAsset>& result);

            void SetMaterialTypeVersion(uint32_t version);
            
            void SetPropertyValue(const AZ::Name& name, const MaterialPropertyValue& value);
            
            void SetPropertyValue(const AZ::Name& name, const AZ::Data::Asset<ImageAsset>& imageAsset);
            void SetPropertyValue(const AZ::Name& name, const AZ::Data::Asset<StreamingImageAsset>& imageAsset);
            void SetPropertyValue(const AZ::Name& name, const AZ::Data::Asset<AttachmentImageAsset>& imageAsset);
        };
    } // namespace RPI
} // namespace AZ
