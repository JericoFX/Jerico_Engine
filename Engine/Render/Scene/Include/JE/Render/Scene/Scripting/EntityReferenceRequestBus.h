/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace JE::Render
{
    namespace Scene
    {
        class EntityReferenceRequests
            : public AZ::ComponentBus
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual ~EntityReferenceRequests() = default;

            virtual AZStd::vector<AZ::EntityId> GetEntityReferences() const = 0;
        };

        using EntityReferenceRequestBus = AZ::EBus<EntityReferenceRequests>;
    }
}
