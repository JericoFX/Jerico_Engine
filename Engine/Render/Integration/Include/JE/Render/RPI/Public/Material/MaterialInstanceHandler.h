/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Render/RPI/Public/Configuration.h>
#include <JE/Render/RPI/Public/Material/MaterialInstanceData.h>
#include <JE/Render/RPI/Public/Material/SharedSamplerState.h>
#include <JE/Render/RPI/Reflect/Image/Image.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Instance/Instance.h>
#include <AzCore/Interface/Interface.h>

namespace JE::Render::RPI
{
    class Material;
    class Buffer;

    // Interface to hold and maintain the global SceneMaterialSrg. Each Material registers itself in the Init() function and
    // gets a 'MaterialInstanceData', which contains either the SceneMaterialSrg and the indices to access the right MaterialParameter -
    // buffer, or a unique MaterialSrg for the material. Also manages the TextureSamplers, and registers them in the appropriate
    // Sampler-Array in the Material SRGs.
    class IMaterialInstanceHandler
    {
    public:
        AZ_RTTI_WITH_NAME(JE::Render::RPI::IMaterialInstanceHandler, "AZ::RPI::IMaterialInstanceHandler", "{C683CF51-4859-4E8E-802B-115A2364BCAF}");

        virtual ~IMaterialInstanceHandler(){};

        virtual MaterialInstanceData RegisterMaterialInstance([[maybe_unused]] const AZ::Data::Instance<Material> material) = 0;
        virtual void ReleaseMaterialInstance([[maybe_unused]] const MaterialInstanceData& materialInstance) = 0;
        virtual int32_t RegisterMaterialTexture(
            [[maybe_unused]] const int materialTypeIndex,
            [[maybe_unused]] const int materialInstanceIndex,
            [[maybe_unused]] AZ::Data::Instance<Image> image) = 0;
        virtual void ReleaseMaterialTexture(
            [[maybe_unused]] const int materialTypeIndex,
            [[maybe_unused]] const int materialInstanceIndex,
            [[maybe_unused]] int32_t textureIndex) = 0;
        virtual AZStd::shared_ptr<SharedSamplerState> RegisterTextureSampler(
            [[maybe_unused]] const int materialTypeIndex,
            [[maybe_unused]] const int materialInstanceIndex,
            [[maybe_unused]] const AZ::RHI::SamplerState& samplerState) = 0;
        virtual const AZ::RHI::SamplerState GetRegisteredTextureSampler(
            const int materialTypeIndex, const int materialInstanceIndex, const uint32_t samplerIndex) = 0;

        virtual void Compile() = 0;
    };

    using MaterialInstanceHandlerInterface = AZ::Interface<IMaterialInstanceHandler>;

} // namespace AZ::RPI
