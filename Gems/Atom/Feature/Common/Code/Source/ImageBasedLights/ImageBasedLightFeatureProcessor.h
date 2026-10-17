/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <JE/Render/RPI/Public/FeatureProcessor.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! This feature processor handles image based lights.
        class ImageBasedLightFeatureProcessor final
            : public ImageBasedLightFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(ImageBasedLightFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::ImageBasedLightFeatureProcessor, "{1206C38B-2143-4EE1-9C83-F876BD465BBB}", AZ::Render::ImageBasedLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            ImageBasedLightFeatureProcessor() = default;
            virtual ~ImageBasedLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            //! Creates pools, buffers, and buffer views
            void Activate() override;
            //! Releases GPU resources.
            void Deactivate() override;
            //! Updates the images for any ibls that changed.
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

            void SetSpecularImage(const Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset) override;
            const Data::Instance<JE::Render::RPI::Image>& GetSpecularImage() const override { return m_specular; }

            void SetDiffuseImage(const Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset) override;
            const Data::Instance<JE::Render::RPI::Image>& GetDiffuseImage() const override { return m_diffuse; }

            void SetExposure(float exposure) override;
            float GetExposure() const override { return m_exposure; }

            void SetOrientation(const Quaternion& orientation) override;
            const Quaternion& GetOrientation() const override { return m_orientation; }

            void Reset() override;

        private:

            ImageBasedLightFeatureProcessor(const ImageBasedLightFeatureProcessor&) = delete;

            void LoadDefaultCubeMaps();
            static Data::Instance<JE::Render::RPI::Image> GetInstanceForImage(const Data::Asset<JE::Render::RPI::StreamingImageAsset>& imageAsset, const Data::Instance<JE::Render::RPI::Image>& defaultImage);
            static bool ValidateIsCubemap(Data::Instance<JE::Render::RPI::Image> image);

            Data::Instance<JE::Render::RPI::ShaderResourceGroup> m_sceneSrg;
            RHI::ShaderInputNameIndex m_specularEnvMapIndex = "m_specularEnvMap";
            RHI::ShaderInputNameIndex m_diffuseEnvMapIndex = "m_diffuseEnvMap";
            RHI::ShaderInputNameIndex m_iblExposureConstantIndex = "m_iblExposure";
            RHI::ShaderInputNameIndex m_iblOrientationConstantIndex = "m_iblOrientation";

            Data::Instance<JE::Render::RPI::Image> m_specular;
            Data::Instance<JE::Render::RPI::Image> m_diffuse;
            Quaternion m_orientation = Quaternion::CreateIdentity();
            float m_exposure = 0;

            Data::Instance<JE::Render::RPI::Image> m_defaultSpecularImage;
            Data::Instance<JE::Render::RPI::Image> m_defaultDiffuseImage;
        };
    }
}
