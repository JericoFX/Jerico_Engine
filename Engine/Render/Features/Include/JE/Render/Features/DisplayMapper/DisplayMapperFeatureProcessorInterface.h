/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/FeatureProcessor.h>

#include <JE/Render/Features/DisplayMapper/DisplayMapperConfigurationDescriptor.h>

#include <Atom/RHI/ImagePool.h>

#include <JE/Render/RPI/Public/Image/StreamingImage.h>

namespace JE::Render
{
    namespace Features
    {
        struct DisplayMapperLut
        {
            AZ::RHI::Ptr<AZ::RHI::Image> m_lutImage;
            AZ::RHI::Ptr<AZ::RHI::ImageView> m_lutImageView;
            AZ::RHI::ImageViewDescriptor    m_lutImageViewDescriptor = {};
        };

        struct DisplayMapperAssetLut
        {
            AZ::Data::Instance<JE::Render::RPI::StreamingImage>     m_lutStreamingImage;
        };

        //! The display mapper feature processor interface for setting and retreiving tonemapping settings, and handing LUTs.
        class DisplayMapperFeatureProcessorInterface
            : public JE::Render::RPI::FeatureProcessor
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::Features::DisplayMapperFeatureProcessorInterface, "AZ::Render::DisplayMapperFeatureProcessorInterface", "{FA57793A-1C7B-4B44-88C4-02AA431C468F}", JE::Render::RPI::FeatureProcessor);

            virtual void GetOwnedLut(DisplayMapperLut& displayMapperLut, const AZ::Name& lutName) = 0;
            virtual void GetDisplayMapperLut(DisplayMapperLut& displayMapperLut) = 0;
            virtual void GetLutFromAssetLocation(DisplayMapperAssetLut& displayMapperAssetLut, const AZStd::string& assetPath) = 0;
            virtual void GetLutFromAssetId(DisplayMapperAssetLut& displayMapperAssetLut, const AZ::Data::AssetId) = 0;
            virtual void RegisterDisplayMapperConfiguration(const DisplayMapperConfigurationDescriptor& config) = 0;
            virtual void UnregisterDisplayMapperConfiguration() = 0;
            virtual const DisplayMapperConfigurationDescriptor* GetDisplayMapperConfiguration() = 0;
        };
    } // namespace Features
} // namespace JE::Render
