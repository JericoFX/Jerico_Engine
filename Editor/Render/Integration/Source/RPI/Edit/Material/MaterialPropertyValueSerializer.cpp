/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertyValueSerializer.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertySerializer.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertyId.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

namespace JE::Render
{
    namespace RPI
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialPropertyValueSerializer, AZ::SystemAllocator);

        template<typename T>
        AZ::JsonSerializationResult::ResultCode JsonMaterialPropertyValueSerializer::LoadVariant(
            MaterialPropertyValue& intoValue,
            const T& defaultValue,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context)
        {
            T value = defaultValue;
            AZ::JsonSerializationResult::ResultCode result = ContinueLoading(&value, azrtti_typeid<T>(), inputValue, context);
            intoValue = value;
            return result;
        }

        AZ::JsonSerializationResult::Result JsonMaterialPropertyValueSerializer::Load(void* outputValue, const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(azrtti_typeid<MaterialPropertyValue>() == outputValueTypeId,
                "Unable to deserialize material property value to json because the provided type is %s",
                outputValueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(outputValueTypeId);

            MaterialPropertyValue* property = reinterpret_cast<MaterialPropertyValue*>(outputValue);
            AZ_Assert(property, "Output value for JsonMaterialPropertyValueSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);

            if (inputValue.IsBool())
            {
                result.Combine(LoadVariant<bool>(*property, false, inputValue, context));
            }
            else if (inputValue.IsInt() || inputValue.IsInt64())
            {
                result.Combine(LoadVariant<int32_t>(*property, 0, inputValue, context));
            }
            else if (inputValue.IsUint() || inputValue.IsUint64())
            {
                result.Combine(LoadVariant<uint32_t>(*property, 0u, inputValue, context));
            }
            else if (inputValue.IsFloat() || inputValue.IsDouble())
            {
                result.Combine(LoadVariant<float>(*property, 0.0f, inputValue, context));
            }
            else if (inputValue.IsArray() && inputValue.Size() == 4)
            {
                result.Combine(LoadVariant<AZ::Vector4>(*property, AZ::Vector4{0.0f, 0.0f, 0.0f, 0.0f}, inputValue, context));
            }
            else if (inputValue.IsArray() && inputValue.Size() == 3)
            {
                result.Combine(LoadVariant<AZ::Vector3>(*property, AZ::Vector3{0.0f, 0.0f, 0.0f}, inputValue, context));
            }
            else if (inputValue.IsArray() && inputValue.Size() == 2)
            {
                result.Combine(LoadVariant<AZ::Vector2>(*property, AZ::Vector2{0.0f, 0.0f}, inputValue, context));
            }
            else if (inputValue.IsObject())
            {
                AZ::JsonSerializationResult::ResultCode resultCode = LoadVariant<AZ::Color>(*property, AZ::Color::CreateZero(), inputValue, context);
                
                if(resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
                {
                    resultCode = LoadVariant<AZ::Vector4>(*property, AZ::Vector4::CreateZero(), inputValue, context);
                }
                
                if(resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
                {
                    resultCode = LoadVariant<AZ::Vector3>(*property, AZ::Vector3::CreateZero(), inputValue, context);
                }
                
                if(resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
                {
                    resultCode = LoadVariant<AZ::Vector2>(*property, AZ::Vector2::CreateZero(), inputValue, context);
                }

                if(resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
                {
                    resultCode = LoadVariant<AZ::RHI::SamplerState>(*property, AZ::RHI::SamplerState{}, inputValue, context);
                }

                if(resultCode.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
                {
                    result.Combine(resultCode);
                }
                else
                {
                    return context.Report(AZ::JsonSerializationResult::Tasks::ReadField, AZ::JsonSerializationResult::Outcomes::Unsupported, "Unknown data type");
                }
            }
            else if (inputValue.IsString())
            {
                result.Combine(LoadVariant<AZStd::string>(*property, AZStd::string{}, inputValue, context));
            }
            else
            {
                return context.Report(AZ::JsonSerializationResult::Tasks::ReadField, AZ::JsonSerializationResult::Outcomes::Unsupported, "Unknown data type");
            }

            if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
            {
                return context.Report(result, "Successfully loaded property value.");
            }
            else
            {
                return context.Report(result, "Partially loaded property value.");
            }
        }

        AZ::JsonSerializationResult::Result JsonMaterialPropertyValueSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
            [[maybe_unused]] const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(azrtti_typeid<MaterialPropertyValue>() == valueTypeId,
                "Unable to serialize material property value to json because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(valueTypeId);

            const MaterialPropertyValue* property = reinterpret_cast<const MaterialPropertyValue*>(inputValue);
            AZ_Assert(property, "Input value for JsonMaterialPropertyValueSerializer can't be null.");
            
            JSR::ResultCode result(JSR::Tasks::WriteValue);

            if (property->Is<bool>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<bool>(), nullptr, azrtti_typeid<bool>(), context));
            }
            else if (property->Is<int32_t>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<int32_t>(), nullptr, azrtti_typeid<int32_t>(), context));
            }
            else if (property->Is<uint32_t>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<uint32_t>(), nullptr, azrtti_typeid<uint32_t>(), context));
            }
            else if (property->Is<float>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<float>(), nullptr, azrtti_typeid<float>(), context));
            }
            else if (property->Is<AZ::Vector2>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<AZ::Vector2>(), nullptr, azrtti_typeid<AZ::Vector2>(), context));
            }
            else if (property->Is<AZ::Vector3>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<AZ::Vector3>(), nullptr, azrtti_typeid<AZ::Vector3>(), context));
            }
            else if (property->Is<AZ::Vector4>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<AZ::Vector4>(), nullptr, azrtti_typeid<AZ::Vector4>(), context));
            }
            else if (property->Is<AZ::Color>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<AZ::Color>(), nullptr, azrtti_typeid<AZ::Color>(), context));
            }
            else if (property->Is<AZStd::string>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<AZStd::string>(), nullptr, azrtti_typeid<AZStd::string>(), context));
            }
            else if (property->Is<AZ::RHI::SamplerState>())
            {
                result.Combine(ContinueStoring(outputValue, &property->GetValue<AZ::RHI::SamplerState>(), nullptr, azrtti_typeid<AZ::RHI::SamplerState>(), context));
            }
            else
            {
                result.Combine(context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Unsupported,
                    AZStd::string::format("MaterialPropertyValue type %s is not supported.", property->GetTypeId().ToString<AZStd::string>().c_str())));
            }

            if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
            {
                return context.Report(result, "Successfully stored property value.");
            }
            else
            {
                return context.Report(result, "Partially stored property value.");
            }
        }

    } // namespace RPI
} // namespace AZ
