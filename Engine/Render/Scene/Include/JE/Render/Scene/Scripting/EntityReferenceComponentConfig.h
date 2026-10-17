/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>

namespace JE::Render
{
    namespace Scene
    {
        class EntityReferenceComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(EntityReferenceComponentConfig, AZ::SystemAllocator)
            AZ_RTTI_WITH_NAME(JE::Render::Scene::EntityReferenceComponentConfig, "AZ::Render::EntityReferenceComponentConfig", "{12D214C7-878A-48D2-AFDD-4FCFF0BBC876}", AZ::ComponentConfig);

            static void Reflect(AZ::ReflectContext* context);

            AZStd::vector<AZ::EntityId> m_entityIdReferences;
        };
    }
}
