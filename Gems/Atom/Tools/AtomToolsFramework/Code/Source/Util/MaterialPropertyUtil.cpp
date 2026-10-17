/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/SamplerState.h>
#include <JE/Editor/Render/RPI/Edit/Common/AssetUtils.h>
#include <JE/Render/RPI/Reflect/Image/AttachmentImageAsset.h>
#include <JE/Render/RPI/Reflect/Image/ImageAsset.h>
#include <JE/Render/RPI/Reflect/Image/StreamingImageAsset.h>
#include <JE/Render/RPI/Reflect/Material/MaterialAsset.h>
#include <JE/Render/RPI/Reflect/Material/MaterialTypeAsset.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AtomToolsFramework
{
    JE::Render::RPI::MaterialPropertyValue ConvertToRuntimeType(const AZStd::any& value)
    {
        return JE::Render::RPI::MaterialPropertyValue::FromAny(value);
    }

    AZStd::any ConvertToEditableType(const JE::Render::RPI::MaterialPropertyValue& value)
    {
        return JE::Render::RPI::MaterialPropertyValue::ToAny(value);
    }

    void ConvertToPropertyConfig(AtomToolsFramework::DynamicPropertyConfig& propertyConfig, const JE::Render::RPI::MaterialPropertySourceData& propertyDefinition)
    {
        propertyConfig.m_name = propertyDefinition.GetName();
        propertyConfig.m_displayName = propertyDefinition.m_displayName;
        propertyConfig.m_description = propertyDefinition.m_description;
        propertyConfig.m_defaultValue = ConvertToEditableType(propertyDefinition.m_value);
        propertyConfig.m_min = ConvertToEditableType(propertyDefinition.m_min);
        propertyConfig.m_max = ConvertToEditableType(propertyDefinition.m_max);
        propertyConfig.m_softMin = ConvertToEditableType(propertyDefinition.m_softMin);
        propertyConfig.m_softMax = ConvertToEditableType(propertyDefinition.m_softMax);
        propertyConfig.m_step = ConvertToEditableType(propertyDefinition.m_step);
        propertyConfig.m_enumValues = propertyDefinition.m_enumValues;
        propertyConfig.m_vectorLabels = propertyDefinition.m_vectorLabels;
        propertyConfig.m_visible = propertyDefinition.m_visibility != JE::Render::RPI::MaterialPropertyVisibility::Hidden;
        propertyConfig.m_readOnly = propertyDefinition.m_visibility == JE::Render::RPI::MaterialPropertyVisibility::Disabled;

        // Use customized property handler (ImageAssetPropertyHandler) for image asset
        if (propertyDefinition.m_dataType == JE::Render::RPI::MaterialPropertyDataType::Image)
        {
            propertyConfig.m_supportedAssetTypes.push_back(azrtti_typeid<JE::Render::RPI::AttachmentImageAsset>());
            propertyConfig.m_supportedAssetTypes.push_back(azrtti_typeid<JE::Render::RPI::StreamingImageAsset>());
        }

        // Update the description for material properties to include script name assuming id is set beforehand
        propertyConfig.m_description = AZStd::string::format(
            "%s%s(Script Name = '%s')",
            propertyConfig.m_description.c_str(),
            propertyConfig.m_description.empty() ? "" : "\n",
            propertyConfig.m_id.GetCStr());
    }

    void ConvertToPropertyConfig(AtomToolsFramework::DynamicPropertyConfig& propertyConfig, const JE::Render::RPI::MaterialPropertyDynamicMetadata& propertyMetaData)
    {
        propertyConfig.m_description = propertyMetaData.m_description;
        propertyConfig.m_min = ConvertToEditableType(propertyMetaData.m_propertyRange.m_min);
        propertyConfig.m_max = ConvertToEditableType(propertyMetaData.m_propertyRange.m_max);
        propertyConfig.m_softMin = ConvertToEditableType(propertyMetaData.m_propertyRange.m_softMin);
        propertyConfig.m_softMax = ConvertToEditableType(propertyMetaData.m_propertyRange.m_softMax);
        propertyConfig.m_visible = propertyMetaData.m_visibility != JE::Render::RPI::MaterialPropertyVisibility::Hidden;
        propertyConfig.m_readOnly = propertyMetaData.m_visibility == JE::Render::RPI::MaterialPropertyVisibility::Disabled;
    }

    void ConvertToPropertyMetaData(JE::Render::RPI::MaterialPropertyDynamicMetadata& propertyMetaData, const AtomToolsFramework::DynamicPropertyConfig& propertyConfig)
    {
        propertyMetaData.m_description = propertyConfig.m_description;
        propertyMetaData.m_propertyRange.m_min = ConvertToRuntimeType(propertyConfig.m_min);
        propertyMetaData.m_propertyRange.m_max = ConvertToRuntimeType(propertyConfig.m_max);
        propertyMetaData.m_propertyRange.m_softMin = ConvertToRuntimeType(propertyConfig.m_softMin);
        propertyMetaData.m_propertyRange.m_softMax = ConvertToRuntimeType(propertyConfig.m_softMax);
        if (!propertyConfig.m_visible)
        {
            propertyMetaData.m_visibility = JE::Render::RPI::MaterialPropertyVisibility::Hidden;
        }
        else if (propertyConfig.m_readOnly)
        {
            propertyMetaData.m_visibility = JE::Render::RPI::MaterialPropertyVisibility::Disabled;
        }
        else
        {
            propertyMetaData.m_visibility = JE::Render::RPI::MaterialPropertyVisibility::Enabled;
        }
    }

    template<typename T>
    bool ComparePropertyValues(const AZStd::any& valueA, const AZStd::any& valueB)
    {
        return valueA.is<T>() && valueB.is<T>() && *AZStd::any_cast<T>(&valueA) == *AZStd::any_cast<T>(&valueB);
    }

    bool ArePropertyValuesEqual(const AZStd::any& valueA, const AZStd::any& valueB)
    {
        if (valueA.type() != valueB.type())
        {
            return false;
        }

        if (ComparePropertyValues<bool>(valueA, valueB) ||
            ComparePropertyValues<int32_t>(valueA, valueB) ||
            ComparePropertyValues<uint32_t>(valueA, valueB) ||
            ComparePropertyValues<float>(valueA, valueB) ||
            ComparePropertyValues<double>(valueA, valueB) ||
            ComparePropertyValues<AZ::Vector2>(valueA, valueB) ||
            ComparePropertyValues<AZ::Vector3>(valueA, valueB) ||
            ComparePropertyValues<AZ::Vector4>(valueA, valueB) ||
            ComparePropertyValues<AZ::Matrix3x3>(valueA, valueB) ||
            ComparePropertyValues<AZ::Matrix3x4>(valueA, valueB) ||
            ComparePropertyValues<AZ::Matrix4x4>(valueA, valueB) ||
            ComparePropertyValues<AZ::Color>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::AssetId>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<AZ::Data::AssetData>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<JE::Render::RPI::ImageAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<JE::Render::RPI::AttachmentImageAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<JE::Render::RPI::MaterialAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZ::Data::Asset<JE::Render::RPI::MaterialTypeAsset>>(valueA, valueB) ||
            ComparePropertyValues<AZStd::string>(valueA, valueB))
        {
            return true;
        }

        return false;
    }

    bool ConvertToExportFormat(
        const AZStd::string& exportPath,
        [[maybe_unused]] const AZ::Name& propertyId,
        const JE::Render::RPI::MaterialPropertySourceData& propertyDefinition,
        JE::Render::RPI::MaterialPropertyValue& propertyValue)
    {
        if (propertyDefinition.m_dataType == JE::Render::RPI::MaterialPropertyDataType::Enum)
        {
            if (propertyDefinition.m_enumValues.empty())
            {
                AZ_Error("AtomToolsFramework", false, "No enum values are specified for property: '%s'.", propertyId.GetCStr());
                return false;
            }

            if (propertyValue.Is<uint32_t>())
            {
                const uint32_t index = propertyValue.GetValue<uint32_t>();
                if (index >= propertyDefinition.m_enumValues.size())
                {
                    AZ_Warning(
                        "AtomToolsFramework",
                        false,
                        "Invalid value for material enough property, using default: '%s'.",
                        propertyId.GetCStr());
                    propertyValue = propertyDefinition.m_enumValues[0];
                    return true;
                }

                propertyValue = propertyDefinition.m_enumValues[index];
                return true;
            }

            if (propertyValue.Is<AZStd::string>())
            {
                const AZStd::string value = propertyValue.GetValue<AZStd::string>();
                if (AZStd::find(propertyDefinition.m_enumValues.begin(), propertyDefinition.m_enumValues.end(), value) ==
                    propertyDefinition.m_enumValues.end())
                {
                    AZ_Warning(
                        "AtomToolsFramework",
                        false,
                        "Invalid value for material enough property, using default: '%s'.",
                        propertyId.GetCStr());
                    propertyValue = propertyDefinition.m_enumValues[0];
                    return true;
                }

                propertyValue = value;
                return true;
            }

            AZ_Error(
                "AtomToolsFramework", false, "Property is of data type enum but value data type is not supported: '%s'.", propertyId.GetCStr());
            return false;
        }

        // Image asset references must be converted from asset IDs to a relative source file path
        if (propertyDefinition.m_dataType == JE::Render::RPI::MaterialPropertyDataType::Image)
        {
            AZStd::string imagePath;
            AZ::Data::AssetId imageAssetId;

            if (propertyValue.Is<AZ::Data::Asset<JE::Render::RPI::ImageAsset>>())
            {
                const auto& imageAsset = propertyValue.GetValue<AZ::Data::Asset<JE::Render::RPI::ImageAsset>>();
                imageAssetId = imageAsset.GetId();
            }

            if (propertyValue.Is<AZ::Data::Instance<JE::Render::RPI::Image>>())
            {
                const auto& image = propertyValue.GetValue<AZ::Data::Instance<JE::Render::RPI::Image>>();
                if (image)
                {
                    imageAssetId = image->GetAssetId();
                }
            }
            
            imagePath = JE::Render::RPI::AssetUtils::GetSourcePathByAssetId(imageAssetId);

            if (imageAssetId.IsValid() && imagePath.empty())
            {
                AZ_Error("AtomToolsFramework", false, "Image asset could not be found for property: '%s'.", propertyId.GetCStr());
                return false;
            }

            propertyValue = GetPathToExteralReference(exportPath, imagePath);
            return true;
        }

        return true;
    }

    JE::Render::RPI::MaterialPropertyDataType GetMaterialPropertyDataTypeFromValue(
        JE::Render::RPI::MaterialPropertyValue& propertyValue, bool hasEnumValues)
    {
        if (propertyValue.Is<bool>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Bool;
        }
        if (propertyValue.Is<AZ::s32>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Int;
        }
        if (propertyValue.Is<AZ::u32>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::UInt;
        }
        if (propertyValue.Is<float>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Float;
        }
        if (propertyValue.Is<AZ::Vector2>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Vector2;
        }
        if (propertyValue.Is<AZ::Vector3>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Vector3;
        }
        if (propertyValue.Is<AZ::Vector4>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Vector4;
        }
        if (propertyValue.Is<AZ::Color>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Color;
        }
        if (propertyValue.Is<AZ::Data::Asset<JE::Render::RPI::ImageAsset>>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Image;
        }
        if (propertyValue.Is<AZ::Data::Instance<JE::Render::RPI::Image>>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::Image;
        }
        if (propertyValue.Is<AZ::RHI::SamplerState>())
        {
            return JE::Render::RPI::MaterialPropertyDataType::SamplerState;
        }
        if (propertyValue.Is<AZStd::string>())
        {
            return hasEnumValues ? JE::Render::RPI::MaterialPropertyDataType::Enum : JE::Render::RPI::MaterialPropertyDataType::Image;
        }

        return JE::Render::RPI::MaterialPropertyDataType::Invalid;
    }
} // namespace AtomToolsFramework
