/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Render/RPI/Edit/Configuration.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialFunctorSourceData.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        class JE_EDITOR_RENDER_INTEGRATION_API JsonMaterialFunctorSourceDataSerializer
            : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI_WITH_NAME(JE::Render::RPI::JsonMaterialFunctorSourceDataSerializer, "AZ::RPI::JsonMaterialFunctorSourceDataSerializer", "{56F2F0D8-165C-45BB-9FE7-54FA90148FE9}", AZ::BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;
            
            AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;

            AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;
        private:
            AZ::BaseJsonSerializer::OperationFlags GetOperationsFlags() const override;
        };

    }

namespace AZ
{ // namespace RPI
} // namespace AZ
