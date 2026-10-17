/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <JE/Render/RPI/Public/Image/StreamingImage.h>

namespace JE::Render
{
    namespace Scene
    {
        class HDRiSkyboxRequests
            : public AZ::ComponentBus
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Scene::HDRiSkyboxRequests, "AZ::Render::HDRiSkyboxRequests", "{86278300-438D-40B3-B87C-B87EE045373E}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            virtual ~HDRiSkyboxRequests() {}

            virtual AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> GetCubemapAsset() const = 0;
            virtual void SetCubemapAsset(const AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& cubemapAsset) = 0;
            virtual void SetCubemapAssetPath(const AZStd::string& path) = 0;
            virtual AZStd::string GetCubemapAssetPath() const = 0;

            virtual void SetCubemapAssetId(AZ::Data::AssetId) = 0;
            virtual AZ::Data::AssetId GetCubemapAssetId() const = 0;

            virtual void SetExposure(float exposure) = 0;
            virtual float GetExposure() const = 0; 
        };

        typedef AZ::EBus<HDRiSkyboxRequests> HDRiSkyboxRequestBus;
    }
}
