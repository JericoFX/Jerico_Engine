/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/FilmGrain/FilmGrainSettings.h>
#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainConstants.h>

#include <PostProcess/PostProcessFeatureProcessor.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <JE/Render/RPI/Public/Image/ImageSystemInterface.h>
#include <JE/Render/RPI/Public/Pass/ParentPass.h>
#include <JE/Render/RPI/Public/Pass/PassSystemInterface.h>
#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/View.h>

namespace AZ
{
    namespace Render
    {
        FilmGrainSettings::FilmGrainSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        void FilmGrainSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        AZ::Data::Instance<JE::Render::RPI::StreamingImage> FilmGrainSettings::LoadStreamingImage(
            const char* textureFilePath, [[maybe_unused]] const char* sampleName)
        {
            using namespace AZ;

            Data::AssetId streamingImageAssetId;
            Data::AssetCatalogRequestBus::BroadcastResult(
                streamingImageAssetId, &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, textureFilePath,
                azrtti_typeid<JE::Render::RPI::StreamingImageAsset>(), false);
            if (!streamingImageAssetId.IsValid())
            {
                AZ_Error(sampleName, false, "Failed to get streaming image asset id with path %s", textureFilePath);
                return nullptr;
            }

            auto streamingImageAsset = Data::AssetManager::Instance().GetAsset<JE::Render::RPI::StreamingImageAsset>(
                streamingImageAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
            streamingImageAsset.BlockUntilLoadComplete();

            if (!streamingImageAsset.IsReady())
            {
                AZ_Error(sampleName, false, "Failed to get streaming image asset '%s'", textureFilePath);
                return nullptr;
            }

            auto image = JE::Render::RPI::StreamingImage::FindOrCreate(streamingImageAsset);
            if (!image)
            {
                AZ_Error(sampleName, false, "Failed to find or create an image instance from image asset '%s'", textureFilePath);
                return nullptr;
            }

            return image;
        }

        void FilmGrainSettings::ApplySettingsTo(FilmGrainSettings* target, float alpha) const
        {
            AZ_Assert(target != nullptr, "FilmGrainSettings::ApplySettingsTo called with nullptr as argument.");

            // Auto-gen code to blend individual params based on their override value onto target settings
#define OVERRIDE_TARGET target
#define OVERRIDE_ALPHA alpha
#include <Atom/Feature/ParamMacros/StartOverrideBlend.inl>
#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef OVERRIDE_TARGET
#undef OVERRIDE_ALPHA
        }

        void FilmGrainSettings::Simulate(float deltaTime)
        {
            m_deltaTime = deltaTime;
        }

    } // namespace Render
} // namespace AZ
