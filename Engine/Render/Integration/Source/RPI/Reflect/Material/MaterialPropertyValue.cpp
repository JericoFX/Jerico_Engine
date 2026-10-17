/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Render/RPI/Reflect/Material/MaterialPropertyValue.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <JE/Render/RPI/Reflect/Image/AttachmentImageAsset.h>

namespace JE::Render
{
    namespace RPI
    {
        // clang-format off
        static_assert((
            AZStd::is_same_v<AZStd::monostate,                   AZStd::variant_alternative_t<0, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<bool,                               AZStd::variant_alternative_t<1, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<int32_t,                            AZStd::variant_alternative_t<2, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<uint32_t,                           AZStd::variant_alternative_t<3, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<float,                              AZStd::variant_alternative_t<4, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<AZ::Vector2,                            AZStd::variant_alternative_t<5, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<AZ::Vector3,                            AZStd::variant_alternative_t<6, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<AZ::Vector4,                            AZStd::variant_alternative_t<7, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<AZ::Color,                              AZStd::variant_alternative_t<8, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<AZ::Data::Asset<ImageAsset>,            AZStd::variant_alternative_t<9, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<AZ::Data::Instance<Image>,              AZStd::variant_alternative_t<10, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<AZStd::string,                      AZStd::variant_alternative_t<11, MaterialPropertyValue::ValueType>> &&
            AZStd::is_same_v<AZ::RHI::SamplerState,                  AZStd::variant_alternative_t<12, MaterialPropertyValue::ValueType>>),
            "Types must be in the order of the type ID array.");
        // clang-format on

        void MaterialPropertyValue::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<ValueType>();

                serializeContext->Class<MaterialPropertyValue>()
                    ->Version(1)
                    ->Field("Value", &MaterialPropertyValue::m_value)
                    ;
            }
        }

        AZ::TypeId MaterialPropertyValue::GetTypeId() const
        {
            // Must sort in the same order defined in the variant.
            static const AZ::TypeId PropertyValueTypeIds[] = {
                azrtti_typeid<AZStd::monostate>(),
                azrtti_typeid<bool>(),
                azrtti_typeid<int32_t>(),
                azrtti_typeid<uint32_t>(),
                azrtti_typeid<float>(),
                azrtti_typeid<AZ::Vector2>(),
                azrtti_typeid<AZ::Vector3>(),
                azrtti_typeid<AZ::Vector4>(),
                azrtti_typeid<AZ::Color>(),
                azrtti_typeid<AZ::Data::Asset<ImageAsset>>(),
                azrtti_typeid<AZ::Data::Instance<Image>>(),
                azrtti_typeid<AZStd::string>(),
                azrtti_typeid<AZ::RHI::SamplerState>(),
            };

            return PropertyValueTypeIds[m_value.index()];
        }

        MaterialPropertyValue MaterialPropertyValue::FromAny(const AZStd::any& value)
        {
            MaterialPropertyValue result;

            if (value.empty())
            {
                return result;
            }

            if (value.is<bool>())
            {
                result.m_value = AZStd::any_cast<bool>(value);
            }
            else if (value.is<int32_t>())
            {
                result.m_value = AZStd::any_cast<int32_t>(value);
            }
            else if (value.is<uint32_t>())
            {
                result.m_value = AZStd::any_cast<uint32_t>(value);
            }
            else if (value.is<float>())
            {
                result.m_value = AZStd::any_cast<float>(value);
            }
            else if (value.is<double>())
            {
                result.m_value = aznumeric_cast<float>(AZStd::any_cast<double>(value));
            }
            else if (value.is<AZ::Vector2>())
            {
                result.m_value = AZStd::any_cast<AZ::Vector2>(value);
            }
            else if (value.is<AZ::Vector3>())
            {
                result.m_value = AZStd::any_cast<AZ::Vector3>(value);
            }
            else if (value.is<AZ::Vector4>())
            {
                result.m_value = AZStd::any_cast<AZ::Vector4>(value);
            }
            else if (value.is<AZ::Color>())
            {
                result.m_value = AZStd::any_cast<AZ::Color>(value);
            }
            else if (value.is<AZ::Data::AssetId>())
            {
                auto assetId = AZStd::any_cast<AZ::Data::AssetId>(value);
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
                if (assetInfo.m_assetId.IsValid())
                {
                    result.m_value = AZ::Data::Asset<JE::Render::RPI::ImageAsset>(
                        assetId,
                        assetInfo.m_assetType,
                        assetInfo.m_relativePath.c_str());
                }
                else
                {
                    result.m_value = AZ::Data::Asset<JE::Render::RPI::ImageAsset>(assetId, azrtti_typeid<JE::Render::RPI::StreamingImageAsset>());
                }
            }
            else if (value.is<AZ::Data::Asset<AZ::Data::AssetData>>())
            {
                auto asset = AZStd::any_cast<AZ::Data::Asset<AZ::Data::AssetData>>(value);

                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, asset.GetId());
                if (assetInfo.m_assetId.IsValid())
                {
                    result.m_value = AZ::Data::Asset<JE::Render::RPI::ImageAsset>(assetInfo.m_assetId, assetInfo.m_assetType, assetInfo.m_relativePath);
                }
                else
                {
                    result.m_value = asset;
                }
            }
            else if (value.is<AZ::Data::Asset<StreamingImageAsset>>())
            {
                auto asset = AZStd::any_cast<AZ::Data::Asset<JE::Render::RPI::StreamingImageAsset>>(value);
                result.m_value = AZ::Data::Asset<JE::Render::RPI::ImageAsset>(asset.GetId(), azrtti_typeid<JE::Render::RPI::StreamingImageAsset>(), asset.GetHint());
            }
            else if (value.is<AZ::Data::Asset<AttachmentImageAsset>>())
            {
                auto asset = AZStd::any_cast<AZ::Data::Asset<JE::Render::RPI::AttachmentImageAsset>>(value);
                result.m_value = AZ::Data::Asset<JE::Render::RPI::ImageAsset>(asset.GetId(), azrtti_typeid<JE::Render::RPI::AttachmentImageAsset>(), asset.GetHint());
            }
            else if (value.is<AZ::Data::Asset<ImageAsset>>())
            {
                result.m_value = AZStd::any_cast<AZ::Data::Asset<ImageAsset>>(value);
            }
            else if (value.is<AZ::Data::Instance<Image>>())
            {
                result.m_value = AZStd::any_cast<AZ::Data::Instance<Image>>(value);
            }
            else if (value.is<AZStd::string>())
            {
                result.m_value = AZStd::any_cast<AZStd::string>(value);
            }
            else if (value.is<AZ::RHI::SamplerState>())
            {
                result.m_value = AZStd::any_cast<AZ::RHI::SamplerState>(value);
            }
            else
            {
                AZ_Warning(
                    "MaterialPropertyValue", false, "Cannot convert any to variant. Type in any is: %s.",
                    value.get_type_info().m_id.ToString<AZStd::string>().data());
            }

            return result;
        }

        AZStd::any MaterialPropertyValue::ToAny(const MaterialPropertyValue& value)
        {
            AZStd::any result;

            if (AZStd::holds_alternative<bool>(value.m_value))
            {
                result = AZStd::get<bool>(value.m_value);
            }
            else if (AZStd::holds_alternative<int32_t>(value.m_value))
            {
                result = AZStd::get<int32_t>(value.m_value);
            }
            else if (AZStd::holds_alternative<uint32_t>(value.m_value))
            {
                result = AZStd::get<uint32_t>(value.m_value);
            }
            else if (AZStd::holds_alternative<float>(value.m_value))
            {
                result = AZStd::get<float>(value.m_value);
            }
            else if (AZStd::holds_alternative<AZ::Vector2>(value.m_value))
            {
                result = AZStd::get<AZ::Vector2>(value.m_value);
            }
            else if (AZStd::holds_alternative<AZ::Vector3>(value.m_value))
            {
                result = AZStd::get<AZ::Vector3>(value.m_value);
            }
            else if (AZStd::holds_alternative<AZ::Vector4>(value.m_value))
            {
                result = AZStd::get<AZ::Vector4>(value.m_value);
            }
            else if (AZStd::holds_alternative<AZ::Color>(value.m_value))
            {
                result = AZStd::get<AZ::Color>(value.m_value);
            }
            else if (AZStd::holds_alternative<AZ::Data::Asset<ImageAsset>>(value.m_value))
            {
                result = AZStd::get<AZ::Data::Asset<ImageAsset>>(value.m_value);
            }
            else if (AZStd::holds_alternative<AZ::Data::Instance<Image>>(value.m_value))
            {
                result = AZStd::get<AZ::Data::Instance<Image>>(value.m_value);
            }
            else if (AZStd::holds_alternative<AZStd::string>(value.m_value))
            {
                result = AZStd::get<AZStd::string>(value.m_value);
            }
            else if (AZStd::holds_alternative<AZ::RHI::SamplerState>(value.m_value))
            {
                result = AZStd::get<AZ::RHI::SamplerState>(value.m_value);
            }

            return result;
        }

        //! Attempts to convert a numeric MaterialPropertyValue to another numeric type @T.
        //! If the original MaterialPropertyValue is not a numeric type, the original value is returned.
        template<typename T>
        static MaterialPropertyValue CastNumericMaterialPropertyValue(const MaterialPropertyValue& value)
        {
            AZ::TypeId typeId = value.GetTypeId();

            if (typeId == azrtti_typeid<bool>())
            {
                return aznumeric_cast<T>(value.GetValue<bool>());
            }
            else if (typeId == azrtti_typeid<int32_t>())
            {
                return aznumeric_cast<T>(value.GetValue<int32_t>());
            }
            else if (typeId == azrtti_typeid<uint32_t>())
            {
                return aznumeric_cast<T>(value.GetValue<uint32_t>());
            }
            else if (typeId == azrtti_typeid<float>())
            {
                return aznumeric_cast<T>(value.GetValue<float>());
            }
            else
            {
                return value;
            }
        }

        //! Attempts to convert an AZ::Vector[2-4] MaterialPropertyValue to another AZ::Vector[2-4] type @T.
        //! Any extra elements will be dropped or set to 0.0 as needed.
        //! If the original MaterialPropertyValue is not a Vector type, the original value is returned.
        template<typename VectorT>
        static MaterialPropertyValue CastVectorMaterialPropertyValue(const MaterialPropertyValue& value)
        {
            float values[4] = {};

            AZ::TypeId typeId = value.GetTypeId();
            if (typeId == azrtti_typeid<AZ::Vector2>())
            {
                value.GetValue<AZ::Vector2>().StoreToFloat2(values);
            }
            else if (typeId == azrtti_typeid<AZ::Vector3>())
            {
                value.GetValue<AZ::Vector3>().StoreToFloat3(values);
            }
            else if (typeId == azrtti_typeid<AZ::Vector4>())
            {
                value.GetValue<AZ::Vector4>().StoreToFloat4(values);
            }
            else
            {
                return value;
            }

            typeId = azrtti_typeid<VectorT>();
            if (typeId == azrtti_typeid<AZ::Vector2>())
            {
                return AZ::Vector2::CreateFromFloat2(values);
            }
            else if (typeId == azrtti_typeid<AZ::Vector3>())
            {
                return AZ::Vector3::CreateFromFloat3(values);
            }
            else if (typeId == azrtti_typeid<AZ::Vector4>())
            {
                return AZ::Vector4::CreateFromFloat4(values);
            }
            else
            {
                return value;
            }
        }

        MaterialPropertyValue MaterialPropertyValue::CastToType(AZ::TypeId requestedType) const
        {
            if (requestedType == azrtti_typeid<bool>())
            {
                return CastNumericMaterialPropertyValue<bool>(*this);
            }
            else if (requestedType == azrtti_typeid<int32_t>())
            {
                return CastNumericMaterialPropertyValue<int32_t>(*this);
            }
            else if (requestedType == azrtti_typeid<uint32_t>())
            {
                return CastNumericMaterialPropertyValue<uint32_t>(*this);
            }
            else if (requestedType == azrtti_typeid<float>())
            {
                return CastNumericMaterialPropertyValue<float>(*this);
            }
            else if (requestedType == azrtti_typeid<AZ::Vector2>())
            {
                return CastVectorMaterialPropertyValue<AZ::Vector2>(*this);
            }
            else if (requestedType == azrtti_typeid<AZ::Vector3>())
            {
                if (GetTypeId() == azrtti_typeid<AZ::Color>())
                {
                    return GetValue<AZ::Color>().GetAsVector3();
                }
                else
                {
                    return CastVectorMaterialPropertyValue<AZ::Vector3>(*this);
                }
            }
            else if (requestedType == azrtti_typeid<AZ::Vector4>())
            {
                if (GetTypeId() == azrtti_typeid<AZ::Color>())
                {
                    return GetValue<AZ::Color>().GetAsVector4();
                }
                else
                {
                    return CastVectorMaterialPropertyValue<AZ::Vector4>(*this);
                }
            }
            else if (requestedType == azrtti_typeid<AZ::Color>())
            {
                if (GetTypeId() == azrtti_typeid<AZ::Vector3>())
                {
                    return AZ::Color::CreateFromVector3(GetValue<AZ::Vector3>());
                }
                else if (GetTypeId() == azrtti_typeid<AZ::Vector4>())
                {
                    AZ::Vector4 vector4 = GetValue<AZ::Vector4>();
                    return AZ::Color::CreateFromVector3AndFloat(vector4.GetAsVector3(), vector4.GetW());
                }
                else
                {
                    // Don't attempt conversion from e.g. Vector2 as that makes little sense.
                    return *this;
                }
            }
            else
            {
                // remaining types are non-numerical and cannot be cast to other types: return as-is
                return *this;
            }
        }
    } // namespace RPI
} // namespace AZ
