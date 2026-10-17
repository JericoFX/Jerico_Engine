/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <JE/Editor/Render/RPI/Edit/Common/ConvertibleSource.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace JE::Render
{
    namespace RPI
    {
        void ConvertibleSource::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ConvertibleSource>()
                    ->SerializeWithNoData()
                    ;
            }
        }


        bool ConvertibleSource::Convert([[maybe_unused]] AZ::TypeId& outTypeId, [[maybe_unused]] AZStd::shared_ptr<void>& outData) const
        {
            return false;
        }
    }
}
