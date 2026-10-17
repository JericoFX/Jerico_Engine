/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Components/ComponentAdapter.h>
#include <JE/Render/Scene/Scripting/EntityReferenceComponentConfig.h>
#include <JE/Render/Scene/Scripting/EntityReferenceConstants.h>
#include <Scripting/EntityReferenceComponentController.h>

namespace JE::Render
{
    namespace Scene
    {
        class EntityReferenceComponent final
            : public AzFramework::Components::ComponentAdapter<EntityReferenceComponentController, EntityReferenceComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<EntityReferenceComponentController, EntityReferenceComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::EntityReferenceComponent, "AZ::Render::EntityReferenceComponent", EntityReferenceComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::EntityReferenceComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::EntityReferenceComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::EntityReferenceComponent, AZ::ComponentAllocator);

            EntityReferenceComponent() = default;
            EntityReferenceComponent(const EntityReferenceComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    }
}
