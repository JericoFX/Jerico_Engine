/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <Decals/DecalComponentController.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <JE/Render/Scene/Decals/DecalConstants.h>

namespace JE::Render
{
    namespace Scene
    {
        class DecalComponent final
            : public AzFramework::Components::ComponentAdapter<DecalComponentController, DecalComponentConfig>
        {
        public:

            using BaseClass = AzFramework::Components::ComponentAdapter<DecalComponentController, DecalComponentConfig>;
            AZ_RTTI_WITH_NAME(JE::Render::Scene::DecalComponent, "AZ::Render::DecalComponent", DecalComponentTypeId, BaseClass, AZ::Component)
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(JE::Render::Scene::DecalComponent)
            AZ_COMPONENT_BASE(JE::Render::Scene::DecalComponent)
            AZ_CLASS_ALLOCATOR(JE::Render::Scene::DecalComponent, AZ::ComponentAllocator);

            DecalComponent() = default;
            DecalComponent(const DecalComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
