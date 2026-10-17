/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

namespace JE::Render
{
    namespace Scene
    {
        //! ImageBasedLightComponentRequestBus provides an interface to request operations on a ImageBasedLightComponent
        class ImageBasedLightComponentRequests
            : public AZ::ComponentBus
        {
        public:
            virtual void SetSpecularImageAsset(const AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset) = 0;
            virtual void SetDiffuseImageAsset(const AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset) = 0;
            virtual AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> GetSpecularImageAsset() const = 0;
            virtual AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset> GetDiffuseImageAsset() const = 0;
            virtual void SetSpecularImageAssetId(const AZ::Data::AssetId imageAssetId) = 0;
            virtual void SetDiffuseImageAssetId(const AZ::Data::AssetId imageAssetId) = 0;
            virtual AZ::Data::AssetId GetSpecularImageAssetId() const = 0;
            virtual AZ::Data::AssetId GetDiffuseImageAssetId() const = 0;
            virtual void SetSpecularImageAssetPath(const AZStd::string path) = 0;
            virtual void SetDiffuseImageAssetPath(const AZStd::string path) = 0;
            virtual AZStd::string GetSpecularImageAssetPath() const = 0;
            virtual AZStd::string GetDiffuseImageAssetPath() const = 0;
            virtual void SetExposure(float exposure) = 0;
            virtual float GetExposure() const = 0;
        };
        using ImageBasedLightComponentRequestBus = AZ::EBus<ImageBasedLightComponentRequests>;

        //! ImageBasedLightComponent provides an interface for receiving notifications about a ImageBasedLightComponent
        class ImageBasedLightComponentNotifications
            : public AZ::ComponentBus
        {
        public:
            virtual void OnSpecularImageUpdated() {};
            virtual void OnDiffuseImageUpdated() {};
        };
        using ImageBasedLightComponentNotificationBus = AZ::EBus<ImageBasedLightComponentNotifications>;

    } // namespace Render
} // namespace AZ
