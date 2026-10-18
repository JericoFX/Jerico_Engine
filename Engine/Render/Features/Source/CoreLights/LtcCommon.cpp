/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Public/Image/StreamingImage.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <CoreLights/LtcCommon.h>

namespace JE::Render::Features
{
    static constexpr const char* s_LtcGgxMatrixPath = "textures/ltc/ltc_mat_lutrgba32f.dds.streamingimage";
    static constexpr const char* s_LtcGgxAmplificationPath = "textures/ltc/ltc_amp_lutrg32f.dds.streamingimage";

    LtcCommon::LtcCommon()
    {
        AZ::Interface<ILtcCommon>::Register(this);
    }

    LtcCommon::~LtcCommon()
    {
        m_assetLoaders.clear();
        AZ::Interface<ILtcCommon>::Unregister(this);
    }

    void LtcCommon::LoadMatricesForSrg(AZ::Data::Instance<JE::Render::RPI::ShaderResourceGroup> srg)
    {
        if (!srg)
        {
            return;
        }

        AZ_Error("LtcCommon", srg->GetId().IsValid(), "Srg used to load matrices must have a valid Id.");

        auto callback = [srg](AZ::Data::Asset<AZ::Data::AssetData> asset, const char* srgName)
        {
            if (asset.IsReady())
            {
                if (const AZ::RHI::ShaderInputImageIndex index = srg->FindShaderInputImageIndex(AZ::Name(srgName)); index.IsValid())
                {
                    auto streamingImageInstance = JE::Render::RPI::StreamingImage::FindOrCreate(asset);
                    srg->SetImage(index, streamingImageInstance);
                }
            }
        };

        // De-duplicate load requests by the srg's guid to avoid holding a refernce to the srg itself.
        const AZ::Uuid& srgGuid = srg->GetId().GetGuid();
        if (m_assetLoaders.find(srgGuid) == m_assetLoaders.end())
        {
            auto& assetLoaders = m_assetLoaders[srgGuid];
            assetLoaders.reserve(2);
            assetLoaders.emplace_back(JE::Render::RPI::AssetUtils::AsyncAssetLoader::Create<JE::Render::RPI::StreamingImageAsset>(
                s_LtcGgxAmplificationPath, 0, AZStd::bind(callback, AZStd::placeholders::_1, "m_ltcAmplification")));
            assetLoaders.emplace_back(JE::Render::RPI::AssetUtils::AsyncAssetLoader::Create<JE::Render::RPI::StreamingImageAsset>(
                s_LtcGgxMatrixPath, 0, AZStd::bind(callback, AZStd::placeholders::_1, "m_ltcMatrix")));
        }
    }
} // namespace JE::Render::Features
