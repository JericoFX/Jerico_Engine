/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <JE/Render/RPI/Public/RPIUtils.h>
#include <JE/Render/RPI/Public/Shader/ShaderResourceGroup.h>
#include <JE/Render/RPI/Public/Scene.h>
#include <JE/Render/RPI/Public/View.h>

#include <Debug/RenderDebugSettings.h>
#include <Debug/RenderDebugFeatureProcessor.h>

namespace JE::Render::Features
{
    RenderDebugSettings::RenderDebugSettings([[maybe_unused]] RenderDebugFeatureProcessor* featureProcessor)
    {
    }

    void RenderDebugSettings::Simulate()
    {
        UpdateOptionsMask();
    }

    void RenderDebugSettings::UpdateOptionsMask()
    {
        m_optionsMask = 0;

        // Enabled
        m_optionsMask |= (AZ::u32)GetEnabled() << (AZ::u32)RenderDebugOptions::DebugEnabled;

        // Material Overrides
        m_optionsMask |= (AZ::u32)GetOverrideBaseColor() << (AZ::u32)RenderDebugOptions::OverrideBaseColor;
        m_optionsMask |= (AZ::u32)GetOverrideRoughness() << (AZ::u32)RenderDebugOptions::OverrideRoughness;
        m_optionsMask |= (AZ::u32)GetOverrideMetallic() << (AZ::u32)RenderDebugOptions::OverrideMetallic;

        // Normal Maps
        m_optionsMask |= (AZ::u32)GetEnableNormalMaps() << (AZ::u32)RenderDebugOptions::EnableNormalMaps;
        m_optionsMask |= (AZ::u32)GetEnableDetailNormalMaps() << (AZ::u32)RenderDebugOptions::EnableDetailNormalMaps;

        // Debug Light
        bool useDebugLight = GetRenderDebugLightingSource() == RenderDebugLightingSource::DebugLight;
        m_optionsMask |= (AZ::u32)useDebugLight << (AZ::u32)RenderDebugOptions::UseDebugLight;

        // Direct & Indirect Lighting

        bool diffuseLightingEnabled = GetRenderDebugLightingType() == RenderDebugLightingType::Diffuse ||
                                      GetRenderDebugLightingType() == RenderDebugLightingType::DiffuseAndSpecular;

        bool specularLightingEnabled = GetRenderDebugLightingType() == RenderDebugLightingType::Specular ||
                                       GetRenderDebugLightingType() == RenderDebugLightingType::DiffuseAndSpecular;

        bool directLightingEnabled = GetRenderDebugLightingSource() == RenderDebugLightingSource::Direct ||
                                     GetRenderDebugLightingSource() == RenderDebugLightingSource::DirectAndIndirect;

        bool indirectLightingEnabled = GetRenderDebugLightingSource() == RenderDebugLightingSource::Indirect ||
                                       GetRenderDebugLightingSource() == RenderDebugLightingSource::DirectAndIndirect;

        if (useDebugLight)
        {
            directLightingEnabled = indirectLightingEnabled = false;
        }

        if (GetRenderDebugViewMode() != RenderDebugViewMode::None)
        {
            diffuseLightingEnabled = specularLightingEnabled = directLightingEnabled = indirectLightingEnabled = false;
        }

        m_optionsMask |= (AZ::u32)diffuseLightingEnabled  << (AZ::u32)RenderDebugOptions::EnableDiffuseLighting;
        m_optionsMask |= (AZ::u32)specularLightingEnabled << (AZ::u32)RenderDebugOptions::EnableSpecularLighting;
        m_optionsMask |= (AZ::u32)directLightingEnabled   << (AZ::u32)RenderDebugOptions::EnableDirectLighting;
        m_optionsMask |= (AZ::u32)indirectLightingEnabled << (AZ::u32)RenderDebugOptions::EnableIndirectLighting;

        // Custom Debug Options
        m_optionsMask |= (AZ::u32)GetCustomDebugOption01() << (AZ::u32)RenderDebugOptions::CustomDebugOption01;
        m_optionsMask |= (AZ::u32)GetCustomDebugOption02() << (AZ::u32)RenderDebugOptions::CustomDebugOption02;
        m_optionsMask |= (AZ::u32)GetCustomDebugOption03() << (AZ::u32)RenderDebugOptions::CustomDebugOption03;
        m_optionsMask |= (AZ::u32)GetCustomDebugOption04() << (AZ::u32)RenderDebugOptions::CustomDebugOption04;
    }

}
