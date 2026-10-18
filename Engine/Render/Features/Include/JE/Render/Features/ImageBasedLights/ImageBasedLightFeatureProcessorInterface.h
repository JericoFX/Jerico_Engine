/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/FeatureProcessor.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>

namespace JE::Render
{
    namespace Features
    {
        //! This feature processor handles image based lights.
        class ImageBasedLightFeatureProcessorInterface
            : public JE::Render::RPI::FeatureProcessor
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::ImageBasedLightFeatureProcessorInterface, "AZ::Render::ImageBasedLightFeatureProcessorInterface", "{EE7441A3-B406-4717-8F35-E3DAC60E3BDB}", JE::Render::RPI::FeatureProcessor);

            //! Sets the global specular IBL image for this scene
            virtual void SetSpecularImage(const AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset) = 0;
            //! Sets the global diffuse IBL image for this scene
            virtual void SetDiffuseImage(const AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset) = 0;
            //! Sets the exposure value in stops for the global IBL. 0 = no change. Final intensity = intensity * 2^exposure.
            virtual void SetExposure(float exposure) = 0;
            //! Sets the orientation of the global IBL.
            virtual void SetOrientation(const AZ::Quaternion& orientation) = 0;
            //! Returns the global specular IBL image for this scene
            virtual const AZ::Data::Instance<JE::Render::RPI::Image>& GetSpecularImage() const = 0;
            //! Returns the global diffuse IBL image for this scene
            virtual const AZ::Data::Instance<JE::Render::RPI::Image>& GetDiffuseImage() const = 0;
            //! Returns the exposure value in stops for the global IBL
            virtual float GetExposure() const = 0;
            //! Returns the orientation of the global IBL.
            virtual const AZ::Quaternion& GetOrientation() const = 0;
            //! Resets the images and exposure back to default (unset) values.
            virtual void Reset() = 0;
        };
    }
}
