/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace JE::Editor::Render
{
    namespace Scene
    {
        // Custom JSON serializer for Mesh Stats
        // Mesh stats should not be serialized with EditorMeshComponent, because they are calculated based on active model,
        // so the serializer simply provides empty load/store functions
        class JsonEditorMeshStatsSerializer : public AZ::BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonEditorMeshStatsSerializer, "{571290F5-98C9-45AE-BC8C-E40968B5BA6F}", AZ::BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            AZ::JsonSerializationResult::Result Load(
                void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                AZ::JsonDeserializerContext& context) override;

            AZ::JsonSerializationResult::Result Store(
                rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, const AZ::Uuid& valueTypeId,
                AZ::JsonSerializerContext& context) override;            
        };
    } // namespace Render
} // namespace AZ
