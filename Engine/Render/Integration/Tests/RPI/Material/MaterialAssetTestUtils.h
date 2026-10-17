/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Common/TestUtils.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>
#include <JE/Render/RPI/Reflect/Material/MaterialTypeAssetCreator.h>
#include <AzCore/Name/Name.h>

namespace UnitTest
{
    template<typename T>
    void CheckPropertyValue(const AZ::Data::Asset<JE::Render::RPI::MaterialTypeAsset>& asset, const AZ::Name& propertyName, T expectedValue)
    {
        const JE::Render::RPI::MaterialPropertyIndex propertyIndex = asset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyName);

        EXPECT_TRUE(propertyIndex.IsValid());

        JE::Render::RPI::MaterialPropertyValue value = asset->GetDefaultPropertyValues()[propertyIndex.GetIndex()];

        EXPECT_TRUE(value == expectedValue);
    }

    template<typename T>
    void CheckMaterialPropertyValue(const AZ::Data::Asset<JE::Render::RPI::MaterialAsset>& asset, const AZ::Name& propertyName, T expectedValue)
    {
        const JE::Render::RPI::MaterialPropertyIndex propertyIndex = asset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyName);

        EXPECT_TRUE(propertyIndex.IsValid());

        JE::Render::RPI::MaterialPropertyValue value = asset->GetPropertyValues()[propertyIndex.GetIndex()];

        EXPECT_TRUE(value == expectedValue);
    }

    void AddMaterialPropertyForSrg(
        JE::Render::RPI::MaterialTypeAssetCreator& materialTypeCreator,
        const AZ::Name& propertyName,
        JE::Render::RPI::MaterialPropertyDataType dataType,
        const AZ::Name& shaderInputName);

    void AddCommonTestMaterialProperties(JE::Render::RPI::MaterialTypeAssetCreator& materialTypeCreator, AZStd::string propertyGroupPrefix = "");

    AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> CreateCommonTestMaterialSrgLayout();
    JE::Render::RPI::MaterialShaderParameterLayout CreateCommonTestMaterialShaderParameterLayout();

    AZ::RHI::SamplerState GetDefaultSamplerState();
    AZ::RHI::SamplerState GetClampSamplerState();
    AZ::RHI::SamplerState GetPointSamplerState();

} //namespace UnitTest
