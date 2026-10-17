/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertySourceData.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertySerializer.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertyConnectionSerializer.h>
#include <JE/Editor/Render/RPI/Edit/Material/MaterialPropertyValueSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

namespace JE::Render
{
    namespace RPI
    {
        void MaterialPropertySourceData::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonMaterialPropertySerializer>()->HandlesType<MaterialPropertySourceData>();
                jsonContext->Serializer<JsonMaterialPropertyConnectionSerializer>()->HandlesType<MaterialPropertySourceData::Connection>();
                jsonContext->Serializer<JsonMaterialPropertyValueSerializer>()->HandlesType<MaterialPropertyValue>();
            }
            else if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Connection>()->Version(3);
                serializeContext->Class<MaterialPropertySourceData>()->Version(1);

                serializeContext->RegisterGenericType<AZStd::unique_ptr<MaterialPropertySourceData>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<MaterialPropertySourceData>>>();
                serializeContext->RegisterGenericType<ConnectionList>();
            }
        }

        MaterialPropertySourceData::MaterialPropertySourceData(AZStd::string_view name) : m_name(name)
        {
        }

        const AZStd::string& MaterialPropertySourceData::GetName() const
        {
            return m_name;
        }

        MaterialPropertySourceData::Connection::Connection(MaterialPropertyOutputType type, AZStd::string_view name)
            : m_type(type)
            , m_name(name)
        {
        }
    } // namespace RPI
} // namespace AZ
