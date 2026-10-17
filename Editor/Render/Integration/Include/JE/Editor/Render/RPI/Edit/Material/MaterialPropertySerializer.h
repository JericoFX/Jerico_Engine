/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Render/RPI/Edit/Configuration.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        struct MaterialPropertySourceData;
        class MaterialPropertyValue;

        class JE_EDITOR_RENDER_INTEGRATION_API JsonMaterialPropertySerializer
            : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonMaterialPropertySerializer, "{1AFEC5BD-AB2E-4BDB-911B-80727EDA0C26}", AZ::BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;
            AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;

            AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;

        private:
            //! Loads a JSON value into AZStd::variant-based MaterialPropertyValue, using the template data type T
            template<typename T>
            AZ::JsonSerializationResult::ResultCode LoadVariant(MaterialPropertyValue& intoValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);
            template<typename T>
            AZ::JsonSerializationResult::ResultCode LoadVariant(MaterialPropertyValue& intoValue, const T& defaultValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

            //! Loads a property's value fields from JSON, for use with numeric types like int and float, which support range limits.
            template<typename T>
            AZ::JsonSerializationResult::ResultCode LoadNumericValues(MaterialPropertySourceData* intoProperty, const T& defaultValue,
                const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

            //! Loads a property's value fields from JSON, for use with non-numeric types like Vector and Color, which don't support range limits.
            template<typename T>
            AZ::JsonSerializationResult::ResultCode LoadNonNumericValues(MaterialPropertySourceData* intoProperty, const T& defaultValue,
                const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

            AZ::JsonSerializationResult::ResultCode LoadVectorLabels(MaterialPropertySourceData* intoProperty,
                const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

            //! Stores a property's value fields to JSON, for use with numeric types like int and float, which support range limits.
            template<typename T>
            AZ::JsonSerializationResult::ResultCode StoreNumericValues(rapidjson::Value& outputValue,
                const MaterialPropertySourceData* property, const T& defaultValue, AZ::JsonSerializerContext& context);

            //! Stores a property's value fields to JSON, for use with non-numeric types like Vector and Color, which don't support range limits.
            template<typename T>
            AZ::JsonSerializationResult::ResultCode StoreNonNumericValues(rapidjson::Value& outputValue,
                const MaterialPropertySourceData* property, const T& defaultValue, AZ::JsonSerializerContext& context);

            AZ::JsonSerializationResult::ResultCode StoreVectorLabels(rapidjson::Value& outputValue,
                const MaterialPropertySourceData* property, AZ::JsonSerializerContext& context);
        };

    }

namespace AZ
{ // namespace RPI
} // namespace AZ
