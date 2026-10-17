/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <JE/Editor/Render/RPI/Edit/Configuration.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialTypeSourceData.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class ReflectContext;

    }

namespace JE::Render::RPI
{
        //! The property connection itself is rather simple, but we need this custom serializer to provide backward compatibility
        //! for when the "id" key was changed to "name". If the JSON serialization system is ever updated to provide built-in
        //! support for versioning, then we can probably remove this class.
        class JE_EDITOR_RENDER_INTEGRATION_API JsonMaterialPropertyConnectionSerializer
            : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonMaterialPropertyConnectionSerializer, "{2B7F00CF-51F7-4409-9C0E-914E59696FB9}", AZ::BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;

            AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;
        };

    }

namespace AZ
{ // namespace RPI
} // namespace AZ
